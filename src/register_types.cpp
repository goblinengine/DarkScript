#include "register_types.h"

#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "dascript_language.h"
#include "dascript_script.h"

namespace godot {

static DAScriptLanguage *s_language = nullptr;

void initialize_dascript_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		ClassDB::register_class<DAScriptLanguage>();
		ClassDB::register_class<DAScript>();

		s_language = memnew(DAScriptLanguage);
		DAScriptLanguage::set_language_singleton(s_language);

		Engine::get_singleton()->register_script_language(s_language);
	}
}

void uninitialize_dascript_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		if (s_language) {
			Engine::get_singleton()->unregister_script_language(s_language);
			memdelete(s_language);
			s_language = nullptr;
		}
		DAScriptLanguage::set_language_singleton(nullptr);
	}
}

} // namespace godot

extern "C" {
uint32_t GDE_EXPORT dascript_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
		GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization *p_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, p_initialization);
	init_obj.register_initializer(godot::initialize_dascript_module);
	init_obj.register_terminator(godot::uninitialize_dascript_module);
	init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SERVERS);
	return init_obj.init();
}
}
