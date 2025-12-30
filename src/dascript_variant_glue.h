#pragma once

#include <godot_cpp/variant/variant.hpp>

namespace godot {

// Placeholder conversion helpers.
// When Daslang is integrated, these functions should convert between Godot Variant
// and daScript values (e.g. vec2/vec3, strings, objects).

struct DasValueStub {
	// Temporary stand-in until daScript types are available.
	Variant as_variant;
};

DasValueStub godot_variant_to_das_stub(const Variant &p_value);
Variant das_stub_to_godot_variant(const DasValueStub &p_value);

} // namespace godot
