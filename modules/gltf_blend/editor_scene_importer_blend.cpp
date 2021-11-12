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
	String addon_path = EditorSettings::get_singleton()->get_setting("filesystem/blend/blender_path");
	String scene_to_blend_setting = "import bpy, os, sys;"
		"bpy.ops.wm.open_mainfile(filepath='GODOT_SOURCE');"
		"bpy.ops.export_scene.gltf("
		"filepath='GODOT_SINK',"
		"export_format='GLTF_SEPARATE',"
		"export_keep_originals=True,"
		"export_texture_dir='GODOT_TEXTURE_DIR',"
		"export_colors=True,"
		"export_all_influences=True,"
		"export_extras=True,"
		"export_cameras=True,"
		"export_lights=True) if bpy.app.version >= (3, 0, 0) else "
		"bpy.ops.export_scene.gltf("
		"filepath='GODOT_SINK',"
		"export_format='GLTF_SEPARATE',"
		"export_texture_dir='GODOT_TEXTURE_DIR',"
		"export_colors=True,"
		"export_all_influences=True,"
		"export_extras=True,"
		"export_cameras=True,"
		"export_lights=True);";
	String script = scene_to_blend_setting;
	String source = ProjectSettings::get_singleton()->globalize_path(p_path);
	String source_global = ProjectSettings::get_singleton()->globalize_path(source);
	source_global = source_global.c_escape();
	core_bind::Directory dir;
	String texture_global = "res://.godot/imported/" + p_path.get_file().get_basename() + "-" + p_path.md5_text() + "_textures";
	texture_global = ProjectSettings::get_singleton()->globalize_path(texture_global);
	dir.make_dir(texture_global);
	texture_global = texture_global.c_escape();
	script = script.replace("GODOT_TEXTURE_DIR", texture_global);
	script = script.replace("GODOT_SOURCE", source_global);
	String sink = "res://.godot/imported/" + p_path.get_file().get_basename() + "-" + p_path.md5_text() + ".gltf";
	String sink_global = ProjectSettings::get_singleton()->globalize_path(sink);
	sink_global = sink_global.c_escape();
	script = script.replace("GODOT_SINK", sink_global);
	Vector<String> args;
	args.push_back("--background");
	args.push_back("--python-expr");
	args.push_back(script);
	print_line(script);
	Array standard_out;
	int32_t ret = core_bind::OS::get_singleton()->execute(addon_path, args, standard_out, true);
	for (int32_t line_i = 0; line_i < standard_out.size(); line_i++) {
		print_line(standard_out[line_i]);
	}
	if (ret != OK) {
		print_error("Blender returned " + itos(ret));
		return nullptr;
	}
	Ref<EditorSceneFormatImporterGLTF> gltf_importer;
	gltf_importer.instantiate();
	return gltf_importer->call("import_scene_from_other_importer", sink_global, p_flags, p_bake_fps);
}

Ref<Animation> EditorSceneFormatImporterBlend::import_animation(const String &p_path,
		uint32_t p_flags,
		int p_bake_fps) {
	return Ref<Animation>();
}

#endif // TOOLS_ENABLED
