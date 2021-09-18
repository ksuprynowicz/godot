/*************************************************************************/
/*  gltf_document_extension_editor_convert_mesh_instance.h               */
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

#ifndef GLTF_EXTENSION_EDITOR_H
#define GLTF_EXTENSION_EDITOR_H

#if TOOLS_ENABLED

#include "core/io/resource.h"
#include "core/variant/dictionary.h"

#include "editor/import/scene_importer_mesh.h"
#include "editor/import/scene_importer_mesh_node_3d.h"
#include "gltf_document.h"
#include "gltf_document_extension.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/main/node.h"

class GLTFDocumentExtension;
class GLTFDocument;
class GLTFDocumentExtensionEditorConvertMeshInstance : public GLTFDocumentExtension {
	GDCLASS(GLTFDocumentExtensionEditorConvertMeshInstance, GLTFDocumentExtension);

protected:
	static void _bind_methods();

public:
	GLTFDocumentExtensionEditorConvertMeshInstance();
	Error import_post(Ref<GLTFDocument> p_document, Node *p_node, Dictionary p_export_settings) override;
};
#endif // GLTF_EXTENSION_EDITOR_H
#endif // TOOLS_ENABLED
