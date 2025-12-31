#include "dascript_variant_glue.h"

namespace godot {

static DasVariant *alloc_variant(const Variant &p_value) {
	DasVariant *out = new DasVariant;
	out->value = p_value;
#if DASCRIPT_HAS_DASLANG
	out->addRef();
#endif
	return out;
}

DasVariant *godot_variant_to_das(const Variant &p_value) {
	return alloc_variant(p_value);
}

Variant das_to_godot_variant(const DasVariant *p_value) {
	return p_value ? p_value->value : Variant();
}

DasVariant *das_make_variant_int(int32_t p_value) {
	return alloc_variant(Variant(p_value));
}

DasVariant *das_make_variant_int64(int64_t p_value) {
	return alloc_variant(Variant(p_value));
}

DasVariant *das_make_variant_float(double p_value) {
	return alloc_variant(Variant(p_value));
}

DasVariant *das_make_variant_bool(bool p_value) {
	return alloc_variant(Variant(p_value));
}

DasVariant *das_make_variant_string(const char *p_value) {
	return alloc_variant(Variant(p_value ? String(p_value) : String("")));
}

void das_variant_release(DasVariant *p_value) {
	if (!p_value) {
		return;
	}
#if DASCRIPT_HAS_DASLANG
	// Balanced with addRef in alloc_variant.
	p_value->delRef();
#else
	delete p_value;
#endif
}

} // namespace godot
