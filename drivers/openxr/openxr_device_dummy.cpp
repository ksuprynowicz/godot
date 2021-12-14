/*************************************************************************/
/*  openxr_device_dummy.cpp                                              */
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

// This dummy class will never be instantiated, we purely have this to
// allow us to compile Godot on platforms that do not support OpenXR
// We do re-use our class definition

#include "openxr_device.h"

void OpenXRDevice::setup_global_defs() {
}

bool OpenXRDevice::openxr_is_enabled() {
}

OpenXRDevice *OpenXRDevice::get_singleton() {
	return nullptr;
}

String OpenXRDevice::get_error_string(XrResult result) {
	return String();
}

String OpenXRDevice::get_view_configuration_name(XrViewConfigurationType p_view_configuration) const {
	return String();
}

String OpenXRDevice::get_reference_space_name(XrReferenceSpaceType p_reference_space) const {
	return String();
}

String OpenXRDevice::make_xr_version_string(XrVersion p_version) {
	return String();
}

bool OpenXRDevice::is_initialized() {
	return false;
}

bool OpenXRDevice::initialise(const String &p_rendering_driver) {
	return false;
}

void OpenXRDevice::finish() {
}

void OpenXRDevice::register_extension_wrapper(OpenXRExtensionWrapper *p_extension_wrapper) {
}

Size2 OpenXRDevice::get_recommended_target_size() {
	return Size2();
}

OpenXRDevice::OpenXRDevice() {
}

OpenXRDevice::~OpenXRDevice() {
}
