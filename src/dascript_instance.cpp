#include "dascript_instance.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <vector>

#include "dascript_script.h"
#include "dascript_language.h"

#if DASCRIPT_HAS_DASLANG
#include <daScript/daScript.h>
#include <daScript/simulate/cast.h>
#include <daScript/simulate/simulate.h>
#include <daScript/simulate/debug_info.h>
#endif
namespace godot {

#if DASCRIPT_HAS_DASLANG
DAScriptInstance::~DAScriptInstance() {
	if (ctx) {
		delete ctx;
		ctx = nullptr;
	}
}

bool DAScriptInstance::ensure_context() {
	if (ctx_ready) {
		return true;
	}
	last_runtime_error = "";
	runtime_error_reported = false;

	if (!script || !script->is_valid()) {
		last_runtime_error = "Script is not valid.";
		return false;
	}
	auto program = script->get_compiled_program();
	if (!program) {
		// In the editor, scripts are often partially typed and not yet compilable.
		// Never treat this as a runtime error in editor context.
		if (!(Engine::get_singleton() && Engine::get_singleton()->is_editor_hint())) {
			last_runtime_error = "Script has no compiled program (reload/compile failed).";
		}
		return false;
	}
	if (!ctx) {
		ctx = new das::Context((uint32_t)program->getContextStackSize());
	}
	das::TextWriter logs;
	if (!program->simulate(*ctx, logs)) {
		last_runtime_error = String("daScript simulate failed: ") + String(logs.str().c_str());
		return false;
	}
	ctx->runInitScript();
	ctx_ready = true;
	return true;
}

static bool variant_to_vec4f(const Variant &v, vec4f &out) {
	using namespace das;
	switch (v.get_type()) {
		case Variant::Type::NIL:
			out = cast<void *>::from(nullptr);
			return true;
		case Variant::Type::BOOL:
			out = cast<bool>::from((bool)v);
			return true;
		case Variant::Type::INT:
			out = cast<int64_t>::from((int64_t)v);
			return true;
		case Variant::Type::FLOAT:
			out = cast<double>::from((double)v);
			return true;
		case Variant::Type::STRING: {
			static thread_local std::string tmp;
			tmp = String(v).utf8().get_data();
			out = cast<char *>::from((char *)tmp.c_str());
			return true;
		}
		case Variant::Type::OBJECT: {
			Object *obj = Object::cast_to<Object>(v);
			out = cast<void *>::from((void *)obj);
			return true;
		}
		default:
			return false;
	}
}

static Variant vec4f_to_variant(vec4f v, das::TypeInfo *type_info) {
	using namespace das;
	if (!type_info) {
		return Variant();
	}
	switch (type_info->type) {
		case Type::tVoid:
			return Variant();
		case Type::tBool:
			return Variant(cast<bool>::to(v));
		case Type::tInt:
			return Variant((int64_t)cast<int32_t>::to(v));
		case Type::tInt64:
			return Variant((int64_t)cast<int64_t>::to(v));
		case Type::tUInt:
			return Variant((int64_t)cast<uint32_t>::to(v));
		case Type::tUInt64:
			return Variant((int64_t)cast<uint64_t>::to(v));
		case Type::tFloat:
			return Variant((double)cast<float>::to(v));
		case Type::tDouble:
			return Variant((double)cast<double>::to(v));
		case Type::tString: {
			char *s = cast<char *>::to(v);
			return Variant(String(s ? s : ""));
		}
		case Type::tPointer: {
			void *p = cast<void *>::to(v);
			return Variant((uint64_t)(uintptr_t)p);
		}
		default:
			return Variant();
	}
}

bool DAScriptInstance::call_das_function(const StringName &p_method, const godot::Variant **p_args, int p_argcount, godot::Variant &r_ret, GDExtensionCallError &r_error) {
	using namespace das;
	r_error.error = GDEXTENSION_CALL_OK;
	r_ret = Variant();

	if (!ensure_context()) {
		r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
		return false;
	}

	std::string method = String(p_method).utf8().get_data();
	SimFunction *fn = ctx->findFunction(method.c_str());
	if (!fn) {
		r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
		return false;
	}

	const uint32_t expected = fn->debugInfo ? fn->debugInfo->count : 0;
	const uint32_t provided = (uint32_t)p_argcount;
	const bool can_inject_self = (expected == provided + 1);

	if (expected != provided && !can_inject_self) {
		r_error.error = GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS;
		if (provided < expected) {
			r_error.error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
		}
		r_error.argument = (int32_t)expected;
		return false;
	}

	vec4f *args = nullptr;
	const uint32_t final_count = can_inject_self ? expected : provided;
	if (final_count > 0) {
		args = (vec4f *)alloca(sizeof(vec4f) * final_count);
		uint32_t offset = 0;
		if (can_inject_self) {
			args[0] = cast<void *>::from((void *)owner);
			offset = 1;
		}
		for (uint32_t i = 0; i < provided; i++) {
			if (!variant_to_vec4f(*p_args[i], args[i + offset])) {
				r_error.error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
				r_error.argument = (int32_t)i;
				return false;
			}
		}
	}

	vec4f res = ctx->evalWithCatch(fn, args);
	if (ctx->getException()) {
		last_runtime_error = String(ctx->getException());
		r_error.error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
		return false;
	}

	TypeInfo *rtype = fn->debugInfo ? fn->debugInfo->result : nullptr;
	r_ret = vec4f_to_variant(res, rtype);
	return true;
}
#else
DAScriptInstance::~DAScriptInstance() = default;
#endif

static GDExtensionBool dasi_set(GDExtensionScriptInstanceDataPtr /*p_instance*/, GDExtensionConstStringNamePtr /*p_name*/, GDExtensionConstVariantPtr /*p_value*/) {
	return false;
}

static GDExtensionBool dasi_get(GDExtensionScriptInstanceDataPtr /*p_instance*/, GDExtensionConstStringNamePtr /*p_name*/, GDExtensionVariantPtr /*r_ret*/) {
	return false;
}

static const GDExtensionPropertyInfo *dasi_get_property_list(GDExtensionScriptInstanceDataPtr /*p_instance*/, uint32_t *r_count) {
	*r_count = 0;
	return nullptr;
}

static void dasi_free_property_list(GDExtensionScriptInstanceDataPtr /*p_instance*/, const GDExtensionPropertyInfo * /*p_list*/, uint32_t /*p_count*/) {
}

static GDExtensionObjectPtr dasi_get_owner(GDExtensionScriptInstanceDataPtr p_instance) {
	auto *inst = reinterpret_cast<DAScriptInstance *>(p_instance);
	return inst->get_owner()->_owner;
}

static void dasi_get_property_state(GDExtensionScriptInstanceDataPtr /*p_instance*/, GDExtensionScriptInstancePropertyStateAdd /*p_add_func*/, void * /*p_userdata*/) {
}

static const GDExtensionMethodInfo *dasi_get_method_list(GDExtensionScriptInstanceDataPtr /*p_instance*/, uint32_t *r_count) {
	*r_count = 0;
	return nullptr;
}

static void dasi_free_method_list(GDExtensionScriptInstanceDataPtr /*p_instance*/, const GDExtensionMethodInfo * /*p_list*/, uint32_t /*p_count*/) {
}

static GDExtensionVariantType dasi_get_property_type(GDExtensionScriptInstanceDataPtr /*p_instance*/, GDExtensionConstStringNamePtr /*p_name*/, GDExtensionBool *r_is_valid) {
	*r_is_valid = false;
	return GDEXTENSION_VARIANT_TYPE_NIL;
}

static GDExtensionBool dasi_validate_property(GDExtensionScriptInstanceDataPtr /*p_instance*/, GDExtensionPropertyInfo * /*p_property*/) {
	return true;
}

static GDExtensionBool dasi_has_method(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name) {
	auto *inst = reinterpret_cast<DAScriptInstance *>(p_instance);
	const StringName &name = *reinterpret_cast<const StringName *>(p_name);
	return inst->has_method(name);
}

static GDExtensionInt dasi_get_method_argument_count(GDExtensionScriptInstanceDataPtr /*p_instance*/, GDExtensionConstStringNamePtr /*p_name*/, GDExtensionBool *r_is_valid) {
	*r_is_valid = false;
	return 0;
}

static void dasi_call(GDExtensionScriptInstanceDataPtr p_self, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_argument_count, GDExtensionVariantPtr r_return, GDExtensionCallError *r_error) {
	auto *inst = reinterpret_cast<DAScriptInstance *>(p_self);
	const StringName &method = *reinterpret_cast<const StringName *>(p_method);

	Variant ret;
	GDExtensionCallError err{};

	// Convert args.
	std::vector<const Variant *> args;
	args.resize(p_argument_count);
	for (int i = 0; i < p_argument_count; i++) {
		args[i] = reinterpret_cast<const Variant *>(p_args[i]);
	}

	inst->call(method, args.data(), p_argument_count, ret, err);

	if (r_error) {
		*r_error = err;
	}

	if (r_return) {
		*reinterpret_cast<Variant *>(r_return) = ret;
	}
}

static void dasi_notification(GDExtensionScriptInstanceDataPtr p_instance, int32_t p_what, GDExtensionBool /*p_reversed*/) {
	auto *inst = reinterpret_cast<DAScriptInstance *>(p_instance);
	inst->notification(p_what);
}

static void dasi_to_string(GDExtensionScriptInstanceDataPtr /*p_instance*/, GDExtensionBool *r_is_valid, GDExtensionStringPtr r_out) {
	*r_is_valid = true;
	String s = "<DAScriptInstance>";
	*reinterpret_cast<String *>(r_out) = s;
}

static void dasi_refcount_incremented(GDExtensionScriptInstanceDataPtr /*p_instance*/) {
}

static GDExtensionBool dasi_refcount_decremented(GDExtensionScriptInstanceDataPtr /*p_instance*/) {
	return true;
}

static GDExtensionObjectPtr dasi_get_script(GDExtensionScriptInstanceDataPtr p_instance) {
	auto *inst = reinterpret_cast<DAScriptInstance *>(p_instance);
	return inst->get_script()->_owner;
}

static GDExtensionBool dasi_is_placeholder(GDExtensionScriptInstanceDataPtr /*p_instance*/) {
	return false;
}

static GDExtensionScriptLanguagePtr dasi_get_language(GDExtensionScriptInstanceDataPtr /*p_instance*/) {
	return DAScriptLanguage::get_singleton()->_owner;
}

static void dasi_free(GDExtensionScriptInstanceDataPtr p_instance) {
	auto *inst = reinterpret_cast<DAScriptInstance *>(p_instance);
	memdelete(inst);
}

void *DAScriptInstance::create(Object *p_owner, DAScript *p_script) {
	static GDExtensionScriptInstanceInfo3 info = {
		/*set_func*/ &dasi_set,
		/*get_func*/ &dasi_get,
		/*get_property_list_func*/ &dasi_get_property_list,
		/*free_property_list_func*/ &dasi_free_property_list,
		/*get_class_category_func*/ nullptr,
		/*property_can_revert_func*/ nullptr,
		/*property_get_revert_func*/ nullptr,
		/*get_owner_func*/ &dasi_get_owner,
		/*get_property_state_func*/ &dasi_get_property_state,
		/*get_method_list_func*/ &dasi_get_method_list,
		/*free_method_list_func*/ &dasi_free_method_list,
		/*get_property_type_func*/ &dasi_get_property_type,
		/*validate_property_func*/ &dasi_validate_property,
		/*has_method_func*/ &dasi_has_method,
		/*get_method_argument_count_func*/ &dasi_get_method_argument_count,
		/*call_func*/ &dasi_call,
		/*notification_func*/ &dasi_notification,
		/*to_string_func*/ &dasi_to_string,
		/*refcount_incremented_func*/ &dasi_refcount_incremented,
		/*refcount_decremented_func*/ &dasi_refcount_decremented,
		/*get_script_func*/ &dasi_get_script,
		/*is_placeholder_func*/ &dasi_is_placeholder,
		/*set_fallback_func*/ nullptr,
		/*get_fallback_func*/ nullptr,
		/*get_language_func*/ &dasi_get_language,
		/*free_func*/ &dasi_free,
	};

	DAScriptInstance *inst = memnew(DAScriptInstance(p_owner, p_script));
	return internal::gdextension_interface_script_instance_create3(&info, reinterpret_cast<GDExtensionScriptInstanceDataPtr>(inst));
}

bool DAScriptInstance::has_method(const StringName &p_method) const {
	if (!script) {
		return false;
	}
	// Be permissive: allow engine to attempt standard callbacks even if we
	// didn't discover them (we'll validate at runtime).
	if (p_method == StringName("_ready") || p_method == StringName("_process") || p_method == StringName("_physics_process") || p_method == StringName("_notification")) {
		return true;
	}
	return script->get_discovered_methods().has(p_method);
}

void DAScriptInstance::call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant &r_ret, GDExtensionCallError &r_error) {
	#if DASCRIPT_HAS_DASLANG
	// Never spam errors while the editor is just probing the script.
	const bool editor_hint = (Engine::get_singleton() && Engine::get_singleton()->is_editor_hint());
	if (call_das_function(p_method, p_args, p_argcount, r_ret, r_error)) {
		return;
	}
	// If execution failed, surface a useful error once (outside editor).
	if (!editor_hint && !last_runtime_error.is_empty() && !runtime_error_reported) {
		runtime_error_reported = true;
		UtilityFunctions::push_error(String("[DAScript] runtime error: ") + last_runtime_error);
	}
	#else
	// Stub: no runtime execution yet.
	r_ret = Variant();
	r_error.error = GDEXTENSION_CALL_OK;
	r_error.argument = -1;
	r_error.expected = 0;
	UtilityFunctions::print(String("[DAScript] call stub: ") + String(p_method));
	#endif
}

void DAScriptInstance::notification(int32_t p_what) {
	#if DASCRIPT_HAS_DASLANG
	// Forward to optional script callbacks.
	// - _notification(what) or _notification(self, what)
	// - _ready() or _ready(self)
	{
		Variant ret;
		GDExtensionCallError err;
		Variant what_arg = p_what;
		const Variant *args1[1] = { &what_arg };
		call(StringName("_notification"), args1, 1, ret, err);
	}
	if (p_what == Node::NOTIFICATION_READY) {
		Variant ret;
		GDExtensionCallError err;
		call(StringName("_ready"), nullptr, 0, ret, err);
	}
	#endif
}

} // namespace godot
