#include "dascript_resource_loader.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "dascript_script.h"
#include "dascript_language.h"

namespace godot {

// DAScriptResourceFormatLoader

PackedStringArray DAScriptResourceFormatLoader::_get_recognized_extensions() const {
	PackedStringArray exts;
	exts.push_back("das");
	return exts;
}

bool DAScriptResourceFormatLoader::_handles_type(const StringName &p_type) const {
	return p_type == StringName("Script") || p_type == StringName("DAScript");
}

String DAScriptResourceFormatLoader::_get_resource_type(const String &p_path) const {
	String ext = p_path.get_extension().to_lower();
	if (ext == "das") {
		return "DAScript";
	}
	return "";
}

Variant DAScriptResourceFormatLoader::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {
	UtilityFunctions::print("DAScript: Loading file: ", p_path);
	
	Ref<DAScript> script;
	script.instantiate();
	
	if (!script.is_valid()) {
		UtilityFunctions::push_error("DAScript: Failed to instantiate script for: ", p_path);
		return Variant();
	}

	script->set_path(p_path);

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		UtilityFunctions::push_error("DAScript: Cannot open file: ", p_path);
		return Variant();
	}

	String source = f->get_as_text();
	f->close();
	
	UtilityFunctions::print("DAScript: File loaded, source length: ", source.length());

	script->set_source_code(source);
	
	UtilityFunctions::print("DAScript: Calling reload...");
	Error err = script->_reload(false);
	if (err != OK) {
		UtilityFunctions::push_error("DAScript: Reload failed with error: ", err);
		// Return the script anyway so it can be edited
	}
	
	UtilityFunctions::print("DAScript: Load complete");

	return script;
}

// DAScriptResourceFormatSaver

PackedStringArray DAScriptResourceFormatSaver::_get_recognized_extensions(const Ref<Resource> &p_resource) const {
	Ref<DAScript> script = p_resource;
	if (script.is_valid()) {
		PackedStringArray exts;
		exts.push_back("das");
		return exts;
	}
	return PackedStringArray();
}

bool DAScriptResourceFormatSaver::_recognize(const Ref<Resource> &p_resource) const {
	Ref<DAScript> script = p_resource;
	return script.is_valid();
}

Error DAScriptResourceFormatSaver::_save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<DAScript> script = p_resource;
	if (script.is_null()) {
		return ERR_INVALID_PARAMETER;
	}
	if (p_path.is_empty()) {
		UtilityFunctions::push_error("Cannot save DAScript: empty path");
		return ERR_INVALID_PARAMETER;
	}

	String source = script->_get_source_code();

	// Ensure parent folder exists; Script Create dialog does not always create directories.
	String dir = p_path.get_base_dir();
	if (!dir.is_empty() && ProjectSettings::get_singleton()) {
		String os_dir = ProjectSettings::get_singleton()->globalize_path(dir);
		Error mk = DirAccess::make_dir_recursive_absolute(os_dir);
		if (mk != OK && mk != ERR_ALREADY_EXISTS) {
			UtilityFunctions::push_error("Cannot create directory for DAScript: ", dir);
			return mk;
		}
	}

	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
	if (file.is_null()) {
		UtilityFunctions::push_error("Cannot save DAScript file: ", p_path);
		if (ProjectSettings::get_singleton()) {
			UtilityFunctions::push_error("Globalized path: ", ProjectSettings::get_singleton()->globalize_path(p_path));
		}
		return ERR_FILE_CANT_WRITE;
	}

	file->store_string(source);
	file->close();

	return OK;
}

} // namespace godot
