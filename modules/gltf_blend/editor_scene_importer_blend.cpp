/*************************************************************************/
/*  editor_scene_importer_blend.cpp                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "core/error/error_list.h"
#include "core/error/error_macros.h"
#include "core/io/json.h"
#include "core/variant/dictionary.h"
#if TOOLS_ENABLED
#include "editor_scene_importer_blend.h"

#include "core/core_bind.h"
#include "core/object/ref_counted.h"
#include "modules/gltf/editor_scene_importer_gltf.h"
#include "scene/3d/node_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/animation.h"

uint32_t EditorSceneFormatImporterBlend::get_import_flags() const {
	return ImportFlags::IMPORT_SCENE | ImportFlags::IMPORT_ANIMATION;
}

void EditorSceneFormatImporterBlend::get_extensions(
		List<String> *r_extensions) const {
	r_extensions->push_back("blend");
}

Node *EditorSceneFormatImporterBlend::import_scene(
		const String &p_path, uint32_t p_flags, const Dictionary &p_options,
		int p_bake_fps, List<String> *r_missing_deps, Error *r_err) {
	String addon_path = EditorSettings::get_singleton()->get_setting(
			"filesystem/blender/blender_path");
	String json_parameters =
			"{\"source\": \"GODOT_SOURCE\", \"sink\": \"GODOT_SINK\"}";
	String source = ProjectSettings::get_singleton()->globalize_path(p_path);
	String source_global =
			ProjectSettings::get_singleton()->globalize_path(source);
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
	String extra_properties_arg = p_options["blender/nodes/custom_properties"] ? "export_extras=True," : "export_extras=False,";
	int32_t skins = p_options["blender/meshes/skins"];
	String influence_arg = "export_all_influences=True,";
	String skins_arg = "export_skins=True,";
	if (skins == BLEND_BONE_INFLUENCES_NONE) {
		influence_arg = "export_all_influences=False,";
		skins_arg = "export_skins=True,";
	} else if (skins == BLEND_BONE_INFLUENCES_COMPATIBLE) {
		influence_arg = "export_all_influences=False,";
		skins_arg = "export_skins=True,";
	} else if (skins == BLEND_BONE_INFLUENCES_ALL) {
		influence_arg = "export_all_influences=True,";
		skins_arg = "export_skins=True,";
	}
	String cameras_arg = p_options["blender/nodes/cameras"] ? "export_cameras=True," : "export_cameras=False,";
	String lights_last_arg = p_options["blender/nodes/lights"] ? "export_lights=True" : "export_lights=False";
	String colors_arg = p_options["blender/meshes/colors"] ? "export_colors=True," : "export_colors=False,";
	String visible_arg = "use_visible=False,use_renderable=False,";
	int32_t visible = p_options["blender/nodes/visible"];
	if (visible == BLEND_VISIBLE_VISIBLE_ONLY) {
		visible_arg = "use_visible=True,";
	} else if (visible == BLEND_VISIBLE_RENDERABLE) {
		visible_arg = "use_renderable=True,";
	}
	String uvs_arg = p_options["blender/meshes/uvs"] ? "export_texcoords=True," : "export_texcoords=False,";
	String normals_arg = p_options["blender/meshes/normals"] ? "export_normals=True," : "export_normals=False,";
	String tangents_arg = p_options["blender/meshes/tangents"] ? "export_tangents=True," : "export_tangents=False,";
	String animation_groups_arg = p_options["blender/animation/group_tracks"] ? "export_nla_strips=True," : "export_nla_strips=False,";
	String animation_limit_playback_arg = p_options["blender/animation/limit_playback"] ? "export_frame_range=True," : "export_frame_range=False,";
	String animation_always_sample_arg = p_options["blender/animation/always_sample"] ? "export_force_sampling=True," : "export_force_sampling=False,";
	String deform_bones_args = p_options["blender/meshes/export_bones_deforming_mesh_only"] ? "export_def_bones=True," : "export_def_bones=False,";
	String apply_modifications_arg = int32_t(p_options["blender/nodes/modifiers"]) == BLEND_MODIFIERS_ALL ? "export_apply=True," : "export_apply=False,";
	String common_args = vformat("filepath='%s',", parameters["sink"]) +
			"export_format='GLTF_SEPARATE',"
			"export_yup=True," +
			visible_arg +
			extra_properties_arg +
			apply_modifications_arg +
			deform_bones_args +
			colors_arg +
			uvs_arg +
			skins_arg +
			influence_arg +
			animation_always_sample_arg +
			animation_limit_playback_arg +
			animation_groups_arg +
			cameras_arg +
			lights_last_arg;
	String scene_to_blend_setting = "import bpy, os, sys;" + vformat("bpy.ops.wm.open_mainfile(filepath='%s');", parameters["source"]) +
			"(bpy.ops.export_scene.gltf(export_keep_originals=True," + common_args + ")) "
																					 "if bpy.app.version >= (3, 0, 0) else (bpy.ops.export_scene.gltf(" +
			common_args + "));";
	String script = scene_to_blend_setting;
	Vector<String> args;
	args.push_back("--background");
	args.push_back("--python-expr");
	args.push_back(script);
	print_line(script);
	Array standard_out;
	int32_t ret = core_bind::OS::get_singleton()->execute(addon_path, args,
			standard_out, true);
	for (int32_t line_i = 0; line_i < standard_out.size(); line_i++) {
		print_line(standard_out[line_i]);
	}
	if (ret != OK) {
		*r_err = ERR_SCRIPT_FAILED;
		print_error("Blender returned " + itos(ret));
		return nullptr;
	}
	return import_scene_from_other_importer(sink,
			p_flags, p_options, p_bake_fps);
}

Ref<Animation> EditorSceneFormatImporterBlend::import_animation(
		const String &p_path, uint32_t p_flags, const Dictionary &p_options,
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
			PropertyInfo(Variant::BOOL, "blender/nodes/punctual_lights"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blender/nodes/cameras"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blender/nodes/custom_properties"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::INT, "blender/nodes/modifiers",
					PropertyHint::PROPERTY_HINT_ENUM,
					"No Modifiers,All Modifiers"),
			BLEND_MODIFIERS_NONE));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blender/meshes/colors"), false));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blender/meshes/uvs"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blender/meshes/normals"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blender/meshes/tangents"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::INT, "blender/meshes/skins",
					PropertyHint::PROPERTY_HINT_ENUM,
					"None,4 Influences (Compatible),All Influences"),
			BLEND_BONE_INFLUENCES_ALL));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL,
					"blender/meshes/export_bones_deforming_mesh_only"),
			false));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blender/animation/limit_playback"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blender/animation/always_sample"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blender/animation/group_tracks"), true));
}

#endif // TOOLS_ENABLED
