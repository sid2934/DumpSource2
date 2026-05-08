# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Project Does

DumpSource2 is a C++ CLI tool that loads Source 2 game modules (DLLs on Windows, SOs on Linux) at runtime and dumps schema bindings, console variables, and console commands to disk. It must be run from inside a game's `bin/` directory. Outputs go to a specified path in several formats: `.h` text dumps, `schemas.json`, `schemas_jsonschema.json` (JSON Schema 2020-12), `convars.txt`, and `module_metadata.json`.

## Building

Prerequisites: CMake 3.16+, a C++20 compiler (MSVC on Windows, GCC on Linux), and all git submodules initialized.

```sh
git submodule update --init --recursive
mkdir build && cd build
cmake ..
# Windows: open the generated .sln in Visual Studio and build
# Linux: make
```

Three executables are produced: `DumpSource2-CS2`, `DumpSource2-DOTA`, `DumpSource2-DEADLOCK`.

CI runs on both Ubuntu and Windows via `.github/workflows/build.yml`.

## Code Style

- **Clang-format** (`.clang-format`): K&R style, 4-space tab width, no column limit, left pointer alignment. Run clang-format before committing.
- **Indentation**: tabs (per `.editorconfig`), UTF-8.

## Architecture

Entry point: `src/main/main.cpp` → `AppSystemInit()` in `src/main/appframework.cpp`.

`appframework.cpp` drives the full lifecycle:
1. Loads game modules via `src/main/utils/module.h` (signature-scan based dynamic loader).
2. Acquires `ISchemaSystem` and `ICvar` interfaces, stored as globals in `src/main/interfaces.h`.
3. Calls each dumper in sequence.

**Dumpers** live under `src/main/dumpers/`:
- `schemas/` — three exporters: filesystem text (`.h` files), JSON (`schemas.json`), and JSON Schema 2020-12 (`schemas_jsonschema.json`). The JSON Schema exporter is the most complex; it synthesizes `$defs` for atomic/math types, maps SDK type strings via `NormalizeTypeString`, and emits per-game `#ifdef`-guarded synthetic types.
- `concommands/` — dumps cvars and concommands to `convars.txt`.
- `module_metadata/` — emits `module_metadata.json` with build strings and version info.

**Game/SDK mapping** is configured in `src/main/CMakeLists.txt` via `build_game(GAME, GAME_PATH, SDK)`. Each game is a separate CMake target with its own `GAME_*` preprocessor define (e.g. `GAME_CS2`). Use `#ifdef GAME_CS2` etc. to gate game-specific code.

**Global state**: output path and version string live in `src/main/globalvariables.h`.

**Platform abstraction**: `src/main/utils/plat.h` and `oslink.h` wrap Windows/Linux differences.

## No Tests

There is no unit test suite. Validation is done by running the tool against a live game installation and inspecting the output files.
