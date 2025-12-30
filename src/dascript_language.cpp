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
	// Minimal starter set; expand once real parsing is in.
	PackedStringArray words;
	words.push_back("def");
	words.push_back("let");
	words.push_back("var");
	words.push_back("return");
	words.push_back("if");
	words.push_back("else");
	words.push_back("for");
	words.push_back("while");
	words.push_back("break");
	words.push_back("continue");
	words.push_back("true");
	words.push_back("false");
	words.push_back("null");
	return words;
}

bool DAScriptLanguage::_is_control_flow_keyword(const String &p_keyword) const {
	return p_keyword == "if" || p_keyword == "else" || p_keyword == "for" || p_keyword == "while" || p_keyword == "break" || p_keyword == "continue" || p_keyword == "return";
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
	// Stub validator: no parse errors produced yet.
	Dictionary result;
	result["valid"] = true;
	result["errors"] = Array();
	result["warnings"] = Array();
	result["path"] = p_path;
	result["has_runtime"] = false;
	result["note"] = "DAScript validation is currently a stub.";
	return result;
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
		code += "def _ready(self) {\n}\n\n";
		code += "def _process(self, delta: float) {\n}\n";
	} else {
		code = "// DAScript\n";
		code += "def _ready(self) {\n}\n";
	}

	scr->set_source_code(code);
	return scr;
}

} // namespace godot
