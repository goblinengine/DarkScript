#pragma once

#include <godot_cpp/variant/string_name.hpp>

#include <godot_cpp/godot.hpp>

namespace godot {

class Object;
class DAScript;
class DAScriptLanguage;

class DAScriptInstance {
	Object *owner = nullptr;
	DAScript *script = nullptr;

public:
	DAScriptInstance(Object *p_owner, DAScript *p_script) : owner(p_owner), script(p_script) {}

	static void *create(Object *p_owner, DAScript *p_script);

	Object *get_owner() const { return owner; }
	DAScript *get_script() const { return script; }

	bool has_method(const StringName &p_method) const;
	void call(const StringName &p_method, const godot::Variant **p_args, int p_argcount, godot::Variant &r_ret, GDExtensionCallError &r_error);
	void notification(int32_t p_what);
};

} // namespace godot
