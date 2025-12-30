#pragma once

#include <godot_cpp/core/class_db.hpp>

namespace godot {

void initialize_dascript_module(ModuleInitializationLevel p_level);
void uninitialize_dascript_module(ModuleInitializationLevel p_level);

} // namespace godot
