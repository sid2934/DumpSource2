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
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

// HL2SDK math headers used for sizeof / offsetof on the synthetic catalogue.
// These types aren't reflected by CSchemaSystem (so we have to inject them
// into $defs ourselves), but their C++ definitions ARE reachable from here
// because we already link against the HL2SDK — so we can pull layout numbers
// from the actual ABI rather than hardcoding them.
#include "mathlib/vector.h"
#include "mathlib/vector2d.h"
#include "mathlib/vector4d.h"
#include "mathlib/mathlib.h"
#include "mathlib/transform.h"
#include "mathlib/camera.h"
#include "Color.h"

using ojson = nlohmann::ordered_json;

namespace Dumpers::Schemas::JsonSchemaExporter
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

// Per-game JSON Schema extension key prefix (x-cs2-*, x-dota2-*, x-deadlock-*).
// Other exporters don't emit JSON Schema extension vocabulary, so they don't need
// a helper like this; we do, so it lives here.
inline std::string ExtPrefix()
{
	return std::string("x-") + kGameToken + "-";
}

inline std::string Ext(const char* suffix)
{
	return ExtPrefix() + suffix;
}

// Counts of atom / builtin names we couldn't match to a known dispatch case,
// reported as a warning summary at the end of Dump(). Mirrors the
// g_unknownMetadataCounts pattern in filesystem_exporter.cpp.
static std::map<std::string, int> g_unknownAtomNames;
static std::map<std::string, int> g_unknownBuiltinNames;

// Strip whitespace inside angle brackets so type-name strings produced by the
// SDK round-trip into a canonical form. Whitespace outside <...> is preserved.
//
//   "CHandle< X >"             -> "CHandle<X>"
//   "CFoo< CBar< int > >"      -> "CFoo<CBar<int>>"     (nested templates)
//   "unsigned int"             -> "unsigned int"        (no <>; preserved)
//   "CFoo<X>>"                 -> "CFoo<X>>"            (malformed; passed through)
//
// Only U+0020 SPACE and U+0009 TAB are stripped — newlines/CRs are left as-is
// because we don't expect them in SDK type names and stripping them silently
// would mask actual corruption.
std::string NormalizeTypeString(std::string_view in)
{
	std::string out;
	out.reserve(in.size());
	int depth = 0;
	for (char c : in)
	{
		const bool insideTemplate = depth > 0;
		const bool stripChar = insideTemplate && (c == ' ' || c == '\t');
		if (stripChar)
			continue;

		if (c == '<')
			++depth;
		else if (c == '>' && depth > 0)
			--depth;

		out.push_back(c);
	}
	return out;
}

// "CHandle<X>" -> "CHandle"; non-templated names returned unchanged.
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

// Atomic-name dispatch sets. Category alone is sufficient for collections (every
// SCHEMA_ATOMIC_COLLECTION_OF_T is a JSON array) and for two-arg containers
// (SCHEMA_ATOMIC_TT). The remaining sets exist only where category + structural
// fields are not enough to know how to map the atom into JSON Schema vocabulary.
//
// Adding a new entry here is a last resort — first prefer a category-based or
// structural discriminator. Each entry is annotated with WHY it's included so a
// future maintainer can judge whether a new Valve type belongs in the set.
static const std::unordered_set<std::string_view> kHandleAtomNames = {
	"CHandle",       // entity handle — generation-counted ref to networked entity
	"CWeakHandle",   // weak resource handle, nullable by definition
	"CStrongHandle", // strong resource handle, also nullable
};

static const std::unordered_set<std::string_view> kStringAtomNames = {
	"CUtlSymbolLarge", // symbol-table interned string
	"CUtlString",      // owning string
	"CUtlStringToken", // hashed string; serializes as plain string at the schema layer
	"CGlobalSymbol",   // global symbol; string-shaped
};

// Names that arrive from CSchemaSystem as SCHEMA_ATOMIC_PLAIN but should
// resolve to entries in the synthetic compound type catalogue (BuildSyntheticDefs).
// CS2 reflection treats math types as opaque atomics rather than declared
// classes — without this set, every field of type Vector / QAngle / etc. would
// fall through to the unresolved path and codegens would never see the synthetic
// $defs we emit.
//
// MUST stay aligned with the keys produced by BuildSyntheticDefs(). If you add
// or remove an entry there, mirror it here (and vice versa).
//
// Single list across all game targets: an entry that appears here but isn't
// actually referenced by a particular game's reflection output is just an
// unused $defs entry in the artifact — harmless bytes. The opposite mistake
// (a name that's referenced but missing from the set) silently produces
// broken output via fallthrough to unresolved. So we err toward inclusion
// rather than per-game guards.
static const std::unordered_set<std::string_view> kSyntheticAtomNames = {
	"Vector",
	"VectorAligned",
	"Vector2D",
	"Vector4D",
	"VectorWS",          // CS2-evidenced (59 refs in cs2.json); "Vector World Space"; not in HL2SDK
	"QAngle",
	"Quaternion",
	"QuaternionStorage", // CS2-evidenced (8 refs in cs2.json); storage form of a quaternion; not in HL2SDK
	"Color",
	"CTransform",
	"AABB_t",            // also a declared_class in mathlib_extended; the reflected
	                     // entry overwrites the synthetic, all refs land on the same $def
	"matrix3x4_t",
	"matrix3x4a_t",
};

bool IsHandleAtom(std::string_view name)
{
	return kHandleAtomNames.find(name) != kHandleAtomNames.end();
}

bool IsStringAtom(std::string_view name)
{
	return kStringAtomNames.find(name) != kStringAtomNames.end();
}

bool IsResourceAtom(std::string_view name)
{
	// CResource* family — a small zoo of CResourceName, CResourceNameTyped<>,
	// CResourceArray<>, etc.; all serialize as a string asset path.
	return name.rfind("CResource", 0) == 0;
}

bool IsSyntheticAtom(std::string_view name)
{
	return kSyntheticAtomNames.find(name) != kSyntheticAtomNames.end();
}

ojson SerializeBuiltin(std::string_view name)
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
	// Unknown builtin: emit a permissive schema (no `type` keyword) plus
	// diagnostic extension keys. JSON Schema treats a missing `type` as
	// "matches any value", so codegens degrade gracefully rather than
	// breaking. Tracked for end-of-dump reporting.
	g_unknownBuiltinNames[std::string(name)]++;
	j[Ext("unresolved")] = true;
	j[Ext("builtin-name")] = std::string(name);
	return j;
}

ojson SerializeRef(const std::string& target)
{
	ojson j;
	j["$ref"] = "#/$defs/" + target;
	return j;
}

ojson SerializeNullable(ojson inner)
{
	ojson j;
	ojson nullSchema;
	nullSchema["type"] = "null";
	j["oneOf"] = ojson::array({std::move(inner), std::move(nullSchema)});
	return j;
}

ojson SerializeUnresolved()
{
	ojson j;
	j[Ext("unresolved")] = true;
	return j;
}

// Returns a JSON Schema fragment describing the given CSchemaType.
//
// Dispatch priority is structural — category first, then category-specific
// fields — and falls back to a small set of well-known atom names ONLY when
// the schema-system's category alone can't tell us how to map the atom into
// JSON Schema vocabulary (e.g. SCHEMA_ATOMIC_T includes both nullable handles
// and other single-arg wrappers, so a name discriminator is required).
ojson SerializeType(CSchemaType* type)
{
	if (!type)
		return SerializeUnresolved();

	const std::string fullName = type->m_sTypeName.String();

	switch (type->m_eTypeCategory)
	{
		case SCHEMA_TYPE_BUILTIN:
			return SerializeBuiltin(fullName);

		case SCHEMA_TYPE_DECLARED_CLASS:
		{
			auto* declared = static_cast<CSchemaType_DeclaredClass*>(type);
			if (declared->m_pClassInfo && declared->m_pClassInfo->m_pszName)
				return SerializeRef(declared->m_pClassInfo->m_pszName);
			return SerializeRef(fullName);
		}
		case SCHEMA_TYPE_DECLARED_ENUM:
		{
			auto* declared = static_cast<CSchemaType_DeclaredEnum*>(type);
			if (declared->m_pEnumInfo && declared->m_pEnumInfo->m_pszName)
				return SerializeRef(declared->m_pEnumInfo->m_pszName);
			return SerializeRef(fullName);
		}
		case SCHEMA_TYPE_POINTER:
		{
			auto* ptr = static_cast<CSchemaType_Ptr*>(type);
			ojson j = SerializeNullable(SerializeType(ptr->m_pObjectType));
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
			// Every COLLECTION_OF_T is a JSON array — category is sufficient.
			if (type->m_eAtomicCategory == SCHEMA_ATOMIC_COLLECTION_OF_T)
			{
				auto* tmpl = static_cast<CSchemaType_Atomic_T*>(type);
				ojson j;
				j["type"] = "array";
				j["items"] = SerializeType(tmpl->m_pTemplateType);
				return j;
			}

			// Two-template-arg containers (maps, pairs) — category is sufficient.
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

			// Single-arg atoms: SCHEMA_ATOMIC_T includes both handles and other
			// wrappers, so we need a name discriminator. Plain atoms (CUtlString
			// etc.) live in SCHEMA_ATOMIC_PLAIN and also need name discrimination.
			const std::string atomName = AtomOuterName(fullName);

			if (type->m_eAtomicCategory == SCHEMA_ATOMIC_T && IsHandleAtom(atomName))
			{
				auto* tmpl = static_cast<CSchemaType_Atomic_T*>(type);
				ojson j = SerializeNullable(SerializeType(tmpl->m_pTemplateType));
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

			// Synthetic compound types (Vector, QAngle, matrix3x4_t, ...) are
			// reflected as SCHEMA_ATOMIC_PLAIN — emit a $ref to their entry in
			// the synthetic catalogue rather than falling through to unresolved.
			if (IsSyntheticAtom(atomName))
				return SerializeRef(atomName);

			if (IsStringAtom(atomName) || IsResourceAtom(atomName))
			{
				ojson j;
				j["type"] = "string";
				return j;
			}

			// Nothing matched — track the name for the end-of-dump summary.
			g_unknownAtomNames[atomName]++;
			ojson j = SerializeUnresolved();
			j[Ext("atomic-name")] = atomName;
			return j;
		}
		default:
		{
			ojson j = SerializeUnresolved();
			j[Ext("type-category")] = static_cast<int>(type->m_eTypeCategory);
			return j;
		}
	}
}

// --- synthetic compound type catalogue ---
//
// Source 2's schema reflection does NOT register basic math types
// (Vector, QAngle, matrix3x4_t, etc.). Fields reference them but they
// never appear in any CSchemaSystemTypeScope, so $refs to them would
// dangle. We inject portable JSON Schema definitions for the common
// ones up front; reflected classes of the same name overwrite the
// synthetic entry.
//
// Layout numbers (size + per-field offset) are pulled from the HL2SDK
// via sizeof / offsetof rather than hardcoded — this keeps them
// ABI-correct for the build target. Field NAMES still come from the
// schema-author's hand (true compile-time member-name reflection isn't
// available until C++26), and where the SDK member name differs from
// the schema name (e.g. QAngle exposes x/y/z which we surface as
// pitch/yaw/roll), the SYNTHETIC_FIELD_AS macro lets us rename for
// the JSON output while still pulling offsetof from the real member.

ojson SyntheticFloat()
{
	ojson j;
	j["type"] = "number";
	j["format"] = "float";
	return j;
}

ojson SyntheticUint8()
{
	ojson j;
	j["type"] = "integer";
	j["format"] = "uint8";
	return j;
}

struct SyntheticField
{
	const char* name;
	std::size_t offset;
	ojson schema;
};

template <typename T>
ojson SyntheticObject(const char* title, const char* description, std::initializer_list<SyntheticField> fields)
{
	// offsetof on non-standard-layout types is conditionally-supported per the
	// C++ standard but reliably works on GCC/Clang/MSVC via __builtin_offsetof.
	// Some HL2SDK math types (VectorAligned, CTransform) aren't standard-layout
	// because they add data members on top of inherited ones — we accept that
	// rather than error out, since the offsets we get are correct in practice.
	ojson def;
	def["type"] = "object";
	def["title"] = title;
	def["description"] = description;
	def[Ext("size")] = sizeof(T);

	ojson props = ojson::object();
	ojson required = ojson::array();
	for (const auto& f : fields)
	{
		ojson schema = f.schema;
		schema[Ext("offset")] = f.offset;
		props[f.name] = std::move(schema);
		required.push_back(f.name);
	}
	def["properties"] = std::move(props);
	def["required"] = std::move(required);
	def[Ext("synthetic")] = true;
	return def;
}

template <typename T>
ojson SyntheticFloatArray(const char* title, const char* description, int count)
{
	ojson def;
	def["type"] = "array";
	def["title"] = title;
	def["description"] = description;
	def["items"] = SyntheticFloat();
	def["minItems"] = count;
	def["maxItems"] = count;
	def[Ext("size")] = sizeof(T);
	def[Ext("synthetic")] = true;
	return def;
}

// Layout-free fallbacks for synthetics whose SDK type isn't reachable
// (Color32 and matrix4x4_t are not declared by those names in HL2SDK).
ojson SyntheticObjectHardcoded(const char* title, const char* description, std::initializer_list<std::pair<const char*, ojson>> fields)
{
	ojson def;
	def["type"] = "object";
	def["title"] = title;
	def["description"] = description;
	ojson props = ojson::object();
	ojson required = ojson::array();
	for (const auto& [name, schema] : fields)
	{
		props[name] = schema;
		required.push_back(name);
	}
	def["properties"] = std::move(props);
	def["required"] = std::move(required);
	def[Ext("synthetic")] = true;
	return def;
}

#define SYNTHETIC_FIELD(StructT, member, schema) \
	SyntheticField { #member, offsetof(StructT, member), schema }

// Same as SYNTHETIC_FIELD but emits the schema property under a friendlier
// name. Used where the SDK member identifier differs from the documented
// JSON shape (QAngle's x/y/z surfaced as pitch/yaw/roll, etc.).
#define SYNTHETIC_FIELD_AS(StructT, member, jsonName, schema) \
	SyntheticField { jsonName, offsetof(StructT, member), schema }

// VectorAligned and CTransform aren't standard-layout (each adds data members
// on top of an inheriting base), so offsetof emits -Winvalid-offsetof on
// GCC/Clang. Suppress for the synthetic catalogue specifically — the offsets
// it returns are correct in practice, which is why it's compiler-supported.
#if defined(__clang__) || defined(__GNUC__)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

ojson BuildSyntheticDefs()
{
	ojson defs = ojson::object();

	defs["Vector"] = SyntheticObject<::Vector>("Vector", "3D vector.", {
		SYNTHETIC_FIELD(::Vector, x, SyntheticFloat()),
		SYNTHETIC_FIELD(::Vector, y, SyntheticFloat()),
		SYNTHETIC_FIELD(::Vector, z, SyntheticFloat()),
	});
	// VectorAligned intentionally exposes only x/y/z. The SDK class also has
	// a fourth `w` slot the engine uses opportunistically as scratch space,
	// but it isn't part of the documented 3D-vector contract — treat as a
	// Vector with alignment + padding. (Vector4DAligned is the proper 4D
	// aligned variant.)
	defs["VectorAligned"] = SyntheticObject<::VectorAligned>("VectorAligned", "16-byte-aligned 3D vector. Schema exposes only x/y/z; the SDK class includes a hidden 4th slot used opportunistically by the engine.", {
		SYNTHETIC_FIELD(::VectorAligned, x, SyntheticFloat()),
		SYNTHETIC_FIELD(::VectorAligned, y, SyntheticFloat()),
		SYNTHETIC_FIELD(::VectorAligned, z, SyntheticFloat()),
	});
	defs["Vector2D"] = SyntheticObject<::Vector2D>("Vector2D", "2D vector.", {
		SYNTHETIC_FIELD(::Vector2D, x, SyntheticFloat()),
		SYNTHETIC_FIELD(::Vector2D, y, SyntheticFloat()),
	});
	defs["Vector4D"] = SyntheticObject<::Vector4D>("Vector4D", "4D vector.", {
		SYNTHETIC_FIELD(::Vector4D, x, SyntheticFloat()),
		SYNTHETIC_FIELD(::Vector4D, y, SyntheticFloat()),
		SYNTHETIC_FIELD(::Vector4D, z, SyntheticFloat()),
		SYNTHETIC_FIELD(::Vector4D, w, SyntheticFloat()),
	});

	// VectorWS ("Vector World Space") is referenced in CS2 schemas but not
	// declared in HL2SDK headers — emit a layout-free synthetic so $refs
	// resolve. Shape assumed to mirror Vector (3 floats) based on naming
	// convention; size/offset are not asserted.
	defs["VectorWS"] = SyntheticObjectHardcoded("VectorWS", "World-space 3D vector. Layout-free synthetic; not declared by this name in HL2SDK.", {
		{"x", SyntheticFloat()},
		{"y", SyntheticFloat()},
		{"z", SyntheticFloat()},
	});
	defs["QAngle"] = SyntheticObject<::QAngle>("QAngle", "Euler angles, in degrees. SDK members x/y/z surfaced as pitch/yaw/roll.", {
		SYNTHETIC_FIELD_AS(::QAngle, x, "pitch", SyntheticFloat()),
		SYNTHETIC_FIELD_AS(::QAngle, y, "yaw", SyntheticFloat()),
		SYNTHETIC_FIELD_AS(::QAngle, z, "roll", SyntheticFloat()),
	});
	defs["Quaternion"] = SyntheticObject<::Quaternion>("Quaternion", "Unit quaternion.", {
		SYNTHETIC_FIELD(::Quaternion, x, SyntheticFloat()),
		SYNTHETIC_FIELD(::Quaternion, y, SyntheticFloat()),
		SYNTHETIC_FIELD(::Quaternion, z, SyntheticFloat()),
		SYNTHETIC_FIELD(::Quaternion, w, SyntheticFloat()),
	});

	// QuaternionStorage is referenced in CS2 schemas but not declared in
	// HL2SDK headers — layout-free synthetic. Shape assumed to mirror
	// Quaternion (4 floats) based on naming convention.
	defs["QuaternionStorage"] = SyntheticObjectHardcoded("QuaternionStorage", "Quaternion in storage form. Layout-free synthetic; not declared by this name in HL2SDK.", {
		{"x", SyntheticFloat()},
		{"y", SyntheticFloat()},
		{"z", SyntheticFloat()},
		{"w", SyntheticFloat()},
	});

	// Color stores 4 channels as `unsigned char _color[4]` (private member),
	// so we can't take offsetof on _color from outside. The bytes are at
	// offsets 0..3 of the class itself since _color is the only data member.
	{
		ojson def;
		def["type"] = "object";
		def["title"] = "Color";
		def["description"] = "RGBA color, 8 bits per channel.";
		def[Ext("size")] = sizeof(::Color);
		ojson props = ojson::object();
		ojson required = ojson::array();
		const char* names[] = {"r", "g", "b", "a"};
		for (std::size_t i = 0; i < 4; ++i)
		{
			ojson f = SyntheticUint8();
			f[Ext("offset")] = i;
			props[names[i]] = std::move(f);
			required.push_back(names[i]);
		}
		def["properties"] = std::move(props);
		def["required"] = std::move(required);
		def[Ext("synthetic")] = true;
		defs["Color"] = std::move(def);
	}

	defs["CTransform"] = SyntheticObject<::CTransform>("CTransform", "Position + rotation transform. SDK members m_vPosition/m_orientation surfaced as position/rotation.", {
		SYNTHETIC_FIELD_AS(::CTransform, m_vPosition, "position", SerializeRef("VectorAligned")),
		SYNTHETIC_FIELD_AS(::CTransform, m_orientation, "rotation", SerializeRef("Quaternion")),
	});
	defs["AABB_t"] = SyntheticObject<::AABB_t>("AABB_t", "Axis-aligned bounding box. SDK members m_vMinBounds/m_vMaxBounds surfaced as mins/maxs.", {
		SYNTHETIC_FIELD_AS(::AABB_t, m_vMinBounds, "mins", SerializeRef("Vector")),
		SYNTHETIC_FIELD_AS(::AABB_t, m_vMaxBounds, "maxs", SerializeRef("Vector")),
	});

	defs["matrix3x4_t"] = SyntheticFloatArray<::matrix3x4_t>("matrix3x4_t", "3x4 transform matrix (row-major, 12 floats).", 12);
	defs["matrix3x4a_t"] = SyntheticFloatArray<::matrix3x4a_t>("matrix3x4a_t", "16-byte-aligned 3x4 transform matrix (row-major, 12 floats).", 12);

	return defs;
}

#if defined(__clang__) || defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif

#undef SYNTHETIC_FIELD
#undef SYNTHETIC_FIELD_AS

ojson SerializeClass(const IntermediateSchemaClass& c)
{
	// Insertion order is the publish order; ordered_json preserves it. No
	// post-hoc reorder pass needed.
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
			allOf.push_back(SerializeRef(parent.name));
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
				fieldSchema[Ext("type")] = NormalizeTypeString(field.type->m_sTypeName.String());
			auto fieldMetadata = SerializeMetadataArray(field.metadata);
			if (!fieldMetadata.empty())
				fieldSchema[Ext("metadata")] = std::move(fieldMetadata);
			properties[field.name] = std::move(fieldSchema);
		}
		def["properties"] = std::move(properties);
	}

	def["unevaluatedProperties"] = false;
	return def;
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
		if (align == "uint8_t")
			def["format"] = "uint8";
		else if (align == "uint16_t")
			def["format"] = "uint16";
		else if (align == "uint32_t")
			def["format"] = "uint32";
		else if (align == "uint64_t")
			def["format"] = "uint64";
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

	return def;
}

// --- internal $ref-resolution self-check ---
//
// REVIEWER NOTE — this block is for context during review and will be removed before merging.
//
//   PROS
//   - Catches a real bug class: a future change that introduces a new $ref
//     without a matching $defs entry would silently produce broken output;
//     this self-check warns at dump time.
//   - Cheap (single pass; ~ms on a full CS2 dump).
//   - Adds no new dependency.
//
//   CONS
//   - Other exporters don't have an in-process validation step; this is
//     unique scope creep relative to filesystem_exporter / json_exporter.
//   - Soft check (logs only); a CI-time external validator (ajv-cli /
//     check-jsonschema) is necessary for actual correctness gating
//     regardless.
//   - ~35 LOC of unique-pattern code adds review surface.
//
// Plan: leave this in during the review cycle to flush out bugs, then
// REMOVE the function and its call in Dump() before this is marked
// ready-for-merge.
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

void Dump(const std::vector<IntermediateSchemaEnum>& enums, const std::vector<IntermediateSchemaClass>& classes)
{
	spdlog::info("Dumping schemas to json schema");

	ojson root;
	root["$schema"] = "https://json-schema.org/draft/2020-12/schema";
	root["$id"] = std::string("https://valveresourceformat.github.io/dumpsource2/") + kGameToken + "_schema.json";
	root["title"] = std::string(kGameToken) + " Schema (DumpSource2 reflection)";
	root["description"] = std::string("JSON Schema describing every entity reflected by DumpSource2 from ") + kGameToken + ". Inheritance is preserved via allOf+$ref. Game-specific information is preserved under " + ExtPrefix() + "* extension keywords.";

	root[Ext("generator")] = "https://github.com/ValveResourceFormat/DumpSource2";

	// std::stoi can throw on a malformed revision string. json_exporter.cpp
	// invokes it bare; we wrap it here so a malformed steam.inf can't take
	// this exporter down independently of the existing one. Happy to drop
	// the try/catch for consistency with json_exporter if reviewers prefer.
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

	// Synthetic primitives go in first; reflected entities of the same name overwrite.
	ojson defs = BuildSyntheticDefs();
	for (const auto& c : classes)
		defs[c.name] = SerializeClass(c);
	for (const auto& e : enums)
		defs[e.name] = SerializeEnum(e);
	root["$defs"] = std::move(defs);

	// Self-check (provisional, see CheckRefs comment above).
	std::unordered_set<std::string> reportedMissing;
	const int dangling = CheckRefs(root, root["$defs"], reportedMissing);
	if (dangling > 0)
		spdlog::error("schemas_jsonschema.json self-validation: {} dangling $ref(s) (across {} unique target(s))", dangling, reportedMissing.size());

	std::ofstream output(Globals::outputPath / "schemas_jsonschema.json");
	output << root.dump(2) << "\n";
	output.close();

	for (const auto& [name, count] : g_unknownAtomNames)
		spdlog::warn("Atomic '{}' fell through to unresolved ({} usages)", name, count);
	for (const auto& [name, count] : g_unknownBuiltinNames)
		spdlog::warn("Builtin '{}' fell through to permissive schema ({} usages)", name, count);

	spdlog::info("Wrote schemas_jsonschema.json ({} classes, {} enums)", classes.size(), enums.size());
}

} // namespace Dumpers::Schemas::JsonSchemaExporter
