#pragma once

#include <godot_cpp/variant/string_name.hpp>

#include <godot_cpp/godot.hpp>

#if DASCRIPT_HAS_DASLANG
namespace das {
	class Context;
	struct SimFunction;
}
#endif

namespace godot {

class Object;
class DAScript;
class DAScriptLanguage;

class DAScriptInstance {
	Object *owner = nullptr;
	DAScript *script = nullptr;

#if DASCRIPT_HAS_DASLANG
	das::Context *ctx = nullptr;
	bool ctx_ready = false;
	String last_runtime_error;
	bool runtime_error_reported = false;

	bool ensure_context();
	bool call_das_function(const StringName &p_method, const godot::Variant **p_args, int p_argcount, godot::Variant &r_ret, GDExtensionCallError &r_error);
#endif

public:
	DAScriptInstance(Object *p_owner, DAScript *p_script) : owner(p_owner), script(p_script) {}
	~DAScriptInstance();

	static void *create(Object *p_owner, DAScript *p_script);

	Object *get_owner() const { return owner; }
	DAScript *get_script() const { return script; }

	bool has_method(const StringName &p_method) const;
	void call(const StringName &p_method, const godot::Variant **p_args, int p_argcount, godot::Variant &r_ret, GDExtensionCallError &r_error);
	void notification(int32_t p_what);

	// Called by DAScript on successful recompile to force re-simulate with the new Program.
	void invalidate_context();
};

} // namespace godot
