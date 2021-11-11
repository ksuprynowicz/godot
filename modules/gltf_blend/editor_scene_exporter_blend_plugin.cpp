/*************************************************************************/
/*  editor_scene_exporter_blend_plugin.cpp                               */
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

#if TOOLS_ENABLED
#include "editor_scene_exporter_blend_plugin.h"
#include "core/config/project_settings.h"
#include "core/core_bind.h"
#include "core/error/error_list.h"
#include "core/object/object.h"
#include "core/templates/vector.h"
#include "editor/editor_file_system.h"
#include "editor/editor_node.h"
#include "modules/gltf/gltf_document.h"
#include "modules/gltf/gltf_state.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/gui/check_box.h"
#include "scene/main/node.h"

String SceneExporterBlendPlugin::get_name() const {
	return "ConvertBlend";
}

bool SceneExporterBlendPlugin::has_main_screen() const {
	return false;
}

SceneExporterBlendPlugin::SceneExporterBlendPlugin(EditorNode *p_node) {
	editor = p_node;
	file_export_lib = memnew(EditorFileDialog);
	editor->get_gui_base()->add_child(file_export_lib);
	file_export_lib->connect("file_selected", callable_mp(this, &SceneExporterBlendPlugin::_blend_dialog_action));
	file_export_lib->set_title(TTR("Export Library"));
	file_export_lib->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	file_export_lib->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	file_export_lib->clear_filters();
	file_export_lib->add_filter("*.blend");
	file_export_lib->set_title(TTR("Export Mesh Blend"));
	String gltf_scene_name = TTR("Export Blend...");
	add_tool_menu_item(gltf_scene_name, callable_mp(this, &SceneExporterBlendPlugin::convert_scene_to_blend));
}

void SceneExporterBlendPlugin::_blend_dialog_action(String p_file) {
	Node *root = editor->get_tree()->get_edited_scene_root();
	if (!root) {
		editor->show_accept(TTR("This operation can't be done without a scene."), TTR("OK"));
		return;
	}
	String file = "res://.godot/imported/" + p_file.get_file().get_basename() + "-" + p_file.md5_text() + ".glb";
	List<String> deps;
	Ref<GLTFDocument> doc;
	doc.instantiate();
	Error err = doc->save_scene(root, file, file, 0, 30.0f, Ref<GLTFState>());
	if (err != OK) {
		ERR_PRINT(vformat("Blend save gltf scene error %s.", itos(err)));
	}
	String path_global = ProjectSettings::get_singleton()->globalize_path(file);
	path_global = path_global.c_escape();
	String output_path = p_file;
	String output_path_global = ProjectSettings::get_singleton()->globalize_path(output_path);
	output_path_global = output_path_global.c_escape();
	Array standard_out;
	String addon_path = EDITOR_GET(blender_path_def);
	String addon_path_global = ProjectSettings::get_singleton()->globalize_path(addon_path);
	String script = (String("import bpy, os, sys;") +
			"bpy.context.scene.render.fps=30.0;" +
			"bpy.ops.import_scene.gltf(filepath='GODOT_FILENAME', bone_heuristic='BLENDER');" +
			"bpy.ops.wm.save_mainfile(filepath='GODOT_EXPORT_PATH');");
	path_global = path_global.c_escape();
	script = script.replace("GODOT_FILENAME", path_global);
	output_path_global = output_path_global.c_escape();
	script = script.replace("GODOT_EXPORT_PATH", output_path_global);
	Vector<String> args;
	args.push_back("--background");
	args.push_back("--python-expr");
	args.push_back(script);
	String blender_path = EDITOR_GET(blender_path_def);
	int32_t ret = core_bind::OS::get_singleton()->execute(blender_path, args, standard_out, true);
	for (int32_t line_i = 0; line_i < standard_out.size(); line_i++) {
		print_line(standard_out[line_i]);
	}
	if (ret != OK) {
		print_error("Blender returned " + itos(ret));
		return;
	}
}

void SceneExporterBlendPlugin::convert_scene_to_blend() {
	Node *root = editor->get_tree()->get_edited_scene_root();
	if (!root) {
		editor->show_accept(TTR("This operation can't be done without a scene."), TTR("OK"));
		return;
	}
	String filename = String(root->get_scene_file_path().get_file().get_basename());
	if (filename.is_empty()) {
		filename = root->get_name();
	}
	file_export_lib->set_current_file(filename + String(".blend"));
	file_export_lib->popup_centered_ratio();
}

#endif // TOOLS_ENABLED
