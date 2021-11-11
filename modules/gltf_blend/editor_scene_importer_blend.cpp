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

void EditorSceneFormatImporterBlend::get_extensions(List<String> *r_extensions) const {
	r_extensions->push_back("blend");
}

Node *EditorSceneFormatImporterBlend::import_scene(const String &p_path,
		uint32_t p_flags, int p_bake_fps,
		List<String> *r_missing_deps,
		Error *r_err) {
	// TODO: fire 2021-11-11 Handle OCTAHEDRAL_COMPRESSION
	String path_global = ProjectSettings::get_singleton()->globalize_path(p_path);
	path_global = path_global.c_escape();
	String output_path = "res://.godot/imported/" + p_path.get_file().get_basename() + "-" + p_path.md5_text() + ".glb";
	String output_path_global = ProjectSettings::get_singleton()->globalize_path(output_path);
	output_path_global = output_path_global.c_escape();
	Array standard_out;
	String addon_path = EDITOR_GET(blender_path_def);
	String addon_path_global = ProjectSettings::get_singleton()->globalize_path(addon_path);
	String script = (String("import bpy, os, sys;") +
			"bpy.ops.wm.open_mainfile(filepath='GODOT_FILENAME');" +
			"bpy.ops.export_scene.gltf(filepath='GODOT_EXPORT_PATH',export_format='GLB',export_colors=True,export_all_influences=True,export_extras=True,export_cameras=True,export_lights=True);");
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
		return nullptr;
	}
	Ref<EditorSceneFormatImporterGLTF> gltf_importer;
	gltf_importer.instantiate();
	Node *root_node = gltf_importer->call("_import_scene_from_other_importer", output_path, p_flags, p_bake_fps);
	return root_node;
}

Ref<Animation> EditorSceneFormatImporterBlend::import_animation(const String &p_path,
		uint32_t p_flags,
		int p_bake_fps) {
	return Ref<Animation>();
}

#endif // TOOLS_ENABLED
