/*************************************************************************/
/*  editor_scene_importer_blend.cpp                                      */
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
#include "editor_scene_importer_blend.h"

#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "core/string/print_string.h"
#include "editor/editor_settings.h"
#include "modules/gltf/editor_scene_importer_gltf.h"
#include "modules/gltf/gltf_document.h"

uint32_t EditorSceneFormatImporterBlend::get_import_flags() const {
	return ImportFlags::IMPORT_SCENE | ImportFlags::IMPORT_ANIMATION;
}

void EditorSceneFormatImporterBlend::get_extensions(
		List<String> *r_extensions) const {
	r_extensions->push_back("blend");
}

Node *EditorSceneFormatImporterBlend::import_scene(
		const String &p_path, uint32_t p_flags, const Map<StringName, Variant> &p_options,
		int p_bake_fps, List<String> *r_missing_deps, Error *r_err) {
	String addon_path = EditorSettings::get_singleton()->get_setting(
			"filesystem/import/blender/blender_path");
	String json_parameters =
			"{\"source\": \"GODOT_SOURCE\", \"sink\": \"GODOT_SINK\"}";
	String source_global =
			ProjectSettings::get_singleton()->globalize_path(p_path);
	source_global = source_global.c_escape();
	json_parameters = json_parameters.replace("GODOT_SOURCE", source_global);
	String sink = "res://.godot/imported/" + p_path.get_file().get_basename() +
			"-" + p_path.md5_text() + ".gltf";
	String sink_global = ProjectSettings::get_singleton()->globalize_path(sink);
	sink_global = sink_global.c_escape();
	json_parameters = json_parameters.replace("GODOT_SINK", sink_global);
	Ref<JSON> json;
	json.instantiate();
	Error err = json->parse(json_parameters);
	if (err != OK) {
		*r_err = err;
		ERR_PRINT(vformat("Blend config can't be read at line %s with error %s",
				json->get_error_line(), json->get_error_message()));
		return nullptr;
	}
	Dictionary parameters = json->get_data();
	String parameters_arg;
	if (p_options.has(SNAME("blender/nodes/custom_properties")) && p_options[SNAME("blender/nodes/custom_properties")]) {
		parameters_arg += "export_extras=True,";
	} else {
		parameters_arg += "export_extras=False,";
	}

	if (p_options.has(SNAME("blender/meshes/skins")) && p_options[SNAME("blender/meshes/skins")]) {
		int32_t skins = p_options["blender/meshes/skins"];
		if (skins == BLEND_BONE_INFLUENCES_NONE) {
			parameters_arg += "export_all_influences=False,";
			parameters_arg += "export_skins=True,";
		} else if (skins == BLEND_BONE_INFLUENCES_COMPATIBLE) {
			parameters_arg += "export_all_influences=False,";
			parameters_arg += "export_skins=True,";
		} else if (skins == BLEND_BONE_INFLUENCES_ALL) {
			parameters_arg += "export_all_influences=True,";
			parameters_arg += "export_skins=True,";
		}
	} else {
		parameters_arg += "export_skins=False,";
	}
	if (p_options.has(SNAME("blender/nodes/cameras")) && p_options[SNAME("blender/nodes/cameras")]) {
		parameters_arg += "export_cameras=True,";
	} else {
		parameters_arg += "export_cameras=False,";
	}
	if (p_options.has(SNAME("blender/nodes/lights")) && p_options[SNAME("blender/nodes/lights")]) {
		parameters_arg += "export_lights=True,";
	} else {
		parameters_arg += "export_lights=False,";
	}
	if (p_options.has(SNAME("blender/meshes/colors")) && p_options[SNAME("blender/meshes/colors")]) {
		parameters_arg += "export_colors=True,";
	} else {
		parameters_arg += "export_colors=False,";
	}
	if (p_options.has(SNAME("blender/nodes/visible")) && p_options[SNAME("blender/nodes/visible")]) {
		int32_t visible = p_options["blender/nodes/visible"];
		if (visible == BLEND_VISIBLE_VISIBLE_ONLY) {
			parameters_arg += "use_visible=True,";
		} else if (visible == BLEND_VISIBLE_RENDERABLE) {
			parameters_arg += "use_renderable=True,";
		} else if (visible == BLEND_VISIBLE_ALL) {
			parameters_arg += "use_visible=False,use_renderable=False,";
		}
	} else {
		parameters_arg += "use_visible=False,use_renderable=False,";
	}
	if (p_options.has(SNAME("blender/meshes/uvs")) && p_options[SNAME("blender/meshes/uvs")]) {
		parameters_arg += "export_texcoords=True,";
	} else {
		parameters_arg += "export_texcoords=False,";
	}
	if (p_options.has(SNAME("blender/meshes/normals")) && p_options[SNAME("blender/meshes/normals")]) {
		parameters_arg += "export_normals=True,";
	} else {
		parameters_arg += "export_normals=False,";
	}
	if (p_options.has(SNAME("blender/meshes/tangents")) && p_options[SNAME("blender/meshes/tangents")]) {
		parameters_arg += "export_tangents=True,";
	} else {
		parameters_arg += "export_tangents=False,";
	}
	if (p_options.has(SNAME("blender/animation/group_tracks")) && p_options[SNAME("blender/animation/group_tracks")]) {
		parameters_arg += "export_nla_strips=True,";
	} else {
		parameters_arg += "export_nla_strips=False,";
	}
	if (p_options.has(SNAME("blender/animation/limit_playback")) && p_options[SNAME("blender/animation/limit_playback")]) {
		parameters_arg += "export_frame_range=True,";
	} else {
		parameters_arg += "export_frame_range=False,";
	}
	if (p_options.has(SNAME("blender/animation/always_sample")) && p_options[SNAME("blender/animation/always_sample")]) {
		parameters_arg += "export_force_sampling=True,";
	} else {
		parameters_arg += "export_force_sampling=False,";
	}
	if (p_options.has(SNAME("blender/meshes/export_bones_deforming_mesh_only")) && p_options[SNAME("blender/meshes/export_bones_deforming_mesh_only")]) {
		parameters_arg += "export_def_bones=True,";
	} else {
		parameters_arg += "export_def_bones=False,";
	}
	if (p_options.has(SNAME("blender/nodes/modifiers")) && p_options[SNAME("blender/nodes/modifiers")]) {
		parameters_arg += "export_apply=True";
	} else {
		parameters_arg += "export_apply=False";
	}

	String unpack_all;
	if (p_options.has(SNAME("blender/materials/unpack_enabled")) && p_options[SNAME("blender/materials/unpack_enabled")]) {
		unpack_all = "bpy.ops.file.unpack_all(method='USE_LOCAL');";
	}
	String common_args = vformat("filepath='%s',", parameters["sink"]) +
			"export_format='GLTF_SEPARATE',"
			"export_yup=True," +
			parameters_arg;
	String scene_to_blend_setting = "import bpy, os, sys;"
									"print('Blender is 3.0 or higher is required.', file=sys.stderr) if bpy.app.version < (3, 0, 0) else None;" +
			vformat("bpy.ops.wm.open_mainfile(filepath='%s');", parameters["source"]) +
			unpack_all +
			"bpy.ops.export_scene.gltf(export_keep_originals=True," + common_args + ");";
	String script = scene_to_blend_setting;
	List<String> args;
	args.push_back("--background");
	args.push_back("--python-expr");
	args.push_back(script);
	print_verbose(script);
	String standard_out;
	String standard_err;
	int32_t ret = OS::get_singleton()->execute(addon_path, args,
			&standard_out, &ret, true);
	print_verbose(standard_out);
	if (ret != OK) {
		*r_err = ERR_SCRIPT_FAILED;
		print_error("Blender returned " + itos(ret));
		return nullptr;
	}
	// Used GLTFDocument instead of gltf importer to keep image references
	Ref<EditorSceneFormatImporterGLTF> gltf;
	gltf.instantiate();
	List<String> deps;
	Node *root = gltf->import_scene(sink, p_flags, p_options, p_bake_fps, &deps, &err);
	if (err != OK) {
		if (r_err) {
			*r_err = FAILED;
		}
		return nullptr;
	}
	return root;
}

Ref<Animation> EditorSceneFormatImporterBlend::import_animation(
		const String &p_path, uint32_t p_flags, const Map<StringName, Variant> &p_options,
		int p_bake_fps) {
	return Ref<Animation>();
}

Variant EditorSceneFormatImporterBlend::get_option_visibility(
		const String &p_path, const String &p_option,
		const Map<StringName, Variant> &p_options) {
	if (p_option.begins_with("animation/")) {
		if (p_option != "animation/import" &&
				!bool(p_options["animation/import"])) {
			return false;
		}
	}
	return true;
}

void EditorSceneFormatImporterBlend::get_import_options(
		const String &p_path, List<ResourceImporter::ImportOption> *r_options) {
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::INT, "blender/nodes/visible",
					PropertyHint::PROPERTY_HINT_ENUM,
					"Visible Only,Renderable,All"),
			BLEND_VISIBLE_ALL));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/nodes/punctual_lights")), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/nodes/cameras")), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/nodes/custom_properties")), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::INT, SNAME("blender/nodes/modifiers"),
					PropertyHint::PROPERTY_HINT_ENUM,
					"No Modifiers,All Modifiers"),
			BLEND_MODIFIERS_ALL));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/meshes/colors")), false));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/meshes/uvs")), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/meshes/normals")), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/meshes/tangents")), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::INT, SNAME("blender/meshes/skins"),
					PropertyHint::PROPERTY_HINT_ENUM,
					"None,4 Influences (Compatible),All Influences"),
			BLEND_BONE_INFLUENCES_ALL));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL,
					"blender/meshes/export_bones_deforming_mesh_only"),
			false));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/materials/unpack_enabled")), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/animation/limit_playback")), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/animation/always_sample")), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, SNAME("blender/animation/group_tracks")), true));
}

#endif // TOOLS_ENABLED
