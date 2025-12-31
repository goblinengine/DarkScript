#include "dascript_script.h"

#include <cstdlib>
#include <cstring>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "dascript_language.h"
#include "dascript_instance.h"
#include "dascript_variant_glue.h"

#include <exception>
#include <mutex>

#if DASCRIPT_HAS_DASLANG
#include <daScript/daScript.h>
#include <daScript/ast/ast.h>
#include <daScript/ast/ast_handle.h>
#include <daScript/ast/ast_typefactory_bind.h>
#include <daScript/simulate/debug_info.h>
#include <daScript/simulate/debug_print.h>

// Optional: basic module to let scripts interact with Godot.
// Usage in .das:
//   require godot
//   godot::print("hello")
MAKE_TYPE_FACTORY(GDVariant, godot::DasVariant);

namespace {
using namespace das;
using godot::DasVariant;
using godot::das_make_variant_int;
using godot::das_make_variant_int64;
using godot::das_make_variant_float;
using godot::das_make_variant_bool;
using godot::das_make_variant_string;
using godot::das_variant_release;

static vec4f gd_print_any(das::Context & /*context*/, das::SimNode_CallBase *call, vec4f *args) {
	if (!call) {
		godot::UtilityFunctions::print(godot::String(""));
		return v_zero();
	}

	das::string out;
	for (int i = 0; i < call->nArguments; i++) {
		if (i > 0) {
			out += " ";
		}
		das::TypeInfo *ti = call->types ? call->types[i] : nullptr;
		if (!ti) {
			out += "<unknown>";
			continue;
		}
		// Use daScript's runtime type information and walker. This supports refs (e.g. float&)
		// and matches embedding best-practices (no source rewriting).
		out += das::debug_value(args[i], ti, das::PrintFlags::none);
	}

	// Print once, like GDScript does, with space-separated arguments.
	godot::UtilityFunctions::print(godot::String(out.c_str()));
	return v_zero();
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
		// Handled type (refcounted) - avoids copy/move restrictions for Variant.
		addAnnotation(make_smart<ManagedStructureAnnotation<DasVariant, true, true>>("GDVariant", lib, "godot::DasVariant"));

		// GDScript-like print: accept arbitrary argument types via daScript interop.
		addInterop<gd_print_any, void, vec4f>(*this, lib, "print", SideEffects::modifyExternal, "gd_print_any")
			->args({"a"});
		addInterop<gd_print_any, void, vec4f, vec4f>(*this, lib, "print", SideEffects::modifyExternal, "gd_print_any")
			->args({"a", "b"});
		addInterop<gd_print_any, void, vec4f, vec4f, vec4f>(*this, lib, "print", SideEffects::modifyExternal, "gd_print_any")
			->args({"a", "b", "c"});
		addInterop<gd_print_any, void, vec4f, vec4f, vec4f, vec4f>(*this, lib, "print", SideEffects::modifyExternal, "gd_print_any")
			->args({"a", "b", "c", "d"});
		addInterop<gd_print_any, void, vec4f, vec4f, vec4f, vec4f, vec4f>(*this, lib, "print", SideEffects::modifyExternal, "gd_print_any")
			->args({"a", "b", "c", "d", "e"});
		addInterop<gd_print_any, void, vec4f, vec4f, vec4f, vec4f, vec4f, vec4f>(*this, lib, "print", SideEffects::modifyExternal, "gd_print_any")
			->args({"a", "b", "c", "d", "e", "f"});
		addInterop<gd_print_any, void, vec4f, vec4f, vec4f, vec4f, vec4f, vec4f, vec4f>(*this, lib, "print", SideEffects::modifyExternal, "gd_print_any")
			->args({"a", "b", "c", "d", "e", "f", "g"});
		addInterop<gd_print_any, void, vec4f, vec4f, vec4f, vec4f, vec4f, vec4f, vec4f, vec4f>(*this, lib, "print", SideEffects::modifyExternal, "gd_print_any")
			->args({"a", "b", "c", "d", "e", "f", "g", "h"});

		// Constructors to build Variant values from primitives.
		addExtern<DAS_BIND_FUN(das_make_variant_int), SimNode_ExtFuncCall>(*this, lib, "variant_int", SideEffects::none, "das_make_variant_int")
			->args({"value"});
		addExtern<DAS_BIND_FUN(das_make_variant_int64), SimNode_ExtFuncCall>(*this, lib, "variant_int64", SideEffects::none, "das_make_variant_int64")
			->args({"value"});
		addExtern<DAS_BIND_FUN(das_make_variant_float), SimNode_ExtFuncCall>(*this, lib, "variant_float", SideEffects::none, "das_make_variant_float")
			->args({"value"});
		addExtern<DAS_BIND_FUN(das_make_variant_bool), SimNode_ExtFuncCall>(*this, lib, "variant_bool", SideEffects::none, "das_make_variant_bool")
			->args({"value"});
		addExtern<DAS_BIND_FUN(das_make_variant_string), SimNode_ExtFuncCall>(*this, lib, "variant_string", SideEffects::none, "das_make_variant_string")
			->args({"value"});
		addExtern<DAS_BIND_FUN(das_variant_release), SimNode_ExtFuncCall>(*this, lib, "variant_release", SideEffects::modifyExternal, "das_variant_release")
			->args({"value"});
		addExtern<DAS_BIND_FUN(gd_call_method0), SimNode_ExtFuncCall>(*this, lib, "call_method0", SideEffects::modifyExternal, "gd_call_method0")
			->args({"self", "method"});
	}
	virtual ModuleAotType aotRequire(TextWriter &) const override { return ModuleAotType::cpp; }
};

static ModuleGroup &get_das_module_group() {
	static ModuleGroup s_group;
	static std::once_flag s_once;
	std::call_once(s_once, []() {
		s_group.addBuiltInModule();
		s_group.addModule(new Module_Godot());
	});
	return s_group;
}



// NOTE:
// We intentionally do not scan/transform script source text here.
// Any language-level behavior (gen2, tool mode, etc.) should be handled by daScript
// itself via its parser/options and/or compilation policies.

static char *dup_cstr(const char *p_str, size_t p_len) {
	// IMPORTANT: daScript's TextFileInfo::freeSourceData uses das_aligned_free16.
	// Therefore any owned source buffer must be allocated with das_aligned_alloc16.
	// Using malloc here will eventually crash when the editor recompiles/reloads.
	char *mem = (char *)das_aligned_alloc16(p_len + 1);
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
	#if DASCRIPT_HAS_DASLANG
	if (!compiled_program || compiled_program->failed()) {
		return;
	}
	das::Module *mod = compiled_program->getThisModule();
	if (!mod) {
		return;
	}
	mod->functions.foreach([&](const das::FunctionPtr &fn) {
		if (!fn) {
			return;
		}
		// Only functions belonging to this module/script.
		if (fn->module != mod) {
			return;
		}
		// Skip builtins/interop and generics; we want user-defined callable functions.
		if (fn->builtIn || fn->interopFn || fn->isGeneric()) {
			return;
		}
		if (fn->name.empty()) {
			return;
		}
		discovered_methods.insert(StringName(fn->name.c_str()));
	});
	#endif

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

#if DASCRIPT_HAS_DASLANG
void DAScript::_register_instance(DAScriptInstance *p_instance) {
	if (!p_instance) {
		return;
	}
	std::lock_guard<std::mutex> lock(instances_mutex);
	instances.insert(p_instance);
}

void DAScript::_unregister_instance(DAScriptInstance *p_instance) {
	if (!p_instance) {
		return;
	}
	std::lock_guard<std::mutex> lock(instances_mutex);
	instances.erase(p_instance);
}

void DAScript::_invalidate_all_instances() {
	std::lock_guard<std::mutex> lock(instances_mutex);
	for (DAScriptInstance *inst : instances) {
		if (inst) {
			inst->invalidate_context();
		}
	}
}
#endif

void DAScript::_set_source_code(const String &p_code) {
	source_code = p_code;
	// Do not parse/compile on edit. Compilation is explicit (save/run/validate).
	// In-editor edits should make reload/validation meaningful.
	// Godot will call _reload separately in many cases.
}

Error DAScript::_reload(bool /*p_keep_state*/) {
	// Handle empty scripts gracefully
	if (source_code.is_empty()) {
		valid = true;
		discovered_methods.clear();
		return OK;
	}

	// Do not compile while editing in the editor. Godot may call reload extremely frequently
	// (often per keystroke). Compilation should be explicit (save/run) to keep the editor robust.
	if (Engine::get_singleton() && Engine::get_singleton()->is_editor_hint()) {
		valid = true;
		return OK;
	}
	
	#if DASCRIPT_HAS_DASLANG
	try {
		return compile_source(false);
	} catch (const std::exception &e) {
		valid = false;
		compile_error = String("Exception during daScript compilation: ") + String(e.what());
		compile_errors = Array();
		return ERR_PARSE_ERROR;
	} catch (...) {
		valid = false;
		compile_error = String("Unknown exception during daScript compilation.");
		compile_errors = Array();
		return ERR_PARSE_ERROR;
	}
	#else
	// Fallback: reload just re-scans the source.
	valid = true;
	scan_discovered_methods();
	return OK;
	#endif
}

#if DASCRIPT_HAS_DASLANG
Error DAScript::compile_source(bool /*p_keep_state*/) {
	// Serialize compilation with instance calls (godot-das style).
	std::lock_guard<std::recursive_mutex> lang_lock(DAScriptLanguage::get_language_mutex());
	static std::mutex s_compile_mutex;
	std::lock_guard<std::mutex> lock(s_compile_mutex);
	compile_log = "";
	compile_error = "";
	compile_errors = Array();

	// NOTE:
	// We do NOT proactively destroy previously failed daScript Programs here.
	// Some daScript error paths appear to be unsafe during Program teardown, which can
	// crash the editor on subsequent save/recompile attempts.

	// IMPORTANT:
	// Do not clear/replace the currently compiled program until we have successfully
	// compiled a new one. The editor (and tool scripts) may have live instances whose
	// Context holds pointers into the previous Program; destroying it mid-frame can
	// crash the editor.
	das::FileAccessPtr new_access;
	das::ProgramPtr new_program = nullptr;

	// Resolve a stable, OS path for daScript's file-based diagnostics.
	String res_path = diagnostic_path;
	if (res_path.is_empty()) {
		res_path = get_path();
	}
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
	ModuleGroup &libGroup = get_das_module_group();

	FileAccessPtr access = make_smart<FileAccess>();
	char *owned = dup_cstr(code_utf8.c_str(), code_utf8.size());
	if (!owned) {
		valid = false;
		compile_error = "Out of memory while preparing script source.";
		return ERR_OUT_OF_MEMORY;
	}
	auto fileInfo = make_unique<TextFileInfo>(owned, (uint32_t)code_utf8.size(), /*own*/true);
	access->setFileInfo(file_name, das::move(fileInfo));
	// Keep access alive for as long as the compiled program exists.
	new_access = access;

	// IMPORTANT: compileDaScript() builds module dependencies but does not expose an exportAll flag.
	// For Godot script callbacks we need functions to be exported into the runtime Context;
	// otherwise the simulated Context can end up with 0 callable functions.
	try {
		das::CodeOfPolicies policies;
		// Prefer gen1.5 by default. Scripts can override via `options gen2=...`.
		policies.version_2_syntax = false;
		new_program = parseDaScript(file_name, /*moduleName*/"", access, logs, libGroup, /*exportAll*/true, /*isDep*/false, policies);
	} catch (const std::exception &e) {
		valid = false;
		compile_error = String("Exception during parseDaScript: ") + String(e.what());
		// Avoid logging from compilation path; editor validation may run off-thread.
		return ERR_PARSE_ERROR;
	} catch (...) {
		valid = false;
		compile_error = "Unknown exception during parseDaScript.";
		// Avoid logging from compilation path; editor validation may run off-thread.
		return ERR_PARSE_ERROR;
	}
	compile_log = String(logs.str().c_str());
	bool failed = false;
	if (new_program) {
		failed = new_program->failed();
	}

	if (!new_program || failed) {
		valid = false;
		compile_error = "daScript compilation failed.";
		if (new_program) {
			for (auto &err : new_program->errors) {
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

	// Compilation succeeded: swap in the new program/access atomically.
	compiled_program = new_program;
	compiled_access = new_access;
	// Keep any previously failed program retained (do not reset here).
	// Invalidate all live instances so they recreate their Context against the new Program.
	_invalidate_all_instances();

	// Update discovered methods (best-effort). We keep the quick scan for now,
	// but compilation success means the callbacks are invokable.
	valid = true;
	// daScript-native tool mode: `options tool=true`
	tool = false;
	if (new_program) {
		tool = new_program->options.getBoolOption("tool", false);
	}
	scan_discovered_methods();
	return OK;
}

Error DAScript::compile_silent() {
	#if DASCRIPT_HAS_DASLANG
	try {
		Error err = compile_source(false);
		if (err != OK) {
			// Only log outside the editor; editor compilation can run off-thread.
			if (Engine::get_singleton() && !Engine::get_singleton()->is_editor_hint()) {
				if (!compile_error.is_empty()) {
					godot::UtilityFunctions::push_error(String("[DAScript] compile_silent: ") + compile_error);
				}
			}
		}
		return err;
	} catch (const std::exception &e) {
		valid = false;
		compile_error = String("Exception during daScript compilation: ") + String(e.what());
		compile_errors = Array();
		if (Engine::get_singleton() && !Engine::get_singleton()->is_editor_hint()) {
			godot::UtilityFunctions::push_error(String("[DAScript] ") + compile_error);
		}
		return ERR_PARSE_ERROR;
	} catch (...) {
		valid = false;
		compile_error = String("Unknown exception during daScript compilation.");
		compile_errors = Array();
		if (Engine::get_singleton() && !Engine::get_singleton()->is_editor_hint()) {
			godot::UtilityFunctions::push_error(String("[DAScript] ") + compile_error);
		}
		return ERR_PARSE_ERROR;
	}
	#else
	return OK;
	#endif
}

Error DAScript::compile_for_tools() {
	#if DASCRIPT_HAS_DASLANG
	try {
		// Compile even in editor so save/reload can show syntax errors.
		Error err = compile_source(false);
		// Update Script Editor diagnostics cache.
		String p = get_path();
		if (p.is_empty()) {
			p = diagnostic_path;
		}
		DAScriptLanguage::cache_validation_result(p, (uint64_t)source_code.hash(), compile_errors, (err == OK && valid));
		// Do not call UtilityFunctions::push_error/print here.
		// Script validation in the editor may run off the main thread.
		return err;
	} catch (const std::exception &e) {
		valid = false;
		compile_error = String("Exception during daScript compilation: ") + String(e.what());
		compile_errors = Array();
		// Avoid logging from editor validation path.
		return ERR_PARSE_ERROR;
	} catch (...) {
		valid = false;
		compile_error = String("Unknown exception during daScript compilation.");
		compile_errors = Array();
		// Avoid logging from editor validation path.
		return ERR_PARSE_ERROR;
	}
	#else
	return OK;
	#endif
}
#endif

bool DAScript::_has_method(const StringName &p_method) const {
	return discovered_methods.has(p_method);
}

bool DAScript::_has_static_method(const StringName &p_method) const {
	return false; // DAScript doesn't support static methods yet
}

TypedArray<Dictionary> DAScript::_get_script_signal_list() const {
	// No signals declared by the language yet.
	return TypedArray<Dictionary>();
}

Variant DAScript::_get_script_method_argument_count(const StringName &p_method) const {
	// Unknown -> return NIL so engine uses fallback paths.
	(void)p_method;
	return Variant();
}

Dictionary DAScript::_get_method_info(const StringName &p_method) const {
	// Minimal method info. Godot accepts empty dict for unknown methods.
	Dictionary d;
	d["name"] = String(p_method);
	return d;
}

Ref<Script> DAScript::_get_base_script() const {
	return Ref<Script>(); // No inheritance support yet
}

void DAScript::_update_exports() {
	// Called when script properties change in editor
}

TypedArray<Dictionary> DAScript::_get_documentation() const {
	return TypedArray<Dictionary>(); // No documentation support yet
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
