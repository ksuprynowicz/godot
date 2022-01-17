/*************************************************************************/
/*  openxr_vulkan_extension_dummy.cpp                                    */
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

void **OpenXRVulkanExtension::set_session_create_and_get_next_pointer(void **p_property) {
	return nullptr;
}

bool OpenXRVulkanExtension::create_vulkan_instance(const VkInstanceCreateInfo *p_vulkan_create_info, VkInstance *r_instance) {
	return false;
}

bool OpenXRVulkanExtension::get_physical_device(VkPhysicalDevice *r_device) {
	return false;
}

bool OpenXRVulkanExtension::create_vulkan_device(const VkDeviceCreateInfo *p_device_create_info, VkDevice *r_device) {
	return false;
}

void OpenXRVulkanExtension::get_usable_swapchain_formats(Vector<int64_t> &p_usable_swap_chains) {
}

String OpenXRVulkanExtension::get_swapchain_format_name(int64_t p_swapchain_format) const {
	return String();
}

bool OpenXRVulkanExtension::get_swapchain_image_data(XrSwapchain p_swapchain, int64_t p_swapchain_format, uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size, void **r_swapchain_graphics_data) {
	return false;
}

void OpenXRVulkanExtension::cleanup_swapchain_graphics_data(void **p_swapchain_graphics_data) {
}

bool OpenXRVulkanExtension::copy_render_target_to_image(RID p_from_render_target, void *p_swapchain_graphics_data, int p_image_index) {
	return false;
}

bool OpenXRVulkanExtension::check_graphics_api_support(XrVersion p_desired_version) {
	return false;
}

XrResult OpenXRVulkanExtension::xrGetVulkanGraphicsRequirements2KHR(XrInstance p_instance, XrSystemId p_system_id, XrGraphicsRequirementsVulkanKHR *p_graphics_requirements) {
	return XR_ERROR_INSTANCE_LOST;
}

XrResult OpenXRVulkanExtension::xrCreateVulkanInstanceKHR(XrInstance p_instance, const XrVulkanInstanceCreateInfoKHR *p_create_info, VkInstance *r_vulkan_instance, VkResult *r_vulkan_result) {
	return XR_ERROR_INSTANCE_LOST;
}

XrResult OpenXRVulkanExtension::xrGetVulkanGraphicsDevice2KHR(XrInstance p_instance, const XrVulkanGraphicsDeviceGetInfoKHR *p_get_info, VkPhysicalDevice *r_vulkan_physical_device) {
	return XR_ERROR_INSTANCE_LOST;
}

XrResult OpenXRVulkanExtension::xrCreateVulkanDeviceKHR(XrInstance p_instance, const XrVulkanDeviceCreateInfoKHR *p_create_info, VkDevice *r_device, VkResult *r_result) {
	return XR_ERROR_INSTANCE_LOST;
}
