/*************************************************************************/
/*  openxr_vulkan_extension_dummy.cpp                                    */
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

#include "openxr_vulkan_extension.h"

OpenXRVulkanExtension *OpenXRVulkanExtension::get_singleton() {
	return nullptr;
}

OpenXRVulkanExtension::OpenXRVulkanExtension(OpenXRDevice *p_openxr_device) :
		OpenXRExtensionWrapper(p_openxr_device) {
}

OpenXRVulkanExtension::~OpenXRVulkanExtension() {
}

void OpenXRVulkanExtension::on_instance_created(const XrInstance instance) {
}

bool OpenXRVulkanExtension::create_vulkan_instance(const VkInstanceCreateInfo *p_vulkan_create_info, VkInstance *r_instance) {
	return false;
}

bool OpenXRVulkanExtension::get_physical_device(VkPhysicalDevice *r_device) {
	return false;
}
