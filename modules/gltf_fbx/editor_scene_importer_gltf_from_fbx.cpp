/*************************************************************************/
/*  editor_scene_importer_gltf_from_fbx.cpp                              */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#if TOOLS_ENABLED
#include "editor_scene_importer_gltf_from_fbx.h"

#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "core/string/print_string.h"
#include "editor/editor_settings.h"
#include "modules/gltf/gltf_document.h"

uint32_t EditorSceneFormatImporterGLTFFromFBX::get_import_flags() const {
	return ImportFlags::IMPORT_SCENE | ImportFlags::IMPORT_ANIMATION;
}

void EditorSceneFormatImporterGLTFFromFBX::get_extensions(
		List<String> *r_extensions) const {
	r_extensions->push_back("fbx");
}

Node *EditorSceneFormatImporterGLTFFromFBX::import_scene(
		const String &p_path, uint32_t p_flags, const Map<StringName, Variant> &p_options,
		int p_bake_fps, List<String> *r_missing_deps, Error *r_err) {
	String addon_path = EditorSettings::get_singleton()->get_setting(
			"filesystem/import/gltf_from_fbx/fbx2gltf_path");
	String json_parameters =
			"{\"source\": \"GODOT_SOURCE\", \"sink\": \"GODOT_SINK\"}";
	String sink = "res://.godot/imported/" + p_path.get_file().get_basename() +
			"-" + p_path.md5_text() + ".gltf";
	{
		String source_global =
				ProjectSettings::get_singleton()->globalize_path(p_path);
		json_parameters = json_parameters.replace("GODOT_SOURCE", source_global);
		String sink_global = ProjectSettings::get_singleton()->globalize_path(sink).get_basename();
		json_parameters = json_parameters.replace("GODOT_SINK", sink_global);
	}
	Ref<JSON> json;
	json.instantiate();
	Error err = json->parse(json_parameters);
	if (err != OK) {
		*r_err = err;
		ERR_PRINT(vformat("FBX2glTF config can't be read at line %s with error %s",
				json->get_error_line(), json->get_error_message()));
		return nullptr;
	}
	Dictionary parameters = json->get_data();
	String parameters_arg;
	String standard_out;
	String standard_err;
	List<String> args;
	args.push_back("--pbr-metallic-roughness");
	args.push_back("--input");
	args.push_back(parameters["source"]);
	args.push_back("--output");
	args.push_back(parameters["sink"]);
	args.push_back("--binary");

	int32_t ret = OS::get_singleton()->execute(addon_path, args,
			&standard_out, &ret, true);
	print_verbose(addon_path);
	print_verbose(standard_out);
	if (ret != OK) {
		*r_err = ERR_SCRIPT_FAILED;
		print_error("FBX2glTF returned " + itos(ret));
		return nullptr;
	}
	// Used GLTFDocument instead of gltf importer to keep image references
	Ref<GLTFDocument> gltf;
	gltf.instantiate();
	List<String> deps;
	Ref<GLTFState> state;
	state.instantiate();
	String gltf_path = sink.get_basename() + ".glb";
	print_verbose(vformat("gltf path: %s", gltf_path));
	err = gltf->append_from_file(gltf_path, state, p_flags, p_bake_fps);
	if (err != OK) {
		if (r_err) {
			*r_err = FAILED;
		}
		return nullptr;
	}
	return gltf->generate_scene(state, p_bake_fps);
}

Ref<Animation> EditorSceneFormatImporterGLTFFromFBX::import_animation(
		const String &p_path, uint32_t p_flags, const Map<StringName, Variant> &p_options,
		int p_bake_fps) {
	return Ref<Animation>();
}

Variant EditorSceneFormatImporterGLTFFromFBX::get_option_visibility(
		const String &p_path, const String &p_option,
		const Map<StringName, Variant> &p_options) {
	return true;
}

void EditorSceneFormatImporterGLTFFromFBX::get_import_options(
		const String &p_path, List<ResourceImporter::ImportOption> *r_options) {
}

#endif // TOOLS_ENABLED
