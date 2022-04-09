/*************************************************************************/
/*  gltf_document_extension.cpp                                          */
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

#include "gltf_document_extension.h"

#include "core/variant/dictionary.h"
#include "gltf_document.h"
#include "gltf_state.h"

void GLTFDocumentExtension::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_import_options"), &GLTFDocumentExtension::get_import_options);
	ClassDB::bind_method(D_METHOD("get_export_options"), &GLTFDocumentExtension::get_export_options);
	ClassDB::bind_method(D_METHOD("set_import_options", "options"), &GLTFDocumentExtension::set_import_options);
	ClassDB::bind_method(D_METHOD("set_export_options", "options"), &GLTFDocumentExtension::set_export_options);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "import_options"), "set_import_options", "get_import_options");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "export_options"), "set_export_options", "get_export_options");
	GDVIRTUAL_BIND(_import_preflight, "options", "state");
	GDVIRTUAL_BIND(_import_post_parse, "options", "state");
	GDVIRTUAL_BIND(_import_node, "options", "state", "gltf_node", "json", "node");
	GDVIRTUAL_BIND(_import_post, "options", "state", "root");
	GDVIRTUAL_BIND(_export_preflight, "options", "root");
	GDVIRTUAL_BIND(_export_node, "options", "state", "gltf_node", "json", "node");
	GDVIRTUAL_BIND(_export_post, "options", "state");
}

Dictionary GLTFDocumentExtension::get_import_options() const {
	return import_options;
}

Dictionary GLTFDocumentExtension::get_export_options() const {
	return import_options;
}

Error GLTFDocumentExtension::import_post(Dictionary p_options, Ref<GLTFState> p_state, Node *p_root) {
	ERR_FAIL_NULL_V(p_root, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(p_state, ERR_INVALID_PARAMETER);
	int err = OK;
	if (GDVIRTUAL_CALL(_import_post, p_options, p_state, p_root, err)) {
		return Error(err);
	}
	return OK;
}

Error GLTFDocumentExtension::import_preflight(Dictionary p_options, Ref<GLTFState> p_state) {
	ERR_FAIL_NULL_V(p_state, ERR_INVALID_PARAMETER);
	int err = OK;
	if (GDVIRTUAL_CALL(_import_preflight, p_options, p_state, err)) {
		return Error(err);
	}
	return OK;
}

Error GLTFDocumentExtension::import_post_parse(Dictionary p_options, Ref<GLTFState> p_state) {
	ERR_FAIL_NULL_V(p_state, ERR_INVALID_PARAMETER);
	int err = OK;
	if (GDVIRTUAL_CALL(_import_post_parse, p_options, p_state, err)) {
		return Error(err);
	}
	return OK;
}

Error GLTFDocumentExtension::export_post(Dictionary p_options, Ref<GLTFState> p_state) {
	ERR_FAIL_NULL_V(p_state, ERR_INVALID_PARAMETER);
	int err = OK;
	if (GDVIRTUAL_CALL(_export_post, p_options, p_state, err)) {
		return Error(err);
	}
	return OK;
}
Error GLTFDocumentExtension::export_preflight(Dictionary p_options, Node *p_root) {
	ERR_FAIL_NULL_V(p_root, ERR_INVALID_PARAMETER);
	int err = OK;
	if (GDVIRTUAL_CALL(_export_preflight, p_options, p_root, err)) {
		return Error(err);
	}
	return OK;
}

Error GLTFDocumentExtension::import_node(Dictionary p_options, Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Dictionary &r_dict, Node *p_node) {
	ERR_FAIL_NULL_V(p_state, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(p_gltf_node, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(p_node, ERR_INVALID_PARAMETER);
	int err = OK;
	if (GDVIRTUAL_CALL(_import_node, p_options, p_state, p_gltf_node, r_dict, p_node, err)) {
		return Error(err);
	}
	return OK;
}

Error GLTFDocumentExtension::export_node(Dictionary p_options, Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Dictionary &r_dict, Node *p_node) {
	ERR_FAIL_NULL_V(p_state, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(p_gltf_node, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(p_node, ERR_INVALID_PARAMETER);
	int err = OK;
	if (GDVIRTUAL_CALL(_export_node, p_options, p_state, p_gltf_node, r_dict, p_node, err)) {
		return Error(err);
	}
	return OK;
}

void GLTFDocumentExtension::set_import_options(Dictionary p_options) {
	import_options = p_options;
}

void GLTFDocumentExtension::set_export_options(Dictionary p_options) {
	export_options = p_options;
}
