#include "dascript_script.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "dascript_language.h"
#include "dascript_instance.h"

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

void DAScript::_set_source_code(const String &p_code) {
	source_code = p_code;
	scan_discovered_methods();
}

Error DAScript::_reload(bool /*p_keep_state*/) {
	// Until Daslang is integrated, reload just re-scans the source.
	valid = true;
	scan_discovered_methods();
	return OK;
}

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
