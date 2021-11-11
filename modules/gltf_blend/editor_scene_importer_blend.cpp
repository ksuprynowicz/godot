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
	return ImportFlags::IMPORT_SCENE;
}

void EditorSceneFormatImporterBlend::get_extensions(
		List<String> *r_extensions) const {
	r_extensions->push_back("blend");
}

Node *EditorSceneFormatImporterBlend::import_scene(
		const String &p_path, uint32_t p_flags, const Dictionary &p_options,
		int p_bake_fps, List<String> *r_missing_deps, Error *r_err) {
	String path = _import_scene_internal(p_path, p_flags, p_options, p_bake_fps,
			r_missing_deps, r_err);
	if (r_err && *r_err != OK) {
		print_error("Blender returned " + itos(*r_err));
		return nullptr;
	}
	if (p_options["blend/animation/animations_only"]) {
		Dictionary options = p_options;
			_modify_animations_only(path);
			options["blend/nodes/punctual_lights"] = false;
			options["blend/nodes/cameras"] = false;
		return import_scene_from_other_importer(path,
				p_flags, options, p_bake_fps);
	}
	return import_scene_from_other_importer(path,
			p_flags, p_options, p_bake_fps);
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
			PropertyInfo(Variant::INT, "blend/nodes/visible",
					PropertyHint::PROPERTY_HINT_ENUM,
					"Visible Only,Renderable,All"),
			BLEND_VISIBLE_ALL));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/nodes/punctual_lights"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/nodes/cameras"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/nodes/custom_properties"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::INT, "blend/nodes/modifiers",
					PropertyHint::PROPERTY_HINT_ENUM,
					"No Modifiers,All Modifiers"),
			BLEND_MODIFIERS_ALL));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/meshes/colors"), false));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/meshes/uvs"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/meshes/normals"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/meshes/tangents"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::INT, "blend/meshes/skins",
					PropertyHint::PROPERTY_HINT_ENUM,
					"None,4 Influences (Compatible),All Influences"),
			BLEND_BONE_INFLUENCES_ALL));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::STRING, "blend/materials/texture_directory",
					PropertyHint::PROPERTY_HINT_DIR),
			""));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL,
					"blend/meshes/export_bones_deforming_mesh_only"),
			false));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/animation/animations_only"), false));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/animation/limit_playback"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/animation/always_sample"), true));
	r_options->push_back(ResourceImporter::ImportOption(
			PropertyInfo(Variant::BOOL, "blend/animation/group_tracks"), true));
}

String EditorSceneFormatImporterBlend::_import_scene_internal(
		const String &p_path, uint32_t p_flags, const Dictionary &p_options,
		int p_bake_fps, List<String> *r_missing_deps, Error *r_err) {
	String addon_path = EditorSettings::get_singleton()->get_setting(
			"filesystem/blend/blender_path");
	String json_parameters =
			"{\"source\": \"GODOT_SOURCE\", \"sink\": \"GODOT_SINK\"}";
	String texture_dir_string = p_options["blend/materials/texture_directory"];
	String texture_dir = texture_dir_string;
	if (texture_dir.is_empty()) {
		texture_dir = "res://.godot/imported/" + p_path.get_file().get_basename() +
				"-" + p_path.md5_text() + "_textures";
	}
	String texture_dir_global =
			ProjectSettings::get_singleton()->globalize_path(texture_dir);
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
		return "";
	}
	Dictionary parameters = json->get_data();
	String extra_properties_arg = p_options["blend/nodes/custom_properties"]
			? "export_extras=True,"
			: "export_extras=False,";
	int32_t skins = p_options["blend/meshes/skins"];
	String influence_arg = "";
	String skins_arg = "";
	if (skins == BLEND_BONE_INFLUENCES_NONE) {
		influence_arg = "export_all_influences=False,";
		skins_arg = "export_skins=False,";
	}
	if (skins == BLEND_BONE_INFLUENCES_COMPATIBLE) {
		influence_arg = "export_all_influences=False,";
	} else if (skins == 2) {
		influence_arg = "export_all_influences=True,";
	}
	String cameras_arg = p_options["blend/nodes/cameras"]
			? "export_cameras=True,"
			: "export_cameras=False,";
	String lights_last_arg = p_options["blend/nodes/lights"]
			? "export_lights=True"
			: "export_lights=False";
	String colors_arg = p_options["blend/meshes/colors"] ? "export_colors=True,"
														 : "export_colors=False,";
	String visible_arg;
	int32_t visible = p_options["blend/nodes/visible"];
	if (visible == BLEND_VISIBLE_VISIBLE_ONLY) {
		visible_arg = "use_visible=True,";
	} else if (visible == BLEND_VISIBLE_RENDERABLE) {
		visible_arg = "use_renderable=True,";
	}
	String uvs_arg =
			p_options["blend/meshes/uvs"] ? "" : "export_texcoords=False,";
	String normals_arg = p_options["blend/meshes/normals"]
			? "export_normals=True,"
			: "export_normals=False,";
	String tangents_arg = p_options["blend/meshes/tangents"]
			? "export_tangents=True,"
			: "export_tangents=False,";
	String animations_arg = p_options["animations/import"]
			? "export_animations=True,"
			: "export_animations=False,";
	String animation_groups_arg = p_options["blend/animation/group_tracks"]
			? "export_nla_strips=True,"
			: "export_nla_strips=False,";
	String animation_limit_playback_arg =
			p_options["blend/animation/limit_playback"] ? "export_frame_range=True,"
														: "";
	String animation_always_sample_arg =
			p_options["blend/animation/always_sample"] ? "export_force_sampling=True,"
													   : "";
	String deform_bones_args =
			p_options["blend/meshes/export_bones_deforming_mesh_only"]
			? "export_def_bones=True,"
			: "export_def_bones=False,";
	String apply_modifications_arg =
			int32_t(p_options["blend/nodes/modifiers"]) == BLEND_MODIFIERS_ALL
			? "export_apply=True,"
			: "export_apply=False,";
	String common_args =
			vformat("filepath='%s',", parameters["sink"]) +
			vformat("export_texture_dir='%s',", texture_dir_global) +
			"export_format='GLTF_SEPARATE',"
			"export_yup=True," +
			visible_arg + extra_properties_arg + apply_modifications_arg +
			deform_bones_args + colors_arg + uvs_arg + skins_arg + influence_arg +
			animation_always_sample_arg + animation_limit_playback_arg +
			animations_arg + animation_groups_arg + cameras_arg + lights_last_arg;
	String scene_to_blend_setting =
			"import bpy, os, sys;" +
			vformat("bpy.ops.wm.open_mainfile(filepath='%s');",
					parameters["source"]) +
			"(bpy.ops.export_scene.gltf(export_keep_originals=True," + common_args +
			")) "
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
		return "";
	}
	return sink;
}

void EditorSceneFormatImporterBlend::_add_all_gltf_nodes_to_skin(
		Dictionary &obj) {
	Dictionary scene_nodes;
	Dictionary scenes = obj["scenes"];
	Array keys = scenes.keys();
	for (int32_t key_i = 0; key_i < scenes.keys().size(); key_i++) {
		PackedInt32Array nodes = scenes["nodes"];
		for (const int32_t &node : nodes) {
			scene_nodes[node] = scenes[keys[key_i]];
		}
	}
	PackedInt32Array new_joints;
	Array nodes = obj["nodes"];
	for (int32_t node_i = 0; node_i < nodes.size(); node_i++) {
		int32_t node = nodes[node_i];
		if (!scene_nodes.has(node)) {
			Dictionary nodes = obj["nodes"];
			Dictionary gltf_node = nodes[node];
			gltf_node["name"] = "Armature";
		} else {
			new_joints.push_back(node);
		}
	}
	new_joints.sort();
	Dictionary new_skin;
	new_skin["joints"] = new_joints;
	if (!obj.has("skins")) {
		obj["skins"] = Array();
	} else {
		Array skins = obj["skins"];
		skins.push_back(new_skin);
	}
	obj["skins"] = skins;
}

Error EditorSceneFormatImporterBlend::_modify_animations_only(String path) {
	FileAccessRef f = FileAccess::open(path, FileAccess::READ);
	if (!f) {
		return FAILED;
	}
	Ref<JSON> json;
	json.instantiate();
	String json_str = f->get_as_utf8_string();
	f->close();
	Error error = json->parse(json_str);
	ERR_FAIL_COND_V_MSG(error != OK, ERR_FILE_UNRECOGNIZED,
			vformat("Failed to parse JSON part of "
					"glTF file in %s : %s : %s ",
					path, json->get_error_line(),
					json->get_error_message()));
	Dictionary gltf_json_parsed = json->get_data();
	_add_all_gltf_nodes_to_skin(gltf_json_parsed);
	_remove_gltf_meshes(gltf_json_parsed);
	FileAccessRef fa = FileAccess::open(path, FileAccess::WRITE);
	if (!fa) {
		return FAILED;
	}
	fa->store_string(json->stringify(gltf_json_parsed));
	fa->close();
	return OK;
}

void EditorSceneFormatImporterBlend::_remove_gltf_meshes(Dictionary &obj) {
  Dictionary scene_nodes;
  Dictionary scenes = obj["scenes"];
  Array keys = scenes.keys();
  for (int32_t key_i = 0; key_i < scenes.keys().size(); key_i++) {
    PackedInt32Array nodes = scenes["nodes"];
    for (const int32_t &node : nodes) {
      scene_nodes[node] = scenes[keys[key_i]];
    }
  }
  Array nodes = obj["nodes"];
  for (int32_t node_i = 0; node_i < nodes.size(); node_i++) {
    int32_t node = nodes[node_i];
    if (!scene_nodes.has(node)) {
      Dictionary nodes = obj["nodes"];
      Dictionary gltf_node = nodes[node];
      gltf_node["mesh"] = -1;
    }
  }
  obj["nodes"] = nodes;
}

#endif // TOOLS_ENABLED