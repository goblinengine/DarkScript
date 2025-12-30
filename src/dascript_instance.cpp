#include "dascript_instance.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <vector>

#include "dascript_script.h"
#include "dascript_language.h"

namespace godot {

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
	return script->_has_method(p_method);
}

void DAScriptInstance::call(const StringName &p_method, const Variant ** /*p_args*/, int /*p_argcount*/, Variant &r_ret, GDExtensionCallError &r_error) {
	r_ret = Variant();

	if (!has_method(p_method)) {
		r_error.error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
		r_error.argument = 0;
		r_error.expected = 0;
		return;
	}

	// Real Daslang execution is not yet wired in.
	// For now, log a message so users see what is happening.
	UtilityFunctions::push_warning(String("DAScript call stub: ") + String(p_method) + " (Daslang runtime not integrated yet)");

	r_error.error = GDEXTENSION_CALL_OK;
	r_error.argument = 0;
	r_error.expected = 0;
}

void DAScriptInstance::notification(int32_t /*p_what*/) {
	// Hook notifications later (e.g. READY/PROCESS), once runtime is integrated.
}

} // namespace godot
