#pragma once

#include <godot_cpp/variant/variant.hpp>

#if DASCRIPT_HAS_DASLANG
#include <daScript/daScript.h>
#include <daScript/misc/smart_ptr.h>
#endif

namespace godot {

// Minimal wrapper so daScript can pass Godot Variant values around.
// We keep it simple for now: store Variant by value.
struct DasVariant
#if DASCRIPT_HAS_DASLANG
		: public das::ptr_ref_count
#endif
{
	Variant value;
};

// Conversions between Godot Variant and DasVariant
DasVariant *godot_variant_to_das(const Variant &p_value);
Variant das_to_godot_variant(const DasVariant *p_value);

// Constructors from common primitive types (keep it small and safe)
DasVariant *das_make_variant_int(int32_t p_value);
DasVariant *das_make_variant_int64(int64_t p_value);
DasVariant *das_make_variant_float(double p_value);
DasVariant *das_make_variant_bool(bool p_value);
DasVariant *das_make_variant_string(const char *p_value);

void das_variant_release(DasVariant *p_value);

} // namespace godot
