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
	String path;
	StringName base_type = "Node";
	HashSet<StringName> discovered_methods;
	bool valid = true;

#if DASCRIPT_HAS_DASLANG
	das::ProgramPtr compiled_program;
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
	Error _reload(bool p_keep_state) override;
	bool _has_method(const StringName &p_method) const override;
	bool _is_tool() const override { return false; }
	bool _is_valid() const override { return valid; }
	ScriptLanguage *_get_language() const override;
	TypedArray<Dictionary> _get_script_method_list() const override;
	TypedArray<Dictionary> _get_script_property_list() const override;

#if DASCRIPT_HAS_DASLANG
	const das::ProgramPtr &get_compiled_program() const { return compiled_program; }
	const String &get_compile_log() const { return compile_log; }
	const String &get_compile_error() const { return compile_error; }
	const Array &get_compile_errors() const { return compile_errors; }
	static const HashSet<DAScript *> &get_live_scripts() { return live_scripts; }
#endif

	// Helpers
	const HashSet<StringName> &get_discovered_methods() const { return discovered_methods; }
};

} // namespace godot
