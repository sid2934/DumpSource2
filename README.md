# DumpSource2

A C++ Application that offline dumps schema bindings and convars/commands for [GameTracking](https://github.com/SteamDatabase/GameTracking) purposes.

[See this file in GameTracking on how its used.](https://github.com/SteamDatabase/GameTracking/blob/master/tools/dump_source2.sh)

## Usage

Run DumpSource2 from the rootbin folder of a source 2 game. `game/bin/win64`

`DumpSource2 <output path>`

- `output path` - absolute or relative path to a folder where output should be stored

## Output

- `schemas/` - per-module `.h` text dump of every reflected class and enum
- `schemas.json` - structured dump of every reflected class and enum
- `schemas_jsonschema.json` - JSON Schema 2020-12 description of every reflected entity, suitable for code generators (NJsonSchema, quicktype, json-schema-to-typescript, ...)
- `convars.txt` - dump of registered convars and concommands
- `module_metadata.json` - per-module metadata (build strings, etc.)
- `.stringsignore` - list of names emitted by the dumpers, used by GameTracking to skip diff noise


# Compilation

## Windows

```sh
mkdir build
cd build
cmake ..
# Open VS solution and compile from VS
```

## Linux
```sh
mkdir build
cd build
cmake ..
make
```

## SDK Configuration

Each game target is built with `build_game(GAME, GAME_PATH, SDK)` in [`src/main/CMakeLists.txt`](./src/main/CMakeLists.txt). The third argument selects which HL2SDK from `vendor/` to link against. Games update more often than the SDKs, so it may be necessary to switch to another game's SDK.
