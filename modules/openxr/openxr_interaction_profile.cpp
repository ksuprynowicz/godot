/*************************************************************************/
/*  openxr_interaction_profile.cpp                                       */
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

#include "openxr_interaction_profile.h"

void OpenXRIPBinding::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_action", "action"), &OpenXRIPBinding::set_action);
	ClassDB::bind_method(D_METHOD("get_action"), &OpenXRIPBinding::get_action);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "action", PROPERTY_HINT_RESOURCE_TYPE, "OpenXRAction"), "set_action", "get_action");

	ClassDB::bind_method(D_METHOD("set_input_paths", "input_paths"), &OpenXRIPBinding::set_input_paths);
	ClassDB::bind_method(D_METHOD("get_input_paths"), &OpenXRIPBinding::get_input_paths);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "input_paths", PROPERTY_HINT_ARRAY_TYPE, "STRING"), "set_input_paths", "get_input_paths");
}

Ref<OpenXRIPBinding> OpenXRIPBinding::new_binding(const Ref<OpenXRAction> p_action, const char *p_input_paths) {
	// This is a helper function to help build our default action sets

	Ref<OpenXRIPBinding> binding;
	binding.instantiate();
	binding->set_action(p_action);
	binding->parse_input_paths(String(p_input_paths));

	return binding;
}

void OpenXRIPBinding::set_action(const Ref<OpenXRAction> p_action) {
	action = p_action;
}

Ref<OpenXRAction> OpenXRIPBinding::get_action() const {
	return action;
}

void OpenXRIPBinding::set_input_paths(const PackedStringArray p_input_paths) {
	input_paths = p_input_paths;
}

PackedStringArray OpenXRIPBinding::get_input_paths() const {
	return input_paths;
}

void OpenXRIPBinding::parse_input_paths(const String p_input_paths) {
	input_paths = p_input_paths.split(",", false);
}

OpenXRIPBinding::~OpenXRIPBinding() {
	action.unref();
}

void OpenXRInteractionProfile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_interaction_profile_path", "interaction_profile_path"), &OpenXRInteractionProfile::set_interaction_profile_path);
	ClassDB::bind_method(D_METHOD("get_interaction_profile_path"), &OpenXRInteractionProfile::get_interaction_profile_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "interaction_profile_path"), "set_interaction_profile_path", "get_interaction_profile_path");
}

Ref<OpenXRInteractionProfile> OpenXRInteractionProfile::new_profile(const char *p_input_profile_path) {
	Ref<OpenXRInteractionProfile> profile;
	profile.instantiate();
	profile->set_interaction_profile_path(String(p_input_profile_path));

	return profile;
}

void OpenXRInteractionProfile::set_interaction_profile_path(const String p_input_profile_path) {
	interaction_profile_path = p_input_profile_path;
}

String OpenXRInteractionProfile::get_interaction_profile_path() const {
	return interaction_profile_path;
}

void OpenXRInteractionProfile::set_bindings(Array p_bindings) {
	clear_bindings();
	for (int i = 0; i < p_bindings.size(); i++) {
		Ref<OpenXRIPBinding> binding = p_bindings[i];
		add_binding(binding);
	}
}

Array OpenXRInteractionProfile::get_bindings() const {
	Array arr;

	for (int i = 0; i < bindings.size(); i++) {
		arr.push_back(bindings[i]);
	}

	return arr;
}

void OpenXRInteractionProfile::add_binding(Ref<OpenXRIPBinding> p_binding) {
	ERR_FAIL_COND(p_binding.is_valid());

	if (bindings.find(p_binding) == -1) {
		// Not sure if Vector properly refcounts so taking the long way around..
		Ref<OpenXRIPBinding> binding;
		if (bindings.push_back(binding)) {
			bindings.ptrw()[bindings.size() - 1] = p_binding;
		}
	}
}

void OpenXRInteractionProfile::remove_binding(Ref<OpenXRIPBinding> p_binding) {
	int idx = bindings.find(p_binding);
	if (idx != -1) {
		// Not sure if Vector properly refcounts so taking the long way around..
		bindings.ptrw()[idx].unref();
		bindings.remove_at(idx);
	}
}

void OpenXRInteractionProfile::clear_bindings() {
	// Not sure if Vector properly refcounts so taking the long way around..
	for (int i = 0; i < bindings.size(); i++) {
		bindings.ptrw()[i].unref();
	}
	bindings.clear();
}

void OpenXRInteractionProfile::add_new_binding(const Ref<OpenXRAction> p_action, const char *p_input_paths) {
	// This is a helper function to help build our default action sets

	Ref<OpenXRIPBinding> binding = OpenXRIPBinding::new_binding(p_action, p_input_paths);
	add_binding(binding);
}

OpenXRInteractionProfile::~OpenXRInteractionProfile() {
	clear_bindings();
}
