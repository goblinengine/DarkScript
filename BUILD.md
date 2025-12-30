# DAScript (Daslang) GDExtension

This extension provides a new Godot 4.5+ scripting language named **DAScript** with file extension **.das**.

## Submodules

This repository is expected to be used with submodules:

- `godot-cpp` at repository root (already used by other extensions)
- `daScript` (Daslang) as a submodule inside this extension

Suggested layout:

- `extensions/dascript/lib/daScript` (git submodule pointing at https://github.com/GaijinEntertainment/daScript)

## Build (Windows)

1. Install Python 3.8+.
2. Install SCons:
   - `pip install scons`
3. Ensure submodules are initialized:
   - `git submodule update --init --recursive`
4. Build:
   - From repo root: `build.bat dascript`
   - Or from `extensions/dascript`: `build.bat`

This produces binaries in:

- `addons/dascript/bin/`

## Notes

- The current implementation scaffolds the Godot scripting hooks (language, script resource, and script instance callbacks).
- Full Daslang execution requires wiring in the daScript embedding API (see https://daslang.io/doc/reference/embedding.html).
