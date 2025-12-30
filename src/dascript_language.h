#pragma once

#include <godot_cpp/classes/script_language_extension.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class DAScriptLanguage : public ScriptLanguageExtension {
	GDCLASS(DAScriptLanguage, ScriptLanguageExtension)

	static DAScriptLanguage *singleton;

protected:
	static void _bind_methods() {}

public:
	static DAScriptLanguage *get_singleton() { return singleton; }
	static void set_language_singleton(DAScriptLanguage *p_singleton) { singleton = p_singleton; }

	String _get_name() const override;
	String _get_type() const override;
	String _get_extension() const override;
	PackedStringArray _get_recognized_extensions() const override;
	PackedStringArray _get_reserved_words() const override;
	bool _is_control_flow_keyword(const String &p_keyword) const override;
	PackedStringArray _get_comment_delimiters() const override;
	PackedStringArray _get_string_delimiters() const override;
	Object *_create_script() const override;
	Dictionary _validate(const String &p_script, const String &p_path, bool p_validate_functions, bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const override;
	String _validate_path(const String &p_path) const override;
	Ref<Script> _make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const override;
};

} // namespace godot
