# DAScript (Daslang) for Godot 4.5+

This extension adds a new scripting language to Godot:

- **Language name**: DAScript
- **File extension**: `.das`

It is intended to embed **Daslang** (formerly daScript) and expose Godot APIs to Daslang.

## Status

Scaffolded scripting infrastructure:

- `DAScriptLanguage` (ScriptLanguageExtension) registers with Godot via `Engine::register_script_language`.
- `DAScript` (ScriptExtension) holds source and creates instances.
- A minimal ScriptInstance bridge exists via GDExtension `GDExtensionScriptInstanceInfo3` callbacks.

DaScript execution and API bindings generation are stubbed/placeholder for now.

## Repo structure

- Extension code: `extensions/dascript/src`
- Godot addon loader: `addons/dascript/dascript.gdextension`
- Optional Daslang submodule: `extensions/dascript/thirdparty/daScript`

## Next steps (planned)

- Integrate Daslang VM + module system.
- Implement robust Variant <-> Daslang value marshaling.
- Generate Daslang wrappers for Godot’s API from `godot-cpp/gdextension/extension_api.json`.
