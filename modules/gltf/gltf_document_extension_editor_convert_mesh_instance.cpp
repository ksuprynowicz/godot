/*************************************************************************/
/*  gltf_document_extension_editor_convert_mesh_instance.cpp             */
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

#include "gltf_document_extension_editor_convert_mesh_instance.h"

#if TOOLS_ENABLED
void GLTFDocumentExtensionEditorConvertMeshInstance::_bind_methods() {
}

Error GLTFDocumentExtensionEditorConvertMeshInstance::import_post(Ref<GLTFDocument> p_document, Node *p_node, Dictionary p_export_settings) {
	List<Node *> queue;
	queue.push_back(p_node);
	List<Node *> delete_queue;
	while (!queue.is_empty()) {
		List<Node *>::Element *E = queue.front();
		Node *node = E->get();
		{
			MeshInstance3D *mesh_3d = cast_to<MeshInstance3D>(node);
			if (mesh_3d) {
				EditorSceneImporterMeshNode3D *editor_mesh_node_3d = memnew(EditorSceneImporterMeshNode3D);
				Ref<ArrayMesh> array_mesh = mesh_3d->get_mesh();
				Ref<EditorSceneImporterMesh> editor_mesh;
				editor_mesh.instantiate();
				for (int32_t surface_i = 0; surface_i < array_mesh->get_surface_count(); surface_i++) {
					Array surface_array = array_mesh->surface_get_arrays(surface_i);
					Array blend_arrays = array_mesh->surface_get_blend_shape_arrays(surface_i);
					Ref<Material> mat = array_mesh->surface_get_material(surface_i);
					editor_mesh->add_surface(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, surface_array, blend_arrays, Dictionary(), mat, mat->get_name(),
							array_mesh->surface_get_format(surface_i));
				}
				editor_mesh->set_blend_shape_mode(array_mesh->get_blend_shape_mode());
				editor_mesh_node_3d->set_name(node->get_name());
				editor_mesh_node_3d->set_transform(mesh_3d->get_transform());
				editor_mesh_node_3d->set_mesh(editor_mesh);
				editor_mesh_node_3d->set_skin(mesh_3d->get_skin());
				editor_mesh_node_3d->set_skeleton_path(mesh_3d->get_skeleton_path());
				node->replace_by(editor_mesh_node_3d);
				delete_queue.push_back(node);
			}
		}
		int child_count = node->get_child_count();
		for (int i = 0; i < child_count; i++) {
			queue.push_back(node->get_child(i));
		}
		queue.pop_front();
	}
	while (!queue.is_empty()) {
		List<Node *>::Element *E = delete_queue.front();
		Node *node = E->get();
		memdelete(node);
		delete_queue.pop_front();
	}
	return OK;
}

GLTFDocumentExtensionEditorConvertMeshInstance::GLTFDocumentExtensionEditorConvertMeshInstance() {
	set_name("Convert mesh instances to editor meshes.");
}

#endif // TOOLS_ENABLED
