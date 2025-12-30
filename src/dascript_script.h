#pragma once

#include <godot_cpp/classes/script_extension.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/hash_set.hpp>

#if DASCRIPT_HAS_DASLANG
#include <daScript/daScript.h>
#endif

namespace godot {

class DAScript : public ScriptExtension {
	GDCLASS(DAScript, ScriptExtension)

	String source_code;
	// Separate from Resource path: used for compiler diagnostics during validation.
	String diagnostic_path;
	StringName base_type = "Node";
	HashSet<StringName> discovered_methods;
	bool valid = true;

#if DASCRIPT_HAS_DASLANG
	das::ProgramPtr compiled_program;
	// Keep the daScript file access alive: Program/debug info may reference it.
	das::FileAccessPtr compiled_access;
	String compile_log;
	String compile_error;
	Array compile_errors;

	static HashSet<DAScript *> live_scripts;
#endif

	void scan_discovered_methods();

#if DASCRIPT_HAS_DASLANG
	Error compile_source(bool p_keep_state);
#endif

protected:
	static void _bind_methods() {}

public:
	DAScript();
	~DAScript();

	bool is_valid() const { return valid; }

	bool _editor_can_reload_from_file() override { return true; }
	bool _can_instantiate() const override { return valid; }
	StringName _get_instance_base_type() const override { return base_type; }
	void *_instance_create(Object *p_for_object) const override;
	void *_placeholder_instance_create(Object *p_for_object) const override;
	bool _instance_has(Object *p_object) const override { return false; }
	bool _has_source_code() const override { return true; }
	String _get_source_code() const override { return source_code; }
	void _set_source_code(const String &p_code) override;
	void set_diagnostic_path(const String &p_path) { diagnostic_path = p_path; }
	Error _reload(bool p_keep_state) override;
	StringName _get_global_name() const override { return StringName(); }
	bool _inherits_script(const Ref<Script> &p_script) const override { return false; }
	StringName _get_doc_class_name() const override { return StringName(); }
	String _get_class_icon_path() const override { return String(); }
	bool _has_method(const StringName &p_method) const override;
	bool _has_static_method(const StringName &p_method) const override;
	bool _has_script_signal(const StringName &p_signal) const override { return false; }
	TypedArray<Dictionary> _get_script_signal_list() const override;
	bool _has_property_default_value(const StringName &p_property) const override { return false; }
	Variant _get_property_default_value(const StringName &p_property) const override { return Variant(); }
	Variant _get_script_method_argument_count(const StringName &p_method) const override;
	Dictionary _get_method_info(const StringName &p_method) const override;
	Ref<Script> _get_base_script() const override;
	void _update_exports() override;
	TypedArray<Dictionary> _get_documentation() const override;
	bool _is_tool() const override { return false; }
	bool _is_valid() const override { return valid; }
	bool _is_abstract() const override { return false; }
	ScriptLanguage *_get_language() const override;
	TypedArray<Dictionary> _get_script_method_list() const override;
	TypedArray<Dictionary> _get_script_property_list() const override;
	int32_t _get_member_line(const StringName &p_member) const override { return -1; }
	Dictionary _get_constants() const override { return Dictionary(); }
	TypedArray<StringName> _get_members() const override { return TypedArray<StringName>(); }
	bool _is_placeholder_fallback_enabled() const override { return true; }
	Variant _get_rpc_config() const override { return Variant(); }

#if DASCRIPT_HAS_DASLANG
	const das::ProgramPtr &get_compiled_program() const { return compiled_program; }
	const String &get_compile_log() const { return compile_log; }
	const String &get_compile_error() const { return compile_error; }
	const Array &get_compile_errors() const { return compile_errors; }
	Error compile_silent();
	Error compile_for_tools();
	static const HashSet<DAScript *> &get_live_scripts() { return live_scripts; }
#endif

	// Helpers
	const HashSet<StringName> &get_discovered_methods() const { return discovered_methods; }
};

} // namespace godot
