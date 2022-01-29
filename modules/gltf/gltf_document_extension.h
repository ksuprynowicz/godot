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

#include "core/error/error_list.h"
#include "core/error/error_macros.h"
#include "core/io/resource.h"
#include "core/object/script_language.h"
#include "core/variant/dictionary.h"
#include "core/variant/typed_array.h"
#include "core/variant/variant.h"

#include "core/object/gdvirtual.gen.inc"

class GLTFState;
class GLTFDocumentExtension : public Resource {
	GDCLASS(GLTFDocumentExtension, Resource);

	Dictionary import_settings;
	Dictionary export_settings;

protected:
	static void _bind_methods();
	GDVIRTUAL0RC(Array, _get_import_setting_keys)
	GDVIRTUAL1RC(Variant, _get_import_setting, StringName)
	GDVIRTUAL2RC(Variant, _set_import_setting, StringName, Variant)
	GDVIRTUAL1RC(int, _import_preflight, RES)
	GDVIRTUAL2RC(int, _import_post, RES, Object *)
	GDVIRTUAL0RC(Array, _get_export_setting_keys)
	GDVIRTUAL1RC(Variant, _set_export_setting, StringName)
	GDVIRTUAL2RC(Variant, _get_export_setting, StringName, Variant)
	GDVIRTUAL2RC(int, _export_preflight, RES, Object *)
	GDVIRTUAL1RC(int, _export_post, RES)

public:
	virtual Error import_preflight(Ref<GLTFState> p_state);
	virtual Error import_post(Ref<GLTFState> p_state, Node *p_node);
	virtual Array get_import_setting_keys() const;
	virtual Variant get_import_setting(const StringName &p_key) const;
	virtual void set_import_setting(const StringName &p_key, Variant p_var);

public:
	virtual Array get_export_setting_keys() const;
	virtual Variant get_export_setting(const StringName &p_key) const;
	virtual void set_export_setting(const StringName &p_key, Variant p_var);
	virtual Error export_preflight(Ref<GLTFState> p_state, Node *p_node);
	virtual Error export_post(Ref<GLTFState> p_state);
};

#endif // GLTF_DOCUMENT_EXTENSION_H
