/*************************************************************************/
/*  gltf_document_extension.h                                            */
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

#ifndef GLTF_DOCUMENT_EXTENSION_H
#define GLTF_DOCUMENT_EXTENSION_H

#include "core/io/resource.h"
#include "core/variant/dictionary.h"
#include "core/variant/typed_array.h"
#include "core/variant/variant.h"
#include "scene/main/node.h"

#include "gltf_accessor.h"
#include "gltf_document.h"
#include "gltf_node.h"
#include "gltf_state.h"

#include "core/object/gdvirtual.gen.inc"
#include "core/object/script_language.h"
#include "core/variant/native_ptr.h"

class GLTFDocumentExtension : public Resource {
	GDCLASS(GLTFDocumentExtension, Resource);

	Dictionary import_options;
	Dictionary export_options;

protected:
	static void _bind_methods();

public:
	virtual Dictionary get_import_options() const;
	virtual void set_import_options(Dictionary p_options);
	virtual Dictionary get_export_options() const;
	virtual void set_export_options(Dictionary p_options);
	virtual Error import_preflight(Dictionary p_options, Ref<GLTFState> p_state);
	virtual Error import_post_parse(Dictionary p_options, Ref<GLTFState> p_state);
	virtual Error export_post(Dictionary p_options, Ref<GLTFState> p_state);
	// These stages should not be able to modify the original node when checking conditions.
	virtual Error import_post(Dictionary p_options, Ref<GLTFState> p_state, Node *p_node);
	virtual Error export_preflight(Dictionary p_options, Node *p_state);
	// End condition checking stages.
	Error import_node(Dictionary p_options, Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Dictionary &r_json, Node *p_node);
	Error export_node(Dictionary p_options, Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Dictionary &r_json, Node *p_node);
	GDVIRTUAL2R(int, _import_preflight, Dictionary, Ref<GLTFState>);
	GDVIRTUAL2R(int, _import_post_parse, Dictionary, Ref<GLTFState>);
	GDVIRTUAL5R(int, _import_node, Dictionary, Ref<GLTFState>, Ref<GLTFNode>, Dictionary, Node *);
	GDVIRTUAL3R(int, _import_post, Dictionary, Ref<GLTFState>, Node *);
	GDVIRTUAL2R(int, _export_preflight, Dictionary, Node *);
	GDVIRTUAL5R(int, _export_node, Dictionary, Ref<GLTFState>, Ref<GLTFNode>, Dictionary, Node *);
	GDVIRTUAL2R(int, _export_post, Dictionary, Ref<GLTFState>);
};

#endif // GLTF_DOCUMENT_EXTENSION_H
