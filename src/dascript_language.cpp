#include "dascript_language.h"

#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "dascript_script.h"

#include <godot_cpp/templates/hash_map.hpp>

#include <mutex>

namespace godot {

namespace {
struct ValidationCacheEntry {
	uint64_t source_hash = 0;
	Array errors;
	bool valid = true;
};

static std::mutex s_validation_cache_mutex;
static HashMap<String, ValidationCacheEntry> s_validation_cache;
} // namespace

void DAScriptLanguage::cache_validation_result(const String &p_path, uint64_t p_source_hash, const Array &p_errors, bool p_valid) {
	if (p_path.is_empty()) {
		return;
	}
	std::lock_guard<std::mutex> lock(s_validation_cache_mutex);
	ValidationCacheEntry e;
	e.source_hash = p_source_hash;
	e.errors = p_errors;
	e.valid = p_valid;
	s_validation_cache[p_path] = e;
}

bool DAScriptLanguage::get_cached_validation_result(const String &p_path, uint64_t p_source_hash, Array &r_errors, bool &r_valid) {
	std::lock_guard<std::mutex> lock(s_validation_cache_mutex);
	HashMap<String, ValidationCacheEntry>::Iterator it = s_validation_cache.find(p_path);
	if (it == s_validation_cache.end()) {
		return false;
	}
	if (it->value.source_hash != p_source_hash) {
		return false;
	}
	r_errors = it->value.errors;
	r_valid = it->value.valid;
	return true;
}

DAScriptLanguage *DAScriptLanguage::singleton = nullptr;

static const char *DAS_SIMPLE_TEMPLATE = R""""(require godot

// class: _CLASS_
// base: _BASE_

def _ready()
_TS_pass

def _process(delta: float)
_TS_pass
)"""";

static String _das_get_indentation() {
	// Keep it simple for now. Later we can read the editor's indentation settings in TOOLS builds.
	return "\t";
}

String DAScriptLanguage::_get_name() const {
	return "DAScript";
}

void DAScriptLanguage::_init() {
	// Language initialization (called once when the language is registered)
}

String DAScriptLanguage::_get_type() const {
	// This is shown in some editor contexts.
	return "DAScript";
}

String DAScriptLanguage::_get_extension() const {
	return "das";
}

void DAScriptLanguage::_finish() {
	// Language cleanup (called when the language is unregistered)
}

PackedStringArray DAScriptLanguage::_get_recognized_extensions() const {
	PackedStringArray exts;
	exts.push_back("das");
	return exts;
}

PackedStringArray DAScriptLanguage::_get_reserved_words() const {
	// Editor-facing list for highlighting/completion.
	// This is intentionally a superset of daScript keywords and literals.
	PackedStringArray words;
	const char *kw[] = {
		"def",
		"let",
		"var",
		"return",
		"if",
		"else",
		"for",
		"while",
		"do",
		"break",
		"continue",
		"switch",
		"case",
		"default",
		"try",
		"catch",
		"finally",
		"defer",
		"yield",
		"in",
		"require",
		"module",
		"struct",
		"class",
		"enum",
		"typedef",
		"type",
		"extern",
		"unsafe",
		"shared",
		"public",
		"private",
		"new",
		"delete",
		"true",
		"false",
		"null",
	};
	for (const char *k : kw) {
		words.push_back(k);
	}
	return words;
}

bool DAScriptLanguage::_is_control_flow_keyword(const String &p_keyword) const {
	return p_keyword == "if" || p_keyword == "else" || p_keyword == "for" || p_keyword == "while" || p_keyword == "do" || p_keyword == "break" || p_keyword == "continue" || p_keyword == "return" || p_keyword == "switch" || p_keyword == "case" || p_keyword == "default" || p_keyword == "try" || p_keyword == "catch" || p_keyword == "finally" || p_keyword == "defer";
}

PackedStringArray DAScriptLanguage::_get_comment_delimiters() const {
	// daScript supports C-style line/block comments.
	PackedStringArray delims;
	delims.push_back("//");
	delims.push_back("/* */");
	return delims;
}

PackedStringArray DAScriptLanguage::_get_doc_comment_delimiters() const {
	PackedStringArray delims;
	// daScript doesn't have a dedicated doc-comment syntax; offer common choices.
	delims.push_back("///");
	delims.push_back("/** */");
	return delims;
}

PackedStringArray DAScriptLanguage::_get_string_delimiters() const {
	PackedStringArray delims;
	delims.push_back("\" \"");
	delims.push_back("' '");
	return delims;
}

Object *DAScriptLanguage::_create_script() const {
	return memnew(DAScript);
}

Dictionary DAScriptLanguage::_validate(const String &p_script, const String &p_path, bool /*p_validate_functions*/, bool p_validate_errors, bool /*p_validate_warnings*/, bool /*p_validate_safe_lines*/) const {
	// Godot expects certain keys to always exist in the returned Dictionary.
	// Missing keys can break script creation in the editor.
	Dictionary result;
	result[String("path")] = p_path;
	result[String("valid")] = true;
	result[String("errors")] = Array();
	result[String("warnings")] = Array();
	result[String("functions")] = Array();
	result[String("safe_lines")] = PackedInt32Array();
	// Godot requires "inherit" to exist even if empty.
	result[String("inherit")] = String();

	// IMPORTANT:
	// - Keep typing safe: compile is wrapped and cached.
	// - Still show real errors in the Script Editor when Godot asks for them.
	Array errors;

	// Lightweight heuristic: flag Python-like ':' blocks early.
	if (!p_script.is_empty()) {
		int32_t idx = 0;
		while (idx >= 0) {
			idx = p_script.find("def", idx);
			if (idx < 0) {
				break;
			}
			int32_t line_end = p_script.find("\n", idx);
			if (line_end < 0) {
				line_end = p_script.length();
			}
			String line = p_script.substr(idx, line_end - idx);
			if (line.find(":") >= 0) {
				Dictionary e;
				e[String("line")] = 0;
				e[String("column")] = 0;
				e[String("message")] = String("daScript does not use ':' blocks. Remove ':' and use daScript indentation/syntax.");
				errors.push_back(e);
				break;
			}
			idx = line_end;
		}
	}

	#if DASCRIPT_HAS_DASLANG
	// Do NOT compile during validation; it can be called on background threads during editor load.
	// Instead, return cached diagnostics from the last compile-on-save / tool reload.
	if (p_validate_errors) {
		Array cached_errors;
		bool cached_valid = true;
		const uint64_t h = (uint64_t)p_script.hash();
		if (get_cached_validation_result(p_path, h, cached_errors, cached_valid)) {
			errors = cached_errors;
			if (!cached_valid) {
				result[String("valid")] = false;
				result[String("errors")] = errors;
				return result;
			}
		}
	}
	#endif

	if (errors.size() > 0) {
		result[String("valid")] = false;
		result[String("errors")] = errors;
	}

	return result;
}

String DAScriptLanguage::_debug_get_error() const {
	return String();
}

int32_t DAScriptLanguage::_debug_get_stack_level_count() const {
	return 0;
}

int32_t DAScriptLanguage::_debug_get_stack_level_line(int32_t /*p_level*/) const {
	return 0;
}

String DAScriptLanguage::_debug_get_stack_level_function(int32_t /*p_level*/) const {
	return String();
}

String DAScriptLanguage::_debug_get_stack_level_source(int32_t /*p_level*/) const {
	return String();
}

Dictionary DAScriptLanguage::_debug_get_stack_level_locals(int32_t /*p_level*/, int32_t /*p_max_subitems*/, int32_t /*p_max_depth*/) {
	return Dictionary();
}

Dictionary DAScriptLanguage::_debug_get_stack_level_members(int32_t /*p_level*/, int32_t /*p_max_subitems*/, int32_t /*p_max_depth*/) {
	return Dictionary();
}

void *DAScriptLanguage::_debug_get_stack_level_instance(int32_t /*p_level*/) {
	return nullptr;
}

Dictionary DAScriptLanguage::_debug_get_globals(int32_t /*p_max_subitems*/, int32_t /*p_max_depth*/) {
	return Dictionary();
}

String DAScriptLanguage::_debug_parse_stack_level_expression(int32_t /*p_level*/, const String &/*p_expression*/, int32_t /*p_max_subitems*/, int32_t /*p_max_depth*/) {
	return String();
}

String DAScriptLanguage::_validate_path(const String &p_path) const {
	// Accept .das paths.
	if (p_path.get_extension().to_lower() != "das") {
		return "Invalid extension for DAScript. Expected .das";
	}
	return "";
}

Ref<Script> DAScriptLanguage::_make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
	Ref<DAScript> scr;
	scr.instantiate();

	// Godot passes the template *content* here.
	String code = p_template;
	code = code.replace("_BASE_", p_base_class_name);
	code = code.replace("_CLASS_", p_class_name);
	code = code.replace("_TS_", _das_get_indentation());

	scr->set_source_code(code);
	return scr;
}

TypedArray<Dictionary> DAScriptLanguage::_get_built_in_templates(const StringName &p_object) const {
	TypedArray<Dictionary> templates;
	// Godot expects dictionaries with the ScriptTemplate fields.
	// The engine checks for "inherit" (base object type) explicitly.
	Dictionary t;
	// "id" is required by ScriptLanguageExtension wrapper. Use a stable identifier.
	t["id"] = 0;
	t["origin"] = 0; // TemplateOrigin::BUILT_IN = 0
	t["inherit"] = String(p_object);
	t["name"] = String("Default");
	t["description"] = String("Base template for Node with default Godot callbacks");
	t["content"] = String(DAS_SIMPLE_TEMPLATE);
	// If Godot asks for templates for an unrelated base, we can still return our default.
	// This matches the pragmatic behavior in many languages (at least one template exists).
	templates.push_back(t);
	return templates;
}

bool DAScriptLanguage::_is_using_templates() {
	return true;
}

bool DAScriptLanguage::_supports_builtin_mode() const {
	// Allow scripts embedded in scenes/resources.
	return true;
}

bool DAScriptLanguage::_supports_documentation() const {
	return false;
}

bool DAScriptLanguage::_can_inherit_from_file() const {
	// Allow extending from other script files.
	return true;
}

bool DAScriptLanguage::_has_named_classes() const {
	return false;
}

int32_t DAScriptLanguage::_find_function(const String &p_function, const String &p_code) const {
	(void)p_function;
	(void)p_code;
	return -1;
}

String DAScriptLanguage::_make_function(const String &p_class_name, const String &p_function_name, const PackedStringArray &p_function_args) const {
	(void)p_class_name;
	String code;
	code += "def " + p_function_name + "(";
	for (int i = 0; i < p_function_args.size(); i++) {
		if (i > 0) {
			code += ", ";
		}
		code += p_function_args[i];
	}
	code += ")\n";
	code += _das_get_indentation() + "pass\n";
	return code;
}

bool DAScriptLanguage::_can_make_function() const {
	return true;
}

Error DAScriptLanguage::_open_in_external_editor(const Ref<Script> &p_script, int32_t p_line, int32_t p_column) {
	(void)p_script;
	(void)p_line;
	(void)p_column;
	return ERR_UNAVAILABLE;
}

bool DAScriptLanguage::_overrides_external_editor() {
	return false;
}

ScriptLanguage::ScriptNameCasing DAScriptLanguage::_preferred_file_name_casing() const {
	return ScriptLanguage::SCRIPT_NAME_CASING_AUTO;
}

Dictionary DAScriptLanguage::_complete_code(const String &p_code, const String &p_path, Object *p_owner) const {
	(void)p_code;
	(void)p_path;
	(void)p_owner;
	return Dictionary();
}

Dictionary DAScriptLanguage::_lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner) const {
	(void)p_code;
	(void)p_symbol;
	(void)p_path;
	(void)p_owner;
	return Dictionary();
}

String DAScriptLanguage::_auto_indent_code(const String &p_code, int32_t p_from_line, int32_t p_to_line) const {
	(void)p_from_line;
	(void)p_to_line;
	return p_code;
}

void DAScriptLanguage::_add_global_constant(const StringName &p_name, const Variant &p_value) {
	(void)p_name;
	(void)p_value;
}

void DAScriptLanguage::_add_named_global_constant(const StringName &p_name, const Variant &p_value) {
	(void)p_name;
	(void)p_value;
}

void DAScriptLanguage::_remove_named_global_constant(const StringName &p_name) {
	(void)p_name;
}

void DAScriptLanguage::_reload_all_scripts() {
	#if DASCRIPT_HAS_DASLANG
	for (DAScript *scr : DAScript::get_live_scripts()) {
		if (scr) {
			scr->_reload(false);
		}
	}
	#endif
}

void DAScriptLanguage::_reload_scripts(const Array &p_scripts, bool p_soft_reload) {
	(void)p_soft_reload;
	for (int i = 0; i < p_scripts.size(); i++) {
		Ref<Script> s = p_scripts[i];
		Ref<DAScript> ds = s;
		if (ds.is_valid()) {
			ds->_reload(false);
		}
	}
}

void DAScriptLanguage::_reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) {
	(void)p_soft_reload;
	Ref<DAScript> ds = p_script;
	if (ds.is_valid()) {
		#if DASCRIPT_HAS_DASLANG
		ds->compile_for_tools();
		#else
		ds->_reload(false);
		#endif
	}
}

void DAScriptLanguage::_thread_enter() {
	// Called when a thread starts using this language.
	// If/when daScript gets a per-thread context, initialize it here.
}

void DAScriptLanguage::_thread_exit() {
	// Called when a thread stops using this language.
	// If/when daScript gets a per-thread context, tear it down here.
}

void DAScriptLanguage::_frame() {
	// Called once per engine frame.
	// Useful for time-sliced work (compilation queues, etc.).
}

bool DAScriptLanguage::_handles_global_class_type(const String &p_type) const {
	// Godot uses this to determine if a global class type belongs to this language.
	// We currently don't support `class_name`-style global classes, but we must
	// report our own type so the editor can query safely.
	return p_type == _get_type() || p_type == _get_name();
}

Dictionary DAScriptLanguage::_get_global_class_name(const String &p_path) const {
	// Global class support (class name/base/icon) is not implemented yet.
	// Return an empty dictionary so Godot treats it as "no global class".
	// Keys expected by Godot (when supported) are typically: class_name, base_type, icon_path.
	(void)p_path;
	return Dictionary();
}

TypedArray<Dictionary> DAScriptLanguage::_debug_get_current_stack_info() {
	// Return current stack info for debugging
	// Empty for now - would need debugger support
	return TypedArray<Dictionary>();
}

} // namespace godot
