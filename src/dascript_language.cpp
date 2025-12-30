#include "dascript_language.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include "dascript_script.h"

namespace godot {

DAScriptLanguage *DAScriptLanguage::singleton = nullptr;

String DAScriptLanguage::_get_name() const {
	return "DAScript";
}

String DAScriptLanguage::_get_type() const {
	// This is shown in some editor contexts.
	return "DAScript";
}

String DAScriptLanguage::_get_extension() const {
	return "das";
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

PackedStringArray DAScriptLanguage::_get_string_delimiters() const {
	PackedStringArray delims;
	delims.push_back("\" \"");
	delims.push_back("' '");
	return delims;
}

Object *DAScriptLanguage::_create_script() const {
	return memnew(DAScript);
}

Dictionary DAScriptLanguage::_validate(const String &p_script, const String &p_path, bool /*p_validate_functions*/, bool /*p_validate_errors*/, bool /*p_validate_warnings*/, bool /*p_validate_safe_lines*/) const {
	Dictionary result;
	result["path"] = p_path;
	result["warnings"] = Array();

	#if DASCRIPT_HAS_DASLANG
	Ref<DAScript> scr;
	scr.instantiate();
	scr->set_source_code(p_script);
	Error err = scr->_reload(false);

	Array errors;
	if (err != OK || !scr->is_valid()) {
		if (scr->get_compile_errors().size() > 0) {
			errors = scr->get_compile_errors();
		} else {
			Dictionary e;
			e["line"] = 0;
			e["column"] = 0;
			e["message"] = scr->get_compile_error();
			errors.push_back(e);
		}
		result["valid"] = false;
	} else {
		result["valid"] = true;
	}
	result["errors"] = errors;
	result["has_runtime"] = true;
	result["log"] = scr->get_compile_log();
	return result;
	#else
	// No daScript runtime compiled in.
	result["valid"] = true;
	result["errors"] = Array();
	result["has_runtime"] = false;
	result["note"] = "DAScript was built without embedded daScript runtime.";
	return result;
	#endif
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

	String code;
	if (p_template == "Node") {
		code = "// DAScript template\n";
		code += "// class: " + p_class_name + "\n";
		code += "// base: " + p_base_class_name + "\n\n";
		code += "require godot\n\n";
		code += "// Godot calls _ready() with no args.\n";
		code += "// This extension also supports _ready(self) (self = void?) if you prefer.\n";
		code += "def _ready() {\n\tgodot::print(\\\"ready\\\")\n}\n\n";
		code += "// Godot calls _process(delta).\n";
		code += "// This extension also supports _process(self, delta) (self = void?).\n";
		code += "def _process(delta: float) {\n}\n";
	} else {
		code = "// DAScript\n";
		code += "require godot\n\n";
		code += "def _ready() {\n}\n";
	}

	scr->set_source_code(code);
	return scr;
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

} // namespace godot
