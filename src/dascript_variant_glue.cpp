#include "dascript_variant_glue.h"

namespace godot {

DasValueStub godot_variant_to_das_stub(const Variant &p_value) {
	DasValueStub out;
	out.as_variant = p_value;
	return out;
}

Variant das_stub_to_godot_variant(const DasValueStub &p_value) {
	return p_value.as_variant;
}

} // namespace godot
