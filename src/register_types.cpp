#include "register_types.h"

#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "dascript_language.h"
#include "dascript_script.h"
#include "dascript_resource_loader.h"

#if DASCRIPT_HAS_DASLANG
#include <daScript/daScriptC.h>
#endif

namespace godot {

static DAScriptLanguage *s_language = nullptr;
static Ref<DAScriptResourceFormatLoader> s_resource_loader;
static Ref<DAScriptResourceFormatSaver> s_resource_saver;

void initialize_dascript_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		#if DASCRIPT_HAS_DASLANG
		das_initialize();
		#endif

		// Must be registered so Godot can bind ScriptLanguageExtension virtuals.
		ClassDB::register_class<DAScriptLanguage>();
		ClassDB::register_class<DAScript>();

		s_language = memnew(DAScriptLanguage);
		DAScriptLanguage::set_language_singleton(s_language);

		Engine::get_singleton()->register_script_language(s_language);
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		ClassDB::register_class<DAScriptResourceFormatLoader>();
		ClassDB::register_class<DAScriptResourceFormatSaver>();

		s_resource_loader.instantiate();
		UtilityFunctions::print("DAScript: Resource loader instantiated: ", s_resource_loader.is_valid());
		if (ResourceLoader::get_singleton()) {
			ResourceLoader::get_singleton()->add_resource_format_loader(s_resource_loader);
			UtilityFunctions::print("DAScript: Resource loader registered");
		} else {
			UtilityFunctions::print("DAScript: ERROR - ResourceLoader singleton is null!");
		}

		s_resource_saver.instantiate();
		UtilityFunctions::print("DAScript: Resource saver instantiated: ", s_resource_saver.is_valid());
		if (ResourceSaver::get_singleton()) {
			ResourceSaver::get_singleton()->add_resource_format_saver(s_resource_saver);
			UtilityFunctions::print("DAScript: Resource saver registered");
		} else {
			UtilityFunctions::print("DAScript: ERROR - ResourceSaver singleton is null!");
		}
	}
}

void uninitialize_dascript_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		if (s_resource_loader.is_valid() && ResourceLoader::get_singleton()) {
			ResourceLoader::get_singleton()->remove_resource_format_loader(s_resource_loader);
		}
		s_resource_loader.unref();

		if (s_resource_saver.is_valid() && ResourceSaver::get_singleton()) {
			ResourceSaver::get_singleton()->remove_resource_format_saver(s_resource_saver);
		}
		s_resource_saver.unref();
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		if (s_language) {
			Engine::get_singleton()->unregister_script_language(s_language);
			memdelete(s_language);
			s_language = nullptr;
		}
		DAScriptLanguage::set_language_singleton(nullptr);

		#if DASCRIPT_HAS_DASLANG
		das_shutdown();
		#endif
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
