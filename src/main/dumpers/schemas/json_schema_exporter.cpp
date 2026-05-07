/**
 * =============================================================================
 * DumpSource2
 * Copyright (C) 2026 ValveResourceFormat Contributors
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "json_schema_exporter.h"
#include "globalvariables.h"
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

using ojson = nlohmann::ordered_json;

namespace Dumpers::Schemas::JsonSchemaExporter
{

namespace
{

#if defined(GAME_CS2)
constexpr const char* kGameToken = "cs2";
#elif defined(GAME_DOTA)
constexpr const char* kGameToken = "dota2";
#elif defined(GAME_DEADLOCK)
constexpr const char* kGameToken = "deadlock";
#else
constexpr const char* kGameToken = "source2";
#endif

inline std::string ExtPrefix()
{
	return std::string("x-") + kGameToken + "-";
}

inline std::string Ext(const char* suffix)
{
	return ExtPrefix() + suffix;
}

// Strip whitespace inside angle brackets so "CHandle< X >" becomes "CHandle<X>".
// Whitespace outside <...> is preserved (e.g. "unsigned int").
std::string NormaliseTypeString(std::string_view in)
{
	std::string out;
	out.reserve(in.size());
	int depth = 0;
	for (char c : in)
	{
		if (c == '<')
		{
			depth++;
			out.push_back(c);
			continue;
		}
		if (c == '>')
		{
			depth--;
			out.push_back(c);
			continue;
		}
		if (depth > 0 && (c == ' ' || c == '\t'))
			continue;
		out.push_back(c);
	}
	return out;
}

// Extract the outer template name: "CHandle<X>" -> "CHandle". For non-templated names returns name unchanged.
std::string AtomOuterName(std::string_view typeName)
{
	auto pos = typeName.find('<');
	if (pos == std::string_view::npos)
		return std::string(typeName);
	auto name = std::string(typeName.substr(0, pos));
	while (!name.empty() && (name.back() == ' ' || name.back() == '\t'))
		name.pop_back();
	return name;
}

ojson SerializeMetadataArray(const std::vector<IntermediateMetadata>& metadataVector)
{
	ojson arr = ojson::array();
	for (const auto& metadata : metadataVector)
	{
		ojson j;
		j["name"] = metadata.name;
		if (metadata.hasValue && metadata.stringValue.has_value())
			j["value"] = *metadata.stringValue;
		arr.push_back(std::move(j));
	}
	return arr;
}

// Map a builtin name to a JSON Schema fragment. Returns std::nullopt for unrecognised builtins.
std::optional<ojson> SerializeBuiltin(std::string_view name)
{
	ojson j;
	if (name == "bool")
	{
		j["type"] = "boolean";
		return j;
	}
	if (name == "int8" || name == "int16" || name == "int32" || name == "int64" || name == "uint8" || name == "uint16" || name == "uint32" || name == "uint64")
	{
		j["type"] = "integer";
		j["format"] = std::string(name);
		return j;
	}
	if (name == "float32")
	{
		j["type"] = "number";
		j["format"] = "float";
		return j;
	}
	if (name == "float64")
	{
		j["type"] = "number";
		j["format"] = "double";
		return j;
	}
	return std::nullopt;
}

bool IsHandleAtom(std::string_view name)
{
	return name == "CHandle" || name == "CWeakHandle" || name == "CStrongHandle";
}

bool IsVectorAtom(std::string_view name)
{
	return name == "CUtlVector" || name == "CNetworkUtlVectorBase" || name == "CUtlVectorEmbeddedNetworkVar" || name == "CUtlLeanVector" || name == "CCopyableUtlVector";
}

bool IsStringAtom(std::string_view name)
{
	return name == "CUtlSymbolLarge" || name == "CUtlString" || name == "CUtlStringToken" || name == "CGlobalSymbol";
}

bool IsResourceAtom(std::string_view name)
{
	// "CResource* family" per spec — names beginning with CResource (CResourceName, CResourceNameTyped, etc.).
	return name.rfind("CResource", 0) == 0;
}

ojson MakeRef(const std::string& target)
{
	ojson j;
	j["$ref"] = "#/$defs/" + target;
	return j;
}

ojson MakeNullable(ojson inner)
{
	ojson j;
	ojson nullSchema;
	nullSchema["type"] = "null";
	j["oneOf"] = ojson::array({std::move(inner), std::move(nullSchema)});
	return j;
}

ojson MakeUnresolved()
{
	ojson j;
	j[Ext("unresolved")] = true;
	return j;
}

// Returns a type-shape fragment per §5. Field-level annotations (offset, type, metadata) are merged by the caller.
ojson SerializeType(CSchemaType* type)
{
	if (!type)
		return MakeUnresolved();

	const std::string fullName = type->m_sTypeName.String();

	switch (type->m_eTypeCategory)
	{
		case SCHEMA_TYPE_BUILTIN:
		{
			auto built = SerializeBuiltin(fullName);
			if (built.has_value())
				return std::move(*built);
			ojson j = MakeUnresolved();
			j[Ext("builtin-name")] = fullName;
			return j;
		}
		case SCHEMA_TYPE_DECLARED_CLASS:
		{
			auto* declared = static_cast<CSchemaType_DeclaredClass*>(type);
			if (declared->m_pClassInfo && declared->m_pClassInfo->m_pszName)
				return MakeRef(declared->m_pClassInfo->m_pszName);
			return MakeRef(fullName);
		}
		case SCHEMA_TYPE_DECLARED_ENUM:
		{
			auto* declared = static_cast<CSchemaType_DeclaredEnum*>(type);
			if (declared->m_pEnumInfo && declared->m_pEnumInfo->m_pszName)
				return MakeRef(declared->m_pEnumInfo->m_pszName);
			return MakeRef(fullName);
		}
		case SCHEMA_TYPE_POINTER:
		{
			auto* ptr = static_cast<CSchemaType_Ptr*>(type);
			ojson inner = SerializeType(ptr->m_pObjectType);
			ojson j = MakeNullable(std::move(inner));
			j[Ext("pointer")] = true;
			return j;
		}
		case SCHEMA_TYPE_FIXED_ARRAY:
		{
			auto* arr = static_cast<CSchemaType_FixedArray*>(type);
			ojson j;
			j["type"] = "array";
			j["items"] = SerializeType(arr->m_pElementType);
			j["minItems"] = arr->m_nElementCount;
			j["maxItems"] = arr->m_nElementCount;
			return j;
		}
		case SCHEMA_TYPE_BITFIELD:
		{
			auto* bits = static_cast<CSchemaType_Bitfield*>(type);
			ojson j;
			j["type"] = "integer";
			j[Ext("bitfield-bits")] = bits->m_nBitfieldCount;
			return j;
		}
		case SCHEMA_TYPE_ATOMIC:
		{
			const std::string atomName = AtomOuterName(fullName);

			// SCHEMA_ATOMIC_T: handles
			if (type->m_eAtomicCategory == SCHEMA_ATOMIC_T && IsHandleAtom(atomName))
			{
				auto* tmpl = static_cast<CSchemaType_Atomic_T*>(type);
				ojson inner = SerializeType(tmpl->m_pTemplateType);
				ojson j = MakeNullable(std::move(inner));
				j[Ext("handle")] = true;
				if (tmpl->m_pTemplateType)
				{
					if (tmpl->m_pTemplateType->m_eTypeCategory == SCHEMA_TYPE_DECLARED_CLASS)
					{
						auto* declared = static_cast<CSchemaType_DeclaredClass*>(tmpl->m_pTemplateType);
						if (declared->m_pClassInfo && declared->m_pClassInfo->m_pszName)
							j[Ext("handle-target")] = declared->m_pClassInfo->m_pszName;
						else
							j[Ext("handle-target")] = tmpl->m_pTemplateType->m_sTypeName.String();
					}
					else
					{
						j[Ext("handle-target")] = tmpl->m_pTemplateType->m_sTypeName.String();
					}
				}
				if (atomName == "CWeakHandle")
					j[Ext("handle-kind")] = "weak";
				else if (atomName == "CStrongHandle")
					j[Ext("handle-kind")] = "strong";
				return j;
			}

			// SCHEMA_ATOMIC_COLLECTION_OF_T: vectors
			if (type->m_eAtomicCategory == SCHEMA_ATOMIC_COLLECTION_OF_T && IsVectorAtom(atomName))
			{
				auto* tmpl = static_cast<CSchemaType_Atomic_T*>(type);
				ojson j;
				j["type"] = "array";
				j["items"] = SerializeType(tmpl->m_pTemplateType);
				return j;
			}

			// String-like atoms (SCHEMA_ATOMIC_PLAIN typically) and CResource* family.
			if (IsStringAtom(atomName) || IsResourceAtom(atomName))
			{
				ojson j;
				j["type"] = "string";
				return j;
			}

			// SCHEMA_ATOMIC_TT: two-template-arg containers (maps, pairs).
			if (type->m_eAtomicCategory == SCHEMA_ATOMIC_TT)
			{
				auto* tt = static_cast<CSchemaType_Atomic_TT*>(type);
				ojson j;
				j["type"] = "object";
				j[Ext("template-args")] = ojson::array();
				j[Ext("template-args")].push_back(tt->m_pTemplateType ? tt->m_pTemplateType->m_sTypeName.String() : "");
				j[Ext("template-args")].push_back(tt->m_pTemplateType2 ? tt->m_pTemplateType2->m_sTypeName.String() : "");
				j[Ext("unresolved-template")] = true;
				return j;
			}

			// Unhandled atom (e.g. unknown SCHEMA_ATOMIC_T name, SCHEMA_ATOMIC_I, SCHEMA_ATOMIC_PLAIN that isn't string/resource).
			ojson j = MakeUnresolved();
			j[Ext("atomic-name")] = atomName;
			return j;
		}
		default:
		{
			ojson j = MakeUnresolved();
			j[Ext("type-category")] = static_cast<int>(type->m_eTypeCategory);
			return j;
		}
	}
}

// Reorder keys per §8: type, title, description, x-* extension keys (alphabetical), allOf, properties, unevaluatedProperties.
// Any keys not in the list are appended after, in their existing order.
ojson ReorderKeys(const ojson& in)
{
	if (!in.is_object())
		return in;

	static const std::vector<std::string> leading{"type", "title", "description"};
	static const std::vector<std::string> trailing{"allOf", "properties", "unevaluatedProperties"};

	ojson out = ojson::object();

	for (const auto& key : leading)
	{
		auto it = in.find(key);
		if (it != in.end())
			out[key] = *it;
	}

	std::vector<std::string> xKeys;
	for (auto it = in.begin(); it != in.end(); ++it)
	{
		const std::string& k = it.key();
		if (k.rfind("x-", 0) == 0)
			xKeys.push_back(k);
	}
	std::sort(xKeys.begin(), xKeys.end());
	for (const auto& k : xKeys)
		out[k] = in.at(k);

	for (const auto& key : trailing)
	{
		auto it = in.find(key);
		if (it != in.end())
			out[key] = *it;
	}

	for (auto it = in.begin(); it != in.end(); ++it)
	{
		const std::string& k = it.key();
		if (out.find(k) != out.end())
			continue;
		out[k] = it.value();
	}

	return out;
}

ojson SerializeClass(const IntermediateSchemaClass& c)
{
	ojson def;
	def["type"] = "object";
	def["title"] = c.name;
	def[Ext("kind")] = "class";
	def[Ext("module")] = c.module;
	def[Ext("size")] = c.size;

	auto classMetadata = SerializeMetadataArray(c.metadata);
	if (!classMetadata.empty())
		def[Ext("metadata")] = std::move(classMetadata);

	if (!c.parents.empty())
	{
		ojson allOf = ojson::array();
		for (const auto& parent : c.parents)
			allOf.push_back(MakeRef(parent.name));
		def["allOf"] = std::move(allOf);
	}

	if (!c.fields.empty())
	{
		ojson properties = ojson::object();
		for (const auto& field : c.fields)
		{
			ojson fieldSchema = SerializeType(field.type);
			fieldSchema[Ext("offset")] = field.offset;
			if (field.type)
				fieldSchema[Ext("type")] = NormaliseTypeString(field.type->m_sTypeName.String());
			auto fieldMetadata = SerializeMetadataArray(field.metadata);
			if (!fieldMetadata.empty())
				fieldSchema[Ext("metadata")] = std::move(fieldMetadata);
			properties[field.name] = ReorderKeys(fieldSchema);
		}
		def["properties"] = std::move(properties);
	}

	def["unevaluatedProperties"] = false;

	return ReorderKeys(def);
}

ojson SerializeEnum(const IntermediateSchemaEnum& e)
{
	ojson def;
	def["type"] = "integer";
	def["title"] = e.name;
	def[Ext("kind")] = "enum";
	def[Ext("module")] = e.module;
	if (e.stringAlignment.has_value())
		def[Ext("alignment")] = *e.stringAlignment;

	if (e.stringAlignment.has_value())
	{
		const std::string& align = *e.stringAlignment;
		std::optional<std::string> format;
		if (align == "uint8_t")
			format = "uint8";
		else if (align == "uint16_t")
			format = "uint16";
		else if (align == "uint32_t")
			format = "uint32";
		else if (align == "uint64_t")
			format = "uint64";
		if (format.has_value())
			def["format"] = *format;
	}

	if (!e.members.empty())
	{
		std::vector<int64_t> values;
		std::unordered_set<int64_t> seen;
		ojson valuesMap = ojson::object();
		ojson memberMetadata = ojson::object();
		bool anyMemberMetadata = false;

		for (const auto& m : e.members)
		{
			if (seen.insert(m.value).second)
				values.push_back(m.value);
			valuesMap[m.name] = m.value;

			auto mm = SerializeMetadataArray(m.metadata);
			if (!mm.empty())
			{
				memberMetadata[m.name] = std::move(mm);
				anyMemberMetadata = true;
			}
		}
		std::sort(values.begin(), values.end());

		ojson enumValues = ojson::array();
		for (auto v : values)
			enumValues.push_back(v);
		def["enum"] = std::move(enumValues);
		def[Ext("enum-values")] = std::move(valuesMap);

		if (anyMemberMetadata)
			def[Ext("enum-member-metadata")] = std::move(memberMetadata);
	}

	auto enumMetadata = SerializeMetadataArray(e.metadata);
	if (!enumMetadata.empty())
		def[Ext("metadata")] = std::move(enumMetadata);

	return ReorderKeys(def);
}

ojson BuildSyntheticDefs()
{
	// Verbatim from spec appendix A. `required` is preserved as written; do not propagate.
	const char* kSyntheticJson = R"JSON({
  "Vector": {
    "type": "object",
    "title": "Vector",
    "description": "3D vector.",
    "properties": {
      "x": { "type": "number", "format": "float" },
      "y": { "type": "number", "format": "float" },
      "z": { "type": "number", "format": "float" }
    },
    "required": ["x", "y", "z"],
    "x-cs2-synthetic": true
  },
  "VectorAligned": {
    "type": "object",
    "title": "VectorAligned",
    "description": "16-byte-aligned 3D vector (memory layout: Vector + padding).",
    "properties": {
      "x": { "type": "number", "format": "float" },
      "y": { "type": "number", "format": "float" },
      "z": { "type": "number", "format": "float" }
    },
    "required": ["x", "y", "z"],
    "x-cs2-synthetic": true
  },
  "Vector2D": {
    "type": "object",
    "title": "Vector2D",
    "description": "2D vector.",
    "properties": {
      "x": { "type": "number", "format": "float" },
      "y": { "type": "number", "format": "float" }
    },
    "required": ["x", "y"],
    "x-cs2-synthetic": true
  },
  "Vector4D": {
    "type": "object",
    "title": "Vector4D",
    "description": "4D vector.",
    "properties": {
      "x": { "type": "number", "format": "float" },
      "y": { "type": "number", "format": "float" },
      "z": { "type": "number", "format": "float" },
      "w": { "type": "number", "format": "float" }
    },
    "required": ["x", "y", "z", "w"],
    "x-cs2-synthetic": true
  },
  "QAngle": {
    "type": "object",
    "title": "QAngle",
    "description": "Euler angles (pitch, yaw, roll), in degrees.",
    "properties": {
      "pitch": { "type": "number", "format": "float" },
      "yaw":   { "type": "number", "format": "float" },
      "roll":  { "type": "number", "format": "float" }
    },
    "required": ["pitch", "yaw", "roll"],
    "x-cs2-synthetic": true
  },
  "Quaternion": {
    "type": "object",
    "title": "Quaternion",
    "description": "Unit quaternion.",
    "properties": {
      "x": { "type": "number", "format": "float" },
      "y": { "type": "number", "format": "float" },
      "z": { "type": "number", "format": "float" },
      "w": { "type": "number", "format": "float" }
    },
    "required": ["x", "y", "z", "w"],
    "x-cs2-synthetic": true
  },
  "Color": {
    "type": "object",
    "title": "Color",
    "description": "RGBA color, 8 bits per channel.",
    "properties": {
      "r": { "type": "integer", "format": "uint8" },
      "g": { "type": "integer", "format": "uint8" },
      "b": { "type": "integer", "format": "uint8" },
      "a": { "type": "integer", "format": "uint8" }
    },
    "required": ["r", "g", "b", "a"],
    "x-cs2-synthetic": true
  },
  "Color32": {
    "type": "object",
    "title": "Color32",
    "description": "Packed 32-bit RGBA color.",
    "properties": {
      "r": { "type": "integer", "format": "uint8" },
      "g": { "type": "integer", "format": "uint8" },
      "b": { "type": "integer", "format": "uint8" },
      "a": { "type": "integer", "format": "uint8" }
    },
    "required": ["r", "g", "b", "a"],
    "x-cs2-synthetic": true
  },
  "CTransform": {
    "type": "object",
    "title": "CTransform",
    "description": "Position + rotation transform.",
    "properties": {
      "position": { "$ref": "#/$defs/VectorAligned" },
      "rotation": { "$ref": "#/$defs/Quaternion" }
    },
    "required": ["position", "rotation"],
    "x-cs2-synthetic": true
  },
  "matrix3x4_t": {
    "type": "array",
    "title": "matrix3x4_t",
    "description": "3x4 transform matrix (row-major, 12 floats).",
    "items": { "type": "number", "format": "float" },
    "minItems": 12,
    "maxItems": 12,
    "x-cs2-synthetic": true
  },
  "matrix3x4a_t": {
    "type": "array",
    "title": "matrix3x4a_t",
    "description": "16-byte-aligned 3x4 transform matrix (row-major, 12 floats).",
    "items": { "type": "number", "format": "float" },
    "minItems": 12,
    "maxItems": 12,
    "x-cs2-synthetic": true
  },
  "matrix4x4_t": {
    "type": "array",
    "title": "matrix4x4_t",
    "description": "4x4 transform matrix (row-major, 16 floats).",
    "items": { "type": "number", "format": "float" },
    "minItems": 16,
    "maxItems": 16,
    "x-cs2-synthetic": true
  },
  "AABB_t": {
    "type": "object",
    "title": "AABB_t",
    "description": "Axis-aligned bounding box.",
    "properties": {
      "mins": { "$ref": "#/$defs/Vector" },
      "maxs": { "$ref": "#/$defs/Vector" }
    },
    "required": ["mins", "maxs"],
    "x-cs2-synthetic": true
  }
})JSON";

	std::string syntheticJson = kSyntheticJson;
	const std::string from = "x-cs2-";
	const std::string to = ExtPrefix();
	if (from != to)
	{
		size_t pos = 0;
		while ((pos = syntheticJson.find(from, pos)) != std::string::npos)
		{
			syntheticJson.replace(pos, from.size(), to);
			pos += to.size();
		}
	}
	return ojson::parse(syntheticJson);
}

// Walk every `$ref` in the document and confirm it resolves to a key in `$defs`.
// Only checks our own emitted refs (fragment of form "#/$defs/<name>").
// Returns the count of dangling refs found; zero means all good.
int CheckRefs(const ojson& node, const ojson& defs, std::unordered_set<std::string>& reportedMissing)
{
	int dangling = 0;
	if (node.is_object())
	{
		for (auto it = node.begin(); it != node.end(); ++it)
		{
			if (it.key() == "$ref" && it.value().is_string())
			{
				const std::string ref = it.value().get<std::string>();
				const std::string prefix = "#/$defs/";
				if (ref.rfind(prefix, 0) == 0)
				{
					const std::string target = ref.substr(prefix.size());
					if (defs.find(target) == defs.end())
					{
						if (reportedMissing.insert(target).second)
							spdlog::warn("schemas_jsonschema.json: dangling $ref to '{}'", target);
						dangling++;
					}
				}
			}
			else
			{
				dangling += CheckRefs(it.value(), defs, reportedMissing);
			}
		}
	}
	else if (node.is_array())
	{
		for (const auto& el : node)
			dangling += CheckRefs(el, defs, reportedMissing);
	}
	return dangling;
}

} // anonymous namespace

void Dump(const std::vector<IntermediateSchemaEnum>& enums, const std::vector<IntermediateSchemaClass>& classes)
{
	spdlog::info("Dumping schemas to json schema");

	ojson root;
	root["$schema"] = "https://json-schema.org/draft/2020-12/schema";
	root["$id"] = std::string("https://valveresourceformat.github.io/dumpsource2/") + kGameToken + "_schema.json";
	root["title"] = std::string(kGameToken) + " Schema (DumpSource2 reflection)";
	root["description"] = std::string("JSON Schema describing every entity reflected by DumpSource2 from ") + kGameToken + ". Inheritance is preserved via allOf+$ref. Game-specific information is preserved under " + ExtPrefix() + "* extension keywords.";

	root[Ext("generator")] = "https://github.com/ValveResourceFormat/DumpSource2";
	if (!Globals::sourceRevision.empty())
	{
		try
		{
			root[Ext("revision")] = std::stoi(Globals::sourceRevision);
		}
		catch (...)
		{
			root[Ext("revision")] = Globals::sourceRevision;
		}
	}
	if (!Globals::versionDate.empty())
		root[Ext("version-date")] = Globals::versionDate;
	if (!Globals::versionTime.empty())
		root[Ext("version-time")] = Globals::versionTime;

	// $defs starts with synthetic primitives; reflected entities can overwrite.
	ojson defs = BuildSyntheticDefs();

	for (const auto& c : classes)
		defs[c.name] = SerializeClass(c);

	for (const auto& e : enums)
		defs[e.name] = SerializeEnum(e);

	root["$defs"] = std::move(defs);

	std::unordered_set<std::string> reportedMissing;
	const int dangling = CheckRefs(root, root["$defs"], reportedMissing);
	if (dangling > 0)
	{
		spdlog::error("schemas_jsonschema.json self-validation failed: {} dangling $ref(s) (across {} unique target(s))", dangling, reportedMissing.size());
	}

	const auto outPath = Globals::outputPath / "schemas_jsonschema.json";
	std::ofstream output(outPath);
	output << root.dump(2) << "\n";
	output.close();

	spdlog::info("Wrote {} ({} classes, {} enums, {} $defs entries)", outPath.generic_string(), classes.size(), enums.size(), root["$defs"].size());
}

} // namespace Dumpers::Schemas::JsonSchemaExporter
