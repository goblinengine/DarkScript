#include "dascript_script.h"

#include <cstdlib>
#include <cstring>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/project_settings.hpp>
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

static godot::String variant_to_string(const godot::Variant &v) {
	return v.stringify();
}

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
		out += das::debug_value(args[i], ti, das::PrintFlags::singleLine);
	}

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
		// print(...) without wrappers: accept arbitrary types via vec4f + TypeInfo.
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


static bool is_ident_char32(char32_t c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static godot::String rewrite_unqualified_call(const godot::String &p_source, const char *p_name, const char *p_qualified) {
	const godot::String name(p_name);
	const godot::String qualified(p_qualified);

	const int32_t len = p_source.length();
	int32_t pos = 0;
	int32_t idx = 0;
	godot::String out;
	while ((idx = p_source.find(name, pos)) >= 0) {
		// Reject if it's part of an identifier or already namespace-qualified (foo::bar).
		if (idx > 0) {
			char32_t prev = p_source[idx - 1];
			if (is_ident_char32(prev) || prev == ':') {
				pos = idx + name.length();
				continue;
			}
		}

		int32_t after = idx + name.length();
		while (after < len && (p_source[after] == ' ' || p_source[after] == '\t')) {
			after++;
		}
		if (after >= len || p_source[after] != '(') {
			pos = idx + name.length();
			continue;
		}

		out += p_source.substr(pos, idx - pos);
		out += qualified;
		pos = idx + name.length();
	}
	out += p_source.substr(pos);
	return out;
}

static godot::String build_compilation_source(const godot::String &p_source) {
	// Default to gen1.5 syntax unless the script explicitly sets gen2.
	const bool has_gen2 = (p_source.find("options gen2=") >= 0);
	const bool has_require_godot = (p_source.find("require godot") >= 0);
	const bool needs_print = (p_source.find("print(") >= 0 && p_source.find("godot::print(") < 0);
	const bool needs_call0 = (p_source.find("call_method0(") >= 0 && p_source.find("godot::call_method0(") < 0);
	const bool needs_godot = (needs_print || needs_call0);

	godot::String prefix;
	if (!has_gen2) {
		prefix += "options gen2=false\n";
	}
	if (needs_godot && !has_require_godot) {
		prefix += "require godot\n";
	}

	godot::String out = p_source;
	if (needs_print) {
		out = rewrite_unqualified_call(out, "print", "godot::print");
	}
	if (needs_call0) {
		out = rewrite_unqualified_call(out, "call_method0", "godot::call_method0");
	}

	if (!prefix.is_empty()) {
		out = prefix + "\n" + out;
	}
	return out;
}

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
		scan_discovered_methods();
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
	static std::mutex s_compile_mutex;
	std::lock_guard<std::mutex> lock(s_compile_mutex);
	compile_log = "";
	compile_error = "";
	compile_errors = Array();
	compiled_program = nullptr;
	compiled_access.reset();

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
	String compilation_source = build_compilation_source(source_code);
	std::string code_utf8 = compilation_source.utf8().get_data();

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
	compiled_access = access;

	// IMPORTANT: compileDaScript() builds module dependencies but does not expose an exportAll flag.
	// For Godot script callbacks we need functions to be exported into the runtime Context;
	// otherwise the simulated Context can end up with 0 callable functions.
	compiled_program = parseDaScript(file_name, /*moduleName*/"", access, logs, libGroup, /*exportAll*/true);
	compile_log = String(logs.str().c_str());
	bool failed = false;
	if (compiled_program) {
		failed = compiled_program->failed();
	}

	if (!compiled_program || failed) {
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

Error DAScript::compile_silent() {
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
		if (err != OK || !valid) {
			String header = compile_error;
			if (header.is_empty()) {
				header = "daScript compilation failed.";
			}
			godot::UtilityFunctions::push_error(String("[DAScript] compile failed: ") + header);

			// Print a few structured errors with line/column.
			const int max_emit = 10;
			for (int i = 0; i < compile_errors.size() && i < max_emit; i++) {
				Dictionary e = compile_errors[i];
				int line = (int)e.get("line", 0);
				int col = (int)e.get("column", 0);
				String msg = e.get("message", String(""));
				String extra = e.get("extra", String(""));
				String loc = String("L") + itos(line) + String(":") + itos(col);
				if (!extra.is_empty()) {
					msg += String(" ") + extra;
				}
				godot::UtilityFunctions::push_error(String("[DAScript] ") + loc + String(": ") + msg);
			}
			if (!compile_log.is_empty()) {
				godot::UtilityFunctions::print(String("[DAScript] compile log:\n") + compile_log);
			}
		}
		return err;
	} catch (const std::exception &e) {
		valid = false;
		compile_error = String("Exception during daScript compilation: ") + String(e.what());
		compile_errors = Array();
		godot::UtilityFunctions::push_error(String("[DAScript] ") + compile_error);
		return ERR_PARSE_ERROR;
	} catch (...) {
		valid = false;
		compile_error = String("Unknown exception during daScript compilation.");
		compile_errors = Array();
		godot::UtilityFunctions::push_error(String("[DAScript] ") + compile_error);
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
