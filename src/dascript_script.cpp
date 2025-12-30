#include "dascript_script.h"

#include <cstdlib>
#include <cstring>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "dascript_language.h"
#include "dascript_instance.h"

#if DASCRIPT_HAS_DASLANG
#include <daScript/daScript.h>
#include <daScript/ast/ast.h>
#include <daScript/simulate/debug_info.h>

// Optional: basic module to let scripts interact with Godot.
// Usage in .das:
//   require godot
//   godot::print("hello")
namespace {
using namespace das;

static void gd_print(const char *p_text) {
	if (!p_text) {
		godot::UtilityFunctions::print(godot::String(""));
		return;
	}
	godot::UtilityFunctions::print(godot::String(p_text));
}

static void gd_call_method0(void *p_self, const char *p_method) {
	if (!p_self || !p_method) {
		return;
	}
	auto *obj = reinterpret_cast<godot::Object *>(p_self);
	obj->call(godot::StringName(p_method));
}

class Module_Godot : public Module {
public:
	Module_Godot() : Module("godot") {
		ModuleLibrary lib(this);
		lib.addBuiltInModule();
		addExtern<DAS_BIND_FUN(gd_print), SimNode_ExtFuncCall>(*this, lib, "print", SideEffects::modifyExternal, "gd_print")
			->args({"text"});
		addExtern<DAS_BIND_FUN(gd_call_method0), SimNode_ExtFuncCall>(*this, lib, "call_method0", SideEffects::modifyExternal, "gd_call_method0")
			->args({"self", "method"});
	}
	virtual ModuleAotType aotRequire(TextWriter &) const override { return ModuleAotType::cpp; }
};

static char *dup_cstr(const char *p_str, size_t p_len) {
	char *mem = (char *)malloc(p_len + 1);
	if (!mem) {
		return nullptr;
	}
	memcpy(mem, p_str, p_len);
	mem[p_len] = '\0';
	return mem;
}
} // namespace

godot::HashSet<godot::DAScript *> godot::DAScript::live_scripts;
#endif

namespace godot {

static bool is_ident_char(char32_t c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

void DAScript::scan_discovered_methods() {
	discovered_methods.clear();

	// Very naive scan for "def <name>" patterns.
	// This will be replaced by real Daslang parsing/compilation.
	String code = source_code;
	int32_t idx = 0;
	while (true) {
		idx = code.find("def", idx);
		if (idx < 0) {
			break;
		}
		int32_t i = idx + 3;
		while (i < code.length() && (code[i] == ' ' || code[i] == '\t')) {
			i++;
		}
		int32_t start = i;
		while (i < code.length() && is_ident_char(code[i])) {
			i++;
		}
		if (i > start) {
			StringName name = code.substr(start, i - start);
			discovered_methods.insert(name);
		}
		idx = i;
	}

	// Always allow these common callbacks even if not discovered yet.
	discovered_methods.insert(StringName("_ready"));
	discovered_methods.insert(StringName("_process"));
	discovered_methods.insert(StringName("_physics_process"));
	discovered_methods.insert(StringName("_enter_tree"));
	discovered_methods.insert(StringName("_exit_tree"));
}

DAScript::DAScript() {
	#if DASCRIPT_HAS_DASLANG
	live_scripts.insert(this);
	#endif
}

DAScript::~DAScript() {
	#if DASCRIPT_HAS_DASLANG
	live_scripts.erase(this);
	#endif
}

void DAScript::_set_source_code(const String &p_code) {
	source_code = p_code;
	scan_discovered_methods();
	// In-editor edits should make reload/validation meaningful.
	// Godot will call _reload separately in many cases.
}

Error DAScript::_reload(bool /*p_keep_state*/) {
	#if DASCRIPT_HAS_DASLANG
	return compile_source(false);
	#else
	// Fallback: reload just re-scans the source.
	valid = true;
	scan_discovered_methods();
	return OK;
	#endif
}

#if DASCRIPT_HAS_DASLANG
Error DAScript::compile_source(bool /*p_keep_state*/) {
	compile_log = "";
	compile_error = "";
	compile_errors = Array();
	compiled_program = nullptr;

	// Resolve a stable, OS path for daScript's file-based diagnostics.
	String res_path = get_path();
	String os_path = res_path;
	if (godot::ProjectSettings::get_singleton()) {
		os_path = godot::ProjectSettings::get_singleton()->globalize_path(res_path);
	}
	if (os_path.is_empty()) {
		os_path = "<dascript>";
	}

	std::string file_name = os_path.utf8().get_data();
	std::string code_utf8 = source_code.utf8().get_data();

	TextWriter logs;
	ModuleGroup libGroup;
	libGroup.addBuiltInModule();
	libGroup.addModule(new Module_Godot());

	FileAccessPtr access = make_smart<FileAccess>();
	char *owned = dup_cstr(code_utf8.c_str(), code_utf8.size());
	if (!owned) {
		valid = false;
		compile_error = "Out of memory while preparing script source.";
		return ERR_OUT_OF_MEMORY;
	}
	auto fileInfo = make_unique<TextFileInfo>(owned, (uint32_t)code_utf8.size(), /*own*/true);
	access->setFileInfo(file_name, das::move(fileInfo));

	compiled_program = compileDaScript(file_name, access, logs, libGroup);
	compile_log = String(logs.str().c_str());

	if (!compiled_program || compiled_program->failed()) {
		valid = false;
		compile_error = "daScript compilation failed.";
		if (compiled_program) {
			for (auto &err : compiled_program->errors) {
				Dictionary e;
				e["line"] = (int)err.at.line;
				e["column"] = (int)err.at.column;
				e["message"] = String(err.what.c_str());
				e["extra"] = String(err.extra.c_str());
				compile_errors.push_back(e);
			}
		}
		return ERR_PARSE_ERROR;
	}

	// Update discovered methods (best-effort). We keep the quick scan for now,
	// but compilation success means the callbacks are invokable.
	valid = true;
	scan_discovered_methods();
	return OK;
}
#endif

bool DAScript::_has_method(const StringName &p_method) const {
	return discovered_methods.has(p_method);
}

ScriptLanguage *DAScript::_get_language() const {
	return DAScriptLanguage::get_singleton();
}

TypedArray<Dictionary> DAScript::_get_script_method_list() const {
	TypedArray<Dictionary> list;
	for (const StringName &name : discovered_methods) {
		Dictionary d;
		d["name"] = String(name);
		list.push_back(d);
	}
	return list;
}

TypedArray<Dictionary> DAScript::_get_script_property_list() const {
	return TypedArray<Dictionary>();
}

void *DAScript::_instance_create(Object *p_for_object) const {
	// Create a GDExtension script instance with our callbacks.
	return DAScriptInstance::create(p_for_object, const_cast<DAScript *>(this));
}

void *DAScript::_placeholder_instance_create(Object *p_for_object) const {
	// Use a normal instance even for placeholders; editor tooling can improve later.
	return _instance_create(p_for_object);
}

} // namespace godot
