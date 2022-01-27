/*************************************************************************/
/*  openxr_device.cpp                                                    */
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

#include "openxr_device.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/memory.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_settings.h"
#endif

#ifdef ANDROID_ENABLED
#include "openxr_android_extension.h"
#endif

#ifdef VULKAN_ENABLED
#include "openxr_vulkan_extension.h"
#endif

OpenXRDevice *OpenXRDevice::singleton = nullptr;

void OpenXRDevice::setup_global_defs() {
	// As OpenXRDevice is not constructed if OpenXR is not enabled, we register our project and editor settings here

	// TODO maybe move rendering/xr/enabled into xr/...

	// Project settings
	GLOBAL_DEF_BASIC("xr/openxr/enabled", false);
	GLOBAL_DEF_BASIC("xr/openxr/default_action_map", "res://default_action_map.tres");
	ProjectSettings::get_singleton()->set_custom_property_info("xr/openxr/default_action_map", PropertyInfo(Variant::STRING, "xr/openxr/default_action_map", PROPERTY_HINT_FILE, "*.tres"));

#ifdef TOOLS_ENABLED
	// editor settings (it seems we're too early in the process when setting up rendering, to access editor settings...)
	// EDITOR_DEF_RST("xr/openxr/in_editor", false);
	GLOBAL_DEF("xr/openxr/in_editor", false);
#endif
}

bool OpenXRDevice::openxr_is_enabled() {
	// @TODO we need an overrule switch so we can force enable openxr, i.e run "godot --openxr_enabled"

	if (Engine::get_singleton()->is_editor_hint()) {
#ifdef TOOLS_ENABLED
		bool enabled = GLOBAL_GET("xr/openxr/in_editor"); // EDITOR_GET("xr/openxr/in_editor");
		return enabled;
#else
		// we should never get here, editor hint won't be true if the editor isn't compiled in.
		return false;
#endif
	} else {
		bool enabled = GLOBAL_GET("xr/openxr/enabled");
		return enabled;
	}
}

OpenXRDevice *OpenXRDevice::get_singleton() {
	if (singleton != nullptr) {
		// already constructed, return our singleton
		return singleton;
	} else if (openxr_is_enabled()) {
		// construct our singleton and return it
		singleton = memnew(OpenXRDevice);
		return singleton;
	} else {
		// not enabled, don't instantiate, return nullptr
		return nullptr;
	}
}

String OpenXRDevice::get_default_action_map_resource_name() {
	String name = GLOBAL_GET("xr/openxr/default_action_map");

	return name;
}

String OpenXRDevice::get_error_string(XrResult result) {
	if (XR_SUCCEEDED(result)) {
		return String("Succeeded");
	}

	if (instance == XR_NULL_HANDLE) {
		Array args;
		args.push_back(Variant(result));
		return String("Error code {0}").format(args);
	}

	char resultString[XR_MAX_RESULT_STRING_SIZE];
	xrResultToString(instance, result, resultString);

	return String(resultString);
}

#define ENUM_TO_STRING_CASE(e) \
	case e: {                  \
		return String(#e);     \
	} break;

// TODO see if we can generate this code further using the xml file with meta data supplied by OpenXR

String OpenXRDevice::get_view_configuration_name(XrViewConfigurationType p_view_configuration) const {
	switch (p_view_configuration) {
		ENUM_TO_STRING_CASE(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO)
		ENUM_TO_STRING_CASE(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
		ENUM_TO_STRING_CASE(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO)
		ENUM_TO_STRING_CASE(XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT)
		ENUM_TO_STRING_CASE(XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM)
		default: {
			return String("View Configuration ") + String::num_int64(int64_t(p_view_configuration));
		} break;
	}
}

String OpenXRDevice::get_reference_space_name(XrReferenceSpaceType p_reference_space) const {
	switch (p_reference_space) {
		ENUM_TO_STRING_CASE(XR_REFERENCE_SPACE_TYPE_VIEW)
		ENUM_TO_STRING_CASE(XR_REFERENCE_SPACE_TYPE_LOCAL)
		ENUM_TO_STRING_CASE(XR_REFERENCE_SPACE_TYPE_STAGE)
		ENUM_TO_STRING_CASE(XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT)
		ENUM_TO_STRING_CASE(XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO)
		ENUM_TO_STRING_CASE(XR_REFERENCE_SPACE_TYPE_MAX_ENUM)
		default: {
			return String("Reference space ") + String::num_int64(int64_t(p_reference_space));
		} break;
	}
}

String OpenXRDevice::get_swapchain_format_name(int64_t p_swapchain_format) const {
	// This is rendering engine dependend...
	if (graphics_extension) {
		return graphics_extension->get_swapchain_format_name(p_swapchain_format);
	}

	return String("Swapchain format ") + String::num_int64(int64_t(p_swapchain_format));
}

String OpenXRDevice::get_structure_type_name(XrStructureType p_structure_type) const {
	switch (p_structure_type) {
		ENUM_TO_STRING_CASE(XR_TYPE_UNKNOWN)
		ENUM_TO_STRING_CASE(XR_TYPE_API_LAYER_PROPERTIES)
		ENUM_TO_STRING_CASE(XR_TYPE_EXTENSION_PROPERTIES)
		ENUM_TO_STRING_CASE(XR_TYPE_INSTANCE_CREATE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_SYSTEM_GET_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_SYSTEM_PROPERTIES)
		ENUM_TO_STRING_CASE(XR_TYPE_VIEW_LOCATE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_VIEW)
		ENUM_TO_STRING_CASE(XR_TYPE_SESSION_CREATE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_CREATE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_SESSION_BEGIN_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_VIEW_STATE)
		ENUM_TO_STRING_CASE(XR_TYPE_FRAME_END_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_HAPTIC_VIBRATION)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_BUFFER)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
		ENUM_TO_STRING_CASE(XR_TYPE_ACTION_STATE_BOOLEAN)
		ENUM_TO_STRING_CASE(XR_TYPE_ACTION_STATE_FLOAT)
		ENUM_TO_STRING_CASE(XR_TYPE_ACTION_STATE_VECTOR2F)
		ENUM_TO_STRING_CASE(XR_TYPE_ACTION_STATE_POSE)
		ENUM_TO_STRING_CASE(XR_TYPE_ACTION_SET_CREATE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_ACTION_CREATE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_INSTANCE_PROPERTIES)
		ENUM_TO_STRING_CASE(XR_TYPE_FRAME_WAIT_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_PROJECTION)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_QUAD)
		ENUM_TO_STRING_CASE(XR_TYPE_REFERENCE_SPACE_CREATE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_ACTION_SPACE_CREATE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING)
		ENUM_TO_STRING_CASE(XR_TYPE_VIEW_CONFIGURATION_VIEW)
		ENUM_TO_STRING_CASE(XR_TYPE_SPACE_LOCATION)
		ENUM_TO_STRING_CASE(XR_TYPE_SPACE_VELOCITY)
		ENUM_TO_STRING_CASE(XR_TYPE_FRAME_STATE)
		ENUM_TO_STRING_CASE(XR_TYPE_VIEW_CONFIGURATION_PROPERTIES)
		ENUM_TO_STRING_CASE(XR_TYPE_FRAME_BEGIN_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_EVENTS_LOST)
		ENUM_TO_STRING_CASE(XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED)
		ENUM_TO_STRING_CASE(XR_TYPE_INTERACTION_PROFILE_STATE)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_ACTION_STATE_GET_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_HAPTIC_ACTION_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_ACTIONS_SYNC_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_CUBE_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_VULKAN_SWAPCHAIN_FORMAT_LIST_CREATE_INFO_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_DEBUG_UTILS_LABEL_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_BINDING_D3D11_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_BINDING_D3D12_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_EYE_GAZE_SAMPLE_TIME_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_VISIBILITY_MASK_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_SESSION_CREATE_INFO_OVERLAY_EXTX)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_MAIN_SESSION_VISIBILITY_CHANGED_EXTX)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_COLOR_SCALE_BIAS_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_IMAGE_LAYOUT_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_ALPHA_BLEND_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_VIEW_CONFIGURATION_DEPTH_RANGE_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_GRAPHICS_BINDING_EGL_MNDX)
		ENUM_TO_STRING_CASE(XR_TYPE_SPATIAL_GRAPH_NODE_SPACE_CREATE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_JOINT_LOCATIONS_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_JOINT_VELOCITIES_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_SYSTEM_HAND_TRACKING_MESH_PROPERTIES_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_MESH_SPACE_CREATE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_MESH_UPDATE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_MESH_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_POSE_TYPE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SESSION_BEGIN_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SECONDARY_VIEW_CONFIGURATION_STATE_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_STATE_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_END_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SECONDARY_VIEW_CONFIGURATION_LAYER_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SWAPCHAIN_CREATE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_CONTROLLER_MODEL_KEY_STATE_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_CONTROLLER_MODEL_NODE_PROPERTIES_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_CONTROLLER_MODEL_PROPERTIES_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_CONTROLLER_MODEL_NODE_STATE_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_CONTROLLER_MODEL_STATE_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_VIEW_CONFIGURATION_VIEW_FOV_EPIC)
		ENUM_TO_STRING_CASE(XR_TYPE_HOLOGRAPHIC_WINDOW_ATTACHMENT_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_REPROJECTION_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_REPROJECTION_PLANE_OVERRIDE_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_ANDROID_SURFACE_SWAPCHAIN_CREATE_INFO_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_SECURE_CONTENT_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_INTERACTION_PROFILE_ANALOG_THRESHOLD_VALVE)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT)
		ENUM_TO_STRING_CASE(XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_OBSERVER_CREATE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_CREATE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_NEW_SCENE_COMPUTE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_VISUAL_MESH_COMPUTE_LOD_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_COMPONENTS_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_COMPONENTS_GET_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_COMPONENT_LOCATIONS_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_COMPONENTS_LOCATE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_OBJECTS_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_COMPONENT_PARENT_FILTER_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_OBJECT_TYPES_FILTER_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_PLANES_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_PLANE_ALIGNMENT_FILTER_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_MESHES_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_MESH_BUFFERS_GET_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_MESH_BUFFERS_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_MESH_VERTEX_BUFFER_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_MESH_INDICES_UINT32_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_MESH_INDICES_UINT16_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SERIALIZED_SCENE_FRAGMENT_DATA_GET_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SCENE_DESERIALIZE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_VIVE_TRACKER_PATHS_HTCX)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX)
		ENUM_TO_STRING_CASE(XR_TYPE_SYSTEM_COLOR_SPACE_PROPERTIES_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_TRACKING_MESH_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_TRACKING_SCALE_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_TRACKING_AIM_STATE_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_HAND_TRACKING_CAPSULES_STATE_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_TRIANGLE_MESH_CREATE_INFO_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_PASSTHROUGH_CREATE_INFO_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_GEOMETRY_INSTANCE_CREATE_INFO_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_GEOMETRY_INSTANCE_TRANSFORM_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_PASSTHROUGH_STYLE_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_RGBA_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_MONO_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_PASSTHROUGH_STATE_CHANGED_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_BINDING_MODIFICATIONS_KHR)
		ENUM_TO_STRING_CASE(XR_TYPE_VIEW_LOCATE_FOVEATED_RENDERING_VARJO)
		ENUM_TO_STRING_CASE(XR_TYPE_FOVEATED_VIEW_CONFIGURATION_VIEW_VARJO)
		ENUM_TO_STRING_CASE(XR_TYPE_SYSTEM_FOVEATED_RENDERING_PROPERTIES_VARJO)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_DEPTH_TEST_VARJO)
		ENUM_TO_STRING_CASE(XR_TYPE_SYSTEM_MARKER_TRACKING_PROPERTIES_VARJO)
		ENUM_TO_STRING_CASE(XR_TYPE_EVENT_DATA_MARKER_TRACKING_UPDATE_VARJO)
		ENUM_TO_STRING_CASE(XR_TYPE_MARKER_SPACE_CREATE_INFO_VARJO)
		ENUM_TO_STRING_CASE(XR_TYPE_SPATIAL_ANCHOR_PERSISTENCE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SPATIAL_ANCHOR_FROM_PERSISTED_ANCHOR_CREATE_INFO_MSFT)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_IMAGE_FOVEATION_VULKAN_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_STATE_ANDROID_SURFACE_DIMENSIONS_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_STATE_SAMPLER_OPENGL_ES_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_SWAPCHAIN_STATE_SAMPLER_VULKAN_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_COMPOSITION_LAYER_SPACE_WARP_INFO_FB)
		ENUM_TO_STRING_CASE(XR_TYPE_SYSTEM_SPACE_WARP_PROPERTIES_FB)
		ENUM_TO_STRING_CASE(XR_STRUCTURE_TYPE_MAX_ENUM)
		default: {
			return String("Structure type ") + String::num_int64(int64_t(p_structure_type));
		} break;
	}
}

String OpenXRDevice::get_session_state_name(XrSessionState p_session_state) const {
	switch (p_session_state) {
		ENUM_TO_STRING_CASE(XR_SESSION_STATE_UNKNOWN)
		ENUM_TO_STRING_CASE(XR_SESSION_STATE_IDLE)
		ENUM_TO_STRING_CASE(XR_SESSION_STATE_READY)
		ENUM_TO_STRING_CASE(XR_SESSION_STATE_SYNCHRONIZED)
		ENUM_TO_STRING_CASE(XR_SESSION_STATE_VISIBLE)
		ENUM_TO_STRING_CASE(XR_SESSION_STATE_FOCUSED)
		ENUM_TO_STRING_CASE(XR_SESSION_STATE_STOPPING)
		ENUM_TO_STRING_CASE(XR_SESSION_STATE_LOSS_PENDING)
		ENUM_TO_STRING_CASE(XR_SESSION_STATE_EXITING)
		ENUM_TO_STRING_CASE(XR_SESSION_STATE_MAX_ENUM)
		default: {
			return String("Session state ") + String::num_int64(int64_t(p_session_state));
		} break;
	}
}

String OpenXRDevice::make_xr_version_string(XrVersion p_version) {
	String version;

	version += String::num_int64(XR_VERSION_MAJOR(p_version));
	version += String(".");
	version += String::num_int64(XR_VERSION_MINOR(p_version));
	version += String(".");
	version += String::num_int64(XR_VERSION_PATCH(p_version));

	return version;
}

bool OpenXRDevice::load_layer_properties() {
	// This queries additional layers that are available and can be initialised when we create our OpenXR instance
	if (layer_properties != nullptr) {
		// already retrieved this
		return true;
	}

	// Note, instance is not yet setup so we can't use get_error_string to retrieve our error
	XrResult result = xrEnumerateApiLayerProperties(0, &num_layer_properties, nullptr);
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerate number of api layer properties");

	layer_properties = (XrApiLayerProperties *)memalloc(sizeof(XrApiLayerProperties) * num_layer_properties);
	ERR_FAIL_NULL_V(layer_properties, false);
	for (uint32_t i = 0; i < num_layer_properties; i++) {
		layer_properties[i].type = XR_TYPE_API_LAYER_PROPERTIES;
		layer_properties[i].next = nullptr;
	}

	result = xrEnumerateApiLayerProperties(num_layer_properties, &num_layer_properties, layer_properties);
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerate api layer properties");

#ifdef DEBUG
	for (uint32_t i = 0; i < num_layer_properties; i++) {
		print_line("OpenXR: Found OpenXR layer ", layer_properties[i].layerName);
	}
#endif

	return true;
}

bool OpenXRDevice::load_supported_extensions() {
	// This queries supported extensions that are available and can be initialised when we create our OpenXR instance

	if (supported_extensions != nullptr) {
		// already retrieved this
		return true;
	}

	// Note, instance is not yet setup so we can't use get_error_string to retrieve our error
	XrResult result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &num_supported_extensions, nullptr);
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerate number of extension properties");

	supported_extensions = (XrExtensionProperties *)memalloc(sizeof(XrExtensionProperties) * num_supported_extensions);
	ERR_FAIL_NULL_V(supported_extensions, false);

	// set our types
	for (uint32_t i = 0; i < num_supported_extensions; i++) {
		supported_extensions[i].type = XR_TYPE_EXTENSION_PROPERTIES;
		supported_extensions[i].next = nullptr;
	}
	result = xrEnumerateInstanceExtensionProperties(nullptr, num_supported_extensions, &num_supported_extensions, supported_extensions);
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerate extension properties");

#ifdef DEBUG
	for (uint32_t i = 0; i < num_supported_extensions; i++) {
		print_line("OpenXR: Found OpenXR extension ", supported_extensions[i].extensionName);
	}
#endif

	return true;
}

bool OpenXRDevice::is_extension_supported(const char *p_extension) const {
	for (uint32_t i = 0; i < num_supported_extensions; i++) {
		if (strcmp(supported_extensions[i].extensionName, p_extension)) {
			return true;
		}
	}

	return false;
}

bool OpenXRDevice::create_instance() {
	// Create our OpenXR instance, this will query any registered extension wrappers for extensions we need to enable.

	// Append the extensions requested by the registered extension wrappers.
	Map<const char *, bool *> requested_extensions;
	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		Map<const char *, bool *> wrapper_request_extensions = wrapper->get_request_extensions();

		// requested_extensions.insert(wrapper_request_extensions.begin(), wrapper_request_extensions.end());
		for (auto &requested_extension : wrapper_request_extensions) {
			requested_extensions[requested_extension.key] = requested_extension.value;
		}
	}

	// Check which extensions are supported
	enabled_extensions.clear();
	for (auto &requested_extension : requested_extensions) {
		if (!is_extension_supported(requested_extension.key)) {
			if (requested_extension.value == nullptr) {
				// nullptr means this is a manditory extension so we fail
				ERR_FAIL_V_MSG(false, "OpenXR: OpenXR Runtime does not support OpenGL extension!");
			} else {
				// set this extension as not supported
				*requested_extension.value = false;
			}
		} else if (requested_extension.value != nullptr) {
			// set this extension as supported
			*requested_extension.value = true;

			// and record that we want to enable it
			enabled_extensions.push_back(requested_extension.key);
		} else {
			// record that we want to enable this
			enabled_extensions.push_back(requested_extension.key);
		}
	}

	// Get our project name
	String project_name = GLOBAL_GET("application/config/name");
	CharString project_name_cs = project_name.utf8();

	// Create our OpenXR instance
	XrApplicationInfo application_info{
		"", // applicationName, we'll set this down below
		1, // applicationVersion, we don't currently have this
		"Godot Game Engine", // engineName
		4, // engineVersion
		XR_CURRENT_API_VERSION // apiVersion
	};

	XrInstanceCreateInfo instance_create_info = {
		XR_TYPE_INSTANCE_CREATE_INFO, // type
		nullptr, // next
		0, // createFlags
		application_info, // applicationInfo
		0, // enabledApiLayerCount, need to find out if we need support for this?
		nullptr, // enabledApiLayerNames
		uint32_t(enabled_extensions.size()), // enabledExtensionCount
		enabled_extensions.ptr() // enabledExtensionNames
	};

	int len = project_name.length();
	if (len < XR_MAX_APPLICATION_NAME_SIZE - 1) {
		// was having weird CI issues with strcpy so....
		memcpy(instance_create_info.applicationInfo.applicationName, project_name_cs.get_data(), len);
		instance_create_info.applicationInfo.applicationName[len] = '\0';
	} else {
		memcpy(instance_create_info.applicationInfo.applicationName, project_name_cs.get_data(), XR_MAX_APPLICATION_NAME_SIZE - 1);
		instance_create_info.applicationInfo.applicationName[XR_MAX_APPLICATION_NAME_SIZE - 1] = '\0';
	}

	XrResult result = xrCreateInstance(&instance_create_info, &instance);
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "Failed to create XR instance.");

	// from this point on we can use get_error_string to get more info about our errors...

	XrInstanceProperties instanceProps = {
		XR_TYPE_INSTANCE_PROPERTIES, // type;
		nullptr, // next
		0, // runtimeVersion, from here will be set by our get call
		"" // runtimeName
	};
	result = xrGetInstanceProperties(instance, &instanceProps);
	if (XR_FAILED(result)) {
		// not fatal probably
		print_line("OpenXR: Failed to get XR instance properties [", get_error_string(result), "]");
	} else {
		print_line("OpenXR: Running on OpenXR runtime: ", instanceProps.runtimeName, " ", make_xr_version_string(instanceProps.runtimeVersion));
	}

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_instance_created(instance);
	}

	return true;
}

bool OpenXRDevice::get_system_info() {
	// Retrieve basic OpenXR system info based on the form factor we desire

	// Retrieve the system for our form factor, fails if form factor is not available
	XrSystemGetInfo system_get_info = {
		XR_TYPE_SYSTEM_GET_INFO, // type;
		nullptr, // next
		form_factor // formFactor
	};

	XrResult result = xrGetSystem(instance, &system_get_info, &system_id);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get system for our form factor [", get_error_string(result), "]");
		return false;
	}

	// obtain info about our system, writing this out completely to make CI on Linux happy..
	XrSystemProperties system_properties = {
		XR_TYPE_SYSTEM_PROPERTIES, // type
		nullptr, // next
		0, // systemId, from here will be set by our get call
		0, // vendorId
		"", // systemName
		{
				0, // maxSwapchainImageHeight
				0, // maxSwapchainImageWidth
				0, // maxLayerCount
		}, // graphicsProperties
		{
				false, // orientationTracking
				false // positionTracking
		} // trackingProperties
	};

	void **property_pointer = &system_properties.next;
	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		void **property_next_pointer = wrapper->set_system_properties_and_get_next_pointer(property_pointer);
		if (*property_pointer && property_next_pointer && !*property_next_pointer) {
			property_pointer = property_next_pointer;
		} else {
			// Invalid return values.
			// Reset the value stored by the property_pointer so it can be reused in the next loop.
			*property_pointer = nullptr;
		}
	}

	result = xrGetSystemProperties(instance, system_id, &system_properties);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get System properties [", get_error_string(result), "]");
		return false;
	}

	// remember this state, we'll use it later
	system_name = String(system_properties.systemName);
	vendor_id = system_properties.vendorId;
	graphics_properties = system_properties.graphicsProperties;
	tracking_properties = system_properties.trackingProperties;

	return true;
}

bool OpenXRDevice::load_supported_view_configuration_types() {
	// This queries the supported configuration types, likely there will only be one chosing between Mono (phone AR) and Stereo (HMDs)

	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);

	if (supported_view_configuration_types != nullptr) {
		// free previous results
		memfree(supported_view_configuration_types);
		supported_view_configuration_types = nullptr;
	}

	XrResult result = xrEnumerateViewConfigurations(instance, system_id, 0, &num_view_configuration_types, nullptr);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get view configuration count [", get_error_string(result), "]");
		return false;
	}

	supported_view_configuration_types = (XrViewConfigurationType *)memalloc(sizeof(XrViewConfigurationType) * num_view_configuration_types);
	ERR_FAIL_NULL_V(supported_view_configuration_types, false);

	result = xrEnumerateViewConfigurations(instance, system_id, num_view_configuration_types, &num_view_configuration_types, supported_view_configuration_types);
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerateview configurations");

#ifdef DEBUG
	for (uint32_t i = 0; i < num_view_configuration_types; i++) {
		print_line("OpenXR: Found supported view configuration ", get_view_configuration_name(supported_view_configuration_types[i]));
	}
#endif

	return true;
}

bool OpenXRDevice::is_view_configuration_supported(XrViewConfigurationType p_configuration_type) const {
	ERR_FAIL_NULL_V(supported_view_configuration_types, false);

	for (uint32_t i = 0; i < num_view_configuration_types; i++) {
		if (supported_view_configuration_types[i] == p_configuration_type) {
			return true;
		}
	}

	return false;
}

bool OpenXRDevice::load_supported_view_configuration_views(XrViewConfigurationType p_configuration_type) {
	// This loads our view configuration for each view so for a stereo HMD, we'll get two entries (that are likely identical)
	// The returned data supplies us with the recommended render target size

	if (!is_view_configuration_supported(p_configuration_type)) {
		print_line("OpenXR: View configuration ", get_view_configuration_name(view_configuration), " is not supported.");
		return false;
	}

	if (view_configuration_views != nullptr) {
		// free previous results
		memfree(view_configuration_views);
		view_configuration_views = nullptr;
	}

	XrResult result = xrEnumerateViewConfigurationViews(instance, system_id, p_configuration_type, 0, &view_count, nullptr);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get view configuration count [", get_error_string(result), "]");
		return false;
	}

	view_configuration_views = (XrViewConfigurationView *)memalloc(sizeof(XrViewConfigurationView) * view_count);
	ERR_FAIL_NULL_V(view_configuration_views, false);

	for (uint32_t i = 0; i < view_count; i++) {
		view_configuration_views[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
		view_configuration_views[i].next = NULL;
	}

	result = xrEnumerateViewConfigurationViews(instance, system_id, p_configuration_type, view_count, &view_count, view_configuration_views);
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerate view configurations");

#ifdef DEBUG
	for (uint32_t i = 0; i < view_count; i++) {
		print_line("OpenXR: Found supported view configuration view");
		print_line(" - width: ", view_configuration_views[i].maxImageRectWidth);
		print_line(" - height: ", view_configuration_views[i].maxImageRectHeight);
		print_line(" - sample count: ", view_configuration_views[i].maxSwapchainSampleCount);
		print_line(" - recommended render width: ", view_configuration_views[i].recommendedImageRectWidth);
		print_line(" - recommended render height: ", view_configuration_views[i].recommendedImageRectHeight);
		print_line(" - recommended render sample count: ", view_configuration_views[i].recommendedSwapchainSampleCount);
	}
#endif

	return true;
}

void OpenXRDevice::destroy_instance() {
	if (view_configuration_views != nullptr) {
		memfree(view_configuration_views);
		view_configuration_views = nullptr;
	}

	if (supported_view_configuration_types != nullptr) {
		memfree(supported_view_configuration_types);
		supported_view_configuration_types = nullptr;
	}

	if (instance != XR_NULL_HANDLE) {
		for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
			wrapper->on_instance_destroyed();
		}

		xrDestroyInstance(instance);
		instance = XR_NULL_HANDLE;
	}
	enabled_extensions.clear();
}

bool OpenXRDevice::create_session() {
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);
	ERR_FAIL_COND_V(session != XR_NULL_HANDLE, false);

	XrSessionCreateInfo session_create_info = {
		XR_TYPE_SESSION_CREATE_INFO, // type
		nullptr, // next
		0, // createFlags
		system_id // systemId
	};

	void **property_pointer = const_cast<void **>(&session_create_info.next);
	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		void **property_next_pointer = wrapper->set_session_create_and_get_next_pointer(property_pointer);
		if (*property_pointer && property_next_pointer && !*property_next_pointer) {
			property_pointer = property_next_pointer;
		} else {
			// Invalid return values.
			// Reset the value stored by the property_pointer so it can be reused in the next loop.
			*property_pointer = nullptr;
		}
	}

	XrResult result = xrCreateSession(instance, &session_create_info, &session);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to create session [", get_error_string(result), "]");
		return false;
	}

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_session_created(session);
	}

	return true;
}

bool OpenXRDevice::load_supported_reference_spaces() {
	// loads the supported reference spaces for our OpenXR session

	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, false);

	if (supported_reference_spaces != nullptr) {
		// free previous results
		memfree(supported_reference_spaces);
		supported_reference_spaces = nullptr;
	}

	XrResult result = xrEnumerateReferenceSpaces(session, 0, &num_reference_spaces, nullptr);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get reference space count [", get_error_string(result), "]");
		return false;
	}

	supported_reference_spaces = (XrReferenceSpaceType *)memalloc(sizeof(XrReferenceSpaceType) * num_reference_spaces);
	ERR_FAIL_NULL_V(supported_reference_spaces, false);

	result = xrEnumerateReferenceSpaces(session, num_reference_spaces, &num_reference_spaces, supported_reference_spaces);
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerate reference spaces");

	// #ifdef DEBUG
	for (uint32_t i = 0; i < num_reference_spaces; i++) {
		print_line("OpenXR: Found supported reference space ", get_reference_space_name(supported_reference_spaces[i]));
	}
	// #endif

	return true;
}

bool OpenXRDevice::is_reference_space_supported(XrReferenceSpaceType p_reference_space) {
	ERR_FAIL_NULL_V(supported_reference_spaces, false);

	for (uint32_t i = 0; i < num_reference_spaces; i++) {
		if (supported_reference_spaces[i] == p_reference_space) {
			return true;
		}
	}

	return false;
}

bool OpenXRDevice::setup_spaces() {
	XrResult result;

	XrPosef identityPose = {
		{ 0.0, 0.0, 0.0, 1.0 },
		{ 0.0, 0.0, 0.0 }
	};

	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, false);

	// create play space
	{
		if (!is_reference_space_supported(reference_space)) {
			print_line("OpenXR: reference space ", get_reference_space_name(reference_space), " is not supported.");
			return false;
		}

		XrReferenceSpaceCreateInfo play_space_create_info = {
			XR_TYPE_REFERENCE_SPACE_CREATE_INFO, // type
			nullptr, // next
			reference_space, // referenceSpaceType
			identityPose // poseInReferenceSpace
		};

		result = xrCreateReferenceSpace(session, &play_space_create_info, &play_space);
		if (XR_FAILED(result)) {
			print_line("OpenXR: Failed to create play space [", get_error_string(result), "]");
			return false;
		}
	}

	// create view space
	{
		if (!is_reference_space_supported(XR_REFERENCE_SPACE_TYPE_VIEW)) {
			print_line("OpenXR: reference space XR_REFERENCE_SPACE_TYPE_VIEW is not supported.");
			return false;
		}

		XrReferenceSpaceCreateInfo view_space_create_info = {
			XR_TYPE_REFERENCE_SPACE_CREATE_INFO, // type
			nullptr, // next
			XR_REFERENCE_SPACE_TYPE_VIEW, // referenceSpaceType
			identityPose // poseInReferenceSpace
		};

		result = xrCreateReferenceSpace(session, &view_space_create_info, &view_space);
		if (XR_FAILED(result)) {
			print_line("OpenXR: Failed to create view space [", get_error_string(result), "]");
			return false;
		}
	}

	return true;
}

bool OpenXRDevice::load_supported_swapchain_formats() {
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, false);

	if (supported_swapchain_formats != nullptr) {
		// free previous results
		memfree(supported_swapchain_formats);
		supported_swapchain_formats = nullptr;
	}

	XrResult result = xrEnumerateSwapchainFormats(session, 0, &num_swapchain_formats, nullptr);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get swapchain format count [", get_error_string(result), "]");
		return false;
	}

	supported_swapchain_formats = (int64_t *)memalloc(sizeof(int64_t) * num_swapchain_formats);
	ERR_FAIL_NULL_V(supported_swapchain_formats, false);

	result = xrEnumerateSwapchainFormats(session, num_swapchain_formats, &num_swapchain_formats, supported_swapchain_formats);
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerate swapchain formats");

	// #ifdef DEBUG
	for (uint32_t i = 0; i < num_swapchain_formats; i++) {
		print_line("OpenXR: Found supported swapchain format ", get_swapchain_format_name(supported_swapchain_formats[i]));
	}
	// #endif

	return true;
}

bool OpenXRDevice::is_swapchain_format_supported(int64_t p_swapchain_format) {
	ERR_FAIL_NULL_V(supported_swapchain_formats, false);

	for (uint32_t i = 0; i < num_swapchain_formats; i++) {
		if (supported_swapchain_formats[i] == p_swapchain_format) {
			return true;
		}
	}

	return false;
}

bool OpenXRDevice::create_main_swapchain() {
	ERR_FAIL_NULL_V(graphics_extension, false);
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, false);

	/*
		TODO: We need to improve on this, for now we're taking our old approach of creating our main swapchains and substituting
		those for the ones Godot normally creates.
		This however means we can only use swapchains for our main XR view.

		It would have been nicer if we could override the swapchain creation in Godot with ours but we have a timing issue here.
		We can't create XR swapchains until after our XR session is fully instantiated, yet Godot creates its swapchain much earlier.

		Also Godot only creates a swapchain for the main output.
		OpenXR will require us to create swapchains as the render target for additional viewports if we want to use the layer system
		to optimise text rendering and background rendering as OpenXR may choose to re-use the results for reprojection while we're
		already rendering the next frame.

		Finally an area we need to expand upon is that Foveated rendering is only enabled for the swap chain we create,
		as we render 3D content into internal buffers that are copied into the swapchain, we don't get any of the performance gains
		until such time as we implement VRS.
	*/

	// Build a vector with swapchain formats we want to use, from best fit to worst
	Vector<int64_t> usable_swapchain_formats;
	int64_t swapchain_format_to_use = 0;

	graphics_extension->get_usable_swapchain_formats(usable_swapchain_formats);

	// now find out which one is supported
	for (int i = 0; i < usable_swapchain_formats.size() && swapchain_format_to_use == 0; i++) {
		if (is_swapchain_format_supported(usable_swapchain_formats[i])) {
			swapchain_format_to_use = usable_swapchain_formats[i];
		}
	}

	if (swapchain_format_to_use == 0) {
		swapchain_format_to_use = usable_swapchain_formats[0]; // just use the first one and hope for the best...
		print_line("Couldn't find usable swap chain format, using", get_swapchain_format_name(swapchain_format_to_use), "instead.");
	} else {
		print_line("Using swap chain format:", get_swapchain_format_name(swapchain_format_to_use));
	}

	Size2 recommended_size = get_recommended_target_size();

	if (!create_swapchain(swapchain_format_to_use, recommended_size.width, recommended_size.height, view_configuration_views[0].recommendedSwapchainSampleCount, view_count, swapchain, &swapchain_graphics_data)) {
		return false;
	}

	views = (XrView *)memalloc(sizeof(XrView) * view_count);
	ERR_FAIL_NULL_V_MSG(views, false, "OpenXR Couldn't allocate memory for views");

	projection_views = (XrCompositionLayerProjectionView *)memalloc(sizeof(XrCompositionLayerProjectionView) * view_count);
	ERR_FAIL_NULL_V_MSG(projection_views, false, "OpenXR Couldn't allocate memory for projection views");

	for (uint32_t i = 0; i < view_count; i++) {
		views[i].type = XR_TYPE_VIEW;
		views[i].next = NULL;

		projection_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
		projection_views[i].next = NULL;
		projection_views[i].subImage.swapchain = swapchain;
		projection_views[i].subImage.imageArrayIndex = i;
		projection_views[i].subImage.imageRect.offset.x = 0;
		projection_views[i].subImage.imageRect.offset.y = 0;
		projection_views[i].subImage.imageRect.extent.width = recommended_size.width;
		projection_views[i].subImage.imageRect.extent.height = recommended_size.height;
	};

	return true;
};

void OpenXRDevice::destroy_session() {
	if (running && session != XR_NULL_HANDLE) {
		xrEndSession(session);
	}

	if (graphics_extension) {
		graphics_extension->cleanup_swapchain_graphics_data(&swapchain_graphics_data);
	}

	if (views != nullptr) {
		memfree(views);
		views = nullptr;
	}

	if (projection_views != nullptr) {
		memfree(projection_views);
		projection_views = nullptr;
	}

	if (swapchain != XR_NULL_HANDLE) {
		xrDestroySwapchain(swapchain);
		swapchain = XR_NULL_HANDLE;
	}

	if (supported_swapchain_formats != nullptr) {
		memfree(supported_swapchain_formats);
		supported_swapchain_formats = nullptr;
	}

	// destroy our spaces
	if (play_space != XR_NULL_HANDLE) {
		xrDestroySpace(play_space);
		play_space = XR_NULL_HANDLE;
	}
	if (view_space != XR_NULL_HANDLE) {
		xrDestroySpace(view_space);
		view_space = XR_NULL_HANDLE;
	}

	if (supported_reference_spaces != nullptr) {
		// free previous results
		memfree(supported_reference_spaces);
		supported_reference_spaces = nullptr;
	}

	if (session != XR_NULL_HANDLE) {
		for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
			wrapper->on_session_destroyed();
		}

		xrDestroySession(session);
		session = XR_NULL_HANDLE;
	}
}

bool OpenXRDevice::create_swapchain(int64_t p_swapchain_format, uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size, XrSwapchain &r_swapchain, void **r_swapchain_graphics_data) {
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, false);
	ERR_FAIL_NULL_V(graphics_extension, false);

	XrResult result;

	XrSwapchainCreateInfo swapchain_create_info = {
		XR_TYPE_SWAPCHAIN_CREATE_INFO, // type
		nullptr, // next
		0, // createFlags
		XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT, // usageFlags
		p_swapchain_format, // format
		p_sample_count, // sampleCount
		p_width, // width
		p_height, // height
		1, // faceCount
		p_array_size, // arraySize
		1 // mipCount
	};

	void **swapchain_create_info_pointer = const_cast<void **>(&swapchain_create_info.next);
	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		void **swapchain_create_info_next_pointer = wrapper->set_swapchain_create_info_and_get_next_pointer(swapchain_create_info_pointer);
		if (*swapchain_create_info_pointer && swapchain_create_info_next_pointer && !*swapchain_create_info_next_pointer) {
			swapchain_create_info_pointer = swapchain_create_info_next_pointer;
		} else {
			// Invalid return values.
			// Reset the value stored by the swapchain_create_info_pointer so it can be reused in the next loop.
			*swapchain_create_info_pointer = nullptr;
		}
	}

	XrSwapchain new_swapchain;
	result = xrCreateSwapchain(session, &swapchain_create_info, &new_swapchain);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get swapchain [", get_error_string(result), "]");
		return false;
	}

	if (!graphics_extension->get_swapchain_image_data(new_swapchain, p_swapchain_format, p_width, p_height, p_array_size, p_array_size, r_swapchain_graphics_data)) {
		xrDestroySwapchain(new_swapchain);
		return false;
	}

	r_swapchain = new_swapchain;

	return true;
}

bool OpenXRDevice::on_state_idle() {
#ifdef DEBUG
	print_line("On state idle");
#endif

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_idle();
	}

	return true;
}

bool OpenXRDevice::on_state_ready() {
#ifdef DEBUG
	print_line("On state ready");
#endif

	// begin session
	XrSessionBeginInfo session_begin_info = {
		XR_TYPE_SESSION_BEGIN_INFO, // type
		nullptr, // next
		view_configuration // primaryViewConfigurationType
	};

	XrResult result = xrBeginSession(session, &session_begin_info);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to begin session [", get_error_string(result), "]");
		return false;
	}

	// This is when we create our swapchain, this can be a "long" time after Godot finishes, we can deal with this for now
	// but once we want to provide Viewports for additional layers where OpenXR requires us to create further swapchains,
	// we'll be creating those viewport WAY before we reach this point.
	// We may need to implement a wait in our init in main.cpp polling our events until the session is ready.
	// That will be very very ugly
	// The other possibility is to create a separate OpenXRViewport type specifically for this goal as part of our OpenXR module

	if (!create_main_swapchain()) {
		return false;
	}

	// we're running
	running = true;

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_ready();
	}

	// TODO emit signal

	// TODO Tell android

	return true;
}

bool OpenXRDevice::on_state_synchronized() {
#ifdef DEBUG
	print_line("On state synchronized");
#endif

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_synchronized();
	}

	return true;
}

bool OpenXRDevice::on_state_visible() {
#ifdef DEBUG
	print_line("On state visible");
#endif

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_visible();
	}

	// TODO emit signal

	return true;
}

bool OpenXRDevice::on_state_focused() {
#ifdef DEBUG
	print_line("On state focused");
#endif

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_focused();
	}

	// TODO emit signal

	return true;
}

bool OpenXRDevice::on_state_stopping() {
#ifdef DEBUG
	print_line("On state stopping");
#endif

	// TODO emit signal

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_stopping();
	}

	if (running) {
		XrResult result = xrEndSession(session);
		if (XR_FAILED(result)) {
			// we only report this..
			print_line("OpenXR: Failed to end session [", get_error_string(result), "]");
		}

		running = false;
	}

	// TODO further cleanup

	return true;
}

bool OpenXRDevice::on_state_loss_pending() {
#ifdef DEBUG
	print_line("On state loss pending");
#endif

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_loss_pending();
	}

	// TODO need to look into the correct action here, read up on the spec but we may need to signal Godot to exit (if it's not already exiting)

	return true;
}

bool OpenXRDevice::on_state_exiting() {
#ifdef DEBUG
	print_line("On state existing");
#endif

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_exiting();
	}

	// TODO need to look into the correct action here, read up on the spec but we may need to signal Godot to exit (if it's not already exiting)

	return true;
}

bool OpenXRDevice::is_initialized() {
	return (instance != XR_NULL_HANDLE);
}

bool OpenXRDevice::is_running() {
	if (instance == XR_NULL_HANDLE) {
		return false;
	}
	if (session == XR_NULL_HANDLE) {
		return false;
	}

	return running;
}

bool OpenXRDevice::initialise(const String &p_rendering_driver) {
	ERR_FAIL_COND_V_MSG(instance != XR_NULL_HANDLE, false, "OpenXR instance was already created");

	if (p_rendering_driver == "vulkan") {
#ifdef VULKAN_ENABLED
		graphics_extension = memnew(OpenXRVulkanExtension(this));
		register_extension_wrapper(graphics_extension);
#else
		// shouldn't be possible...
		ERR_FAIL_V(false);
#endif
	} else if (p_rendering_driver == "opengl3") {
#ifdef OPENGL3_ENABLED
		// graphics_extension = memnew(OpenXROpenGLExtension(this));
		// register_extension_wrapper(graphics_extension);
		ERR_FAIL_V_MSG(false, "OpenXR: OpenGL is not supported at this time.");
#else
		// shouldn't be possible...
		ERR_FAIL_V(false);
#endif
	} else {
		ERR_FAIL_V_MSG(false, "OpenXR: Unsupported rendering device.");
	}

	// initialise
	if (!load_layer_properties()) {
		destroy_instance();
		return false;
	}

	if (!load_supported_extensions()) {
		destroy_instance();
		return false;
	}

	if (!create_instance()) {
		destroy_instance();
		return false;
	}

	if (!get_system_info()) {
		destroy_instance();
		return false;
	}

	if (!load_supported_view_configuration_types()) {
		destroy_instance();
		return false;
	}

	if (!load_supported_view_configuration_views(view_configuration)) {
		destroy_instance();
		return false;
	}

	return true;
}

bool OpenXRDevice::initialise_session() {
	if (!create_session()) {
		destroy_session();
		return false;
	}

	if (!load_supported_reference_spaces()) {
		destroy_session();
		return false;
	}

	if (!setup_spaces()) {
		destroy_session();
		return false;
	}

	if (!load_supported_swapchain_formats()) {
		destroy_session();
		return false;
	}

	return true;
}

void OpenXRDevice::finish() {
	destroy_session();

	destroy_instance();
}

void OpenXRDevice::register_extension_wrapper(OpenXRExtensionWrapper *p_extension_wrapper) {
	registered_extension_wrappers.push_back(p_extension_wrapper);
}

Size2 OpenXRDevice::get_recommended_target_size() {
	ERR_FAIL_NULL_V(view_configuration_views, Size2());

	Size2 target_size;

	target_size.width = view_configuration_views[0].recommendedImageRectWidth;
	target_size.height = view_configuration_views[0].recommendedImageRectHeight;

	return target_size;
}

OpenXRDevice::TrackingConfidence OpenXRDevice::get_head_center(Transform3D &r_transform, Vector3 &r_linear_velocity, Vector3 &r_angular_velocity) {
	XrResult result;

	ERR_FAIL_COND_V(!running, TRACKING_CONFIDENCE_NO);

	// xrWaitFrame not run yet
	if (frame_state.predictedDisplayTime == 0) {
		return TRACKING_CONFIDENCE_NO;
	}

	// Get timing for the next frame, as that is the current frame we're processing
	XrTime display_time = get_next_frame_time();

	XrSpaceVelocity velocity = {
		XR_TYPE_SPACE_VELOCITY, // type
		nullptr, // next
		0, // velocityFlags
		{ 0.0, 0.0, 0.0 }, // linearVelocity
		{ 0.0, 0.0, 0.0 } // angularVelocity
	};

	XrSpaceLocation location = {
		XR_TYPE_SPACE_LOCATION, // type
		&velocity, // next
		0, // locationFlags
		{
				{ 0.0, 0.0, 0.0, 0.0 }, // orientation
				{ 0.0, 0.0, 0.0 } // position
		} // pose
	};

	result = xrLocateSpace(view_space, play_space, display_time, &location);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to locate view space in play space [", get_error_string(result), "]");
		return TRACKING_CONFIDENCE_NO;
	}

	TrackingConfidence confidence = transform_from_location(location, r_transform);
	parse_velocities(velocity, r_linear_velocity, r_angular_velocity);

	if (head_pose_confidence != confidence) {
		// prevent error spam
		head_pose_confidence = confidence;
		if (head_pose_confidence == TRACKING_CONFIDENCE_NO) {
			print_line("OpenXR head space location not valid (check tracking?)");
#ifdef DEBUG
		} else if (head_pose_confidence == TRACKING_CONFIDENCE_LOW) {
			print_line("OpenVR Head pose now tracking with low confidence");
		} else {
			print_line("OpenVR Head pose now tracking with high confidence");
#endif
		}
	}

	return confidence;
}

bool OpenXRDevice::get_view_transform(uint32_t p_view, Transform3D &r_transform) {
	ERR_FAIL_COND_V(!running, false);

	// xrWaitFrame not run yet
	if (frame_state.predictedDisplayTime == 0) {
		return false;
	}

	if (views == NULL || !view_pose_valid) {
		return false;
	}

	// Note, the timing of this is set right before rendering, which is what we need here.
	r_transform = transform_from_pose(views[p_view].pose);

	return true;
}

bool OpenXRDevice::get_view_projection(uint32_t p_view, double p_z_near, double p_z_far, CameraMatrix &p_camera_matrix) {
	ERR_FAIL_COND_V(!running, false);
	ERR_FAIL_NULL_V(graphics_extension, false);

	// TODO maybe move this into process, which is called first...

	XrViewLocateInfo view_locate_info = {
		XR_TYPE_VIEW_LOCATE_INFO, // type
		nullptr, // next
		view_configuration, // viewConfigurationType
		frame_state.predictedDisplayTime, // displayTime
		play_space // space
	};
	XrViewState view_state = {
		XR_TYPE_VIEW_STATE, // type
		nullptr, // next
		0 // viewStateFlags
	};
	uint32_t view_count_output;

	XrResult result = xrLocateViews(session, &view_locate_info, &view_state, view_count, &view_count_output, views);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Could not locate views [", get_error_string(result), "]");
		return false;
	}

	return graphics_extension->create_projection_fov(views[p_view].fov, p_z_near, p_z_far, p_camera_matrix);
}

bool OpenXRDevice::poll_events() {
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);

	XrEventDataBuffer runtimeEvent;
	runtimeEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
	runtimeEvent.next = nullptr;
	// runtimeEvent.varying = ...

	XrResult pollResult = xrPollEvent(instance, &runtimeEvent);
	while (pollResult == XR_SUCCESS) {
		bool handled = false;
		for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
			handled |= wrapper->on_event_polled(runtimeEvent);
		}
		switch (runtimeEvent.type) {
			// case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
			// } break;
			// case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR: {
			// } break;
			// case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
			// } break;
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				XrEventDataSessionStateChanged *event = (XrEventDataSessionStateChanged *)&runtimeEvent;

				session_state = event->state;
				if (session_state >= XR_SESSION_STATE_MAX_ENUM) {
					print_line("OpenXR EVENT: session state changed to UNKNOWN -", session_state);
				} else {
					print_line("OpenXR EVENT: session state changed to", get_session_state_name(session_state));

					switch (session_state) {
						case XR_SESSION_STATE_IDLE:
							on_state_idle();
							break;
						case XR_SESSION_STATE_READY:
							on_state_ready();
							break;
						case XR_SESSION_STATE_SYNCHRONIZED:
							on_state_synchronized();
							break;
						case XR_SESSION_STATE_VISIBLE:
							on_state_visible();
							break;
						case XR_SESSION_STATE_FOCUSED:
							on_state_focused();
							break;
						case XR_SESSION_STATE_STOPPING:
							on_state_stopping();
							break;
						case XR_SESSION_STATE_LOSS_PENDING:
							on_state_loss_pending();
							break;
						case XR_SESSION_STATE_EXITING:
							on_state_exiting();
							break;
						default:
							break;
					}
				}
			} break;
			// case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
			// } break;
			// case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
			// } break;
			default:
				if (!handled) {
					print_line("OpenXR Unhandled event type", get_structure_type_name(runtimeEvent.type));
				}
				break;
		}

		runtimeEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
		pollResult = xrPollEvent(instance, &runtimeEvent);
	}

	if (pollResult == XR_EVENT_UNAVAILABLE) {
		// processed all events in the queue
		return true;
	} else {
		ERR_FAIL_V_MSG(false, "OpenXR: Failed to poll events!");
	}
}

bool OpenXRDevice::process() {
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);

	if (!poll_events()) {
		return false;
	}

	if (!running) {
		return false;
	}

	// update_actions();

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_process();
	}

	return true;
}

bool OpenXRDevice::acquire_image(XrSwapchain p_swapchain, uint32_t &r_image_index) {
	ERR_FAIL_COND_V(image_acquired, true); // this was not released when it should be, error out and re-use...

	XrResult result;
	XrSwapchainImageAcquireInfo swapchain_image_acquire_info = {
		XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, // type
		nullptr // next
	};
	result = xrAcquireSwapchainImage(p_swapchain, &swapchain_image_acquire_info, &r_image_index);
	if (XR_FAILED(result)) {
		print_line("OpenXR: failed to acquire swapchain image [", get_error_string(result), "]");
		return false;
	}

	XrSwapchainImageWaitInfo swapchain_image_wait_info = {
		XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, // type
		nullptr, // next
		17000000 // timeout in nanoseconds
	};

	result = xrWaitSwapchainImage(p_swapchain, &swapchain_image_wait_info);
	if (XR_FAILED(result)) {
		print_line("OpenXR: failed to wait for swapchain image [", get_error_string(result), "]");
		return false;
	}

	return true;
}

bool OpenXRDevice::release_image(XrSwapchain p_swapchain) {
	XrSwapchainImageReleaseInfo swapchain_image_release_info = {
		XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, // type
		nullptr // next
	};
	XrResult result = xrReleaseSwapchainImage(swapchain, &swapchain_image_release_info);
	if (XR_FAILED(result)) {
		print_line("OpenXR: failed to release swapchain image! [", get_error_string(result), "]");
		return false;
	}

	return true;
}

void OpenXRDevice::pre_render() {
	ERR_FAIL_COND(instance == XR_NULL_HANDLE);

	if (!running) {
		return;
	}

	// Waitframe does 2 important things in our process:
	// 1) It provides us with predictive timing, telling us when OpenXR expects to display the frame we're about to commit
	// 2) It will use the previous timing to pause our thread so that rendering starts as close to displaying as possible
	// This must thus be called as close to when we start rendering as possible
	XrFrameWaitInfo frame_wait_info = { XR_TYPE_FRAME_WAIT_INFO, nullptr };
	XrResult result = xrWaitFrame(session, &frame_wait_info, &frame_state);
	if (XR_FAILED(result)) {
		print_line("OpenXR: xrWaitFrame() was not successful [", get_error_string(result), "]");
		return;
	}

	for (OpenXRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_pre_render();
	}

	// Get our view info for the frame we're about to render
	XrViewLocateInfo view_locate_info = {
		XR_TYPE_VIEW_LOCATE_INFO, // type
		nullptr, // next
		view_configuration, // viewConfigurationType
		frame_state.predictedDisplayTime, // displayTime
		play_space // space
	};
	XrViewState view_state = {
		XR_TYPE_VIEW_STATE, // type
		nullptr, // next
		0 // viewStateFlags
	};
	uint32_t view_count_output;
	result = xrLocateViews(session, &view_locate_info, &view_state, view_count, &view_count_output, views);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Couldn't locate views [", get_error_string(result), "]");
		return;
	}

	bool pose_valid = true;
	for (uint64_t i = 0; i < view_count_output; i++) {
		if ((view_state.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0 ||
				(view_state.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0) {
			pose_valid = false;
		}
	}
	if (view_pose_valid != pose_valid) {
		view_pose_valid = pose_valid;
#ifdef DEBUG
		if (!view_pose_valid) {
			print_line("OpenXR View pose became invalid");
		} else {
			print_line("OpenXR View pose became valid");
		}
#endif
	}

	if (frame_state.shouldRender) {
		// let's start our frame..
		XrFrameBeginInfo frame_begin_info = {
			XR_TYPE_FRAME_BEGIN_INFO, // type
			nullptr // next
		};
		result = xrBeginFrame(session, &frame_begin_info);
		if (XR_FAILED(result)) {
			print_line("OpenXR: failed to being frame [", get_error_string(result), "]");
			return;
		}
	}
}

bool OpenXRDevice::pre_draw_viewport(RID p_render_target) {
	if (!can_render()) {
		return false;
	}

	// TODO: at some point in time we may support multiple viewports in which case we need to handle that...

	return true;
}

void OpenXRDevice::post_draw_viewport(RID p_render_target) {
	if (!can_render()) {
		return;
	}

	// TODO: at some point in time we may support multiple viewports in which case we need to handle that...

	// TODO: if we can get PR 51179 to work properly we can change away from this approach and move this into get_external_texture or something
	if (!image_acquired) {
		if (!acquire_image(swapchain, image_index)) {
			return;
		}
		image_acquired = true;

		// print_line("OpenXR: acquired image " + itos(image_index) + ", copying...");

		// Copy our buffer into our swap chain (remove once PR 51179 is done)
		graphics_extension->copy_render_target_to_image(p_render_target, swapchain_graphics_data, image_index);
	}
};

void OpenXRDevice::end_frame() {
	XrResult result;

	ERR_FAIL_COND(instance == XR_NULL_HANDLE);

	if (!running) {
		return;
	}

	if (frame_state.shouldRender && view_pose_valid && !image_acquired) {
		print_line("OpenXR: No viewport was marked with use_xr, there is no rendered output!");
	}

	// must have:
	// - shouldRender set to true
	// - a valid view pose for projection_views[eye].pose to submit layer
	// - an image to render
	if (!frame_state.shouldRender || !view_pose_valid || !image_acquired) {
		// submit 0 layers when we shouldn't render
		XrFrameEndInfo frame_end_info = {
			XR_TYPE_FRAME_END_INFO, // type
			nullptr, // next
			frame_state.predictedDisplayTime, // displayTime
			XR_ENVIRONMENT_BLEND_MODE_OPAQUE, // environmentBlendMode
			0, // layerCount
			nullptr // layers
		};
		result = xrEndFrame(session, &frame_end_info);
		if (XR_FAILED(result)) {
			print_line("OpenXR: failed to end frame! [", get_error_string(result), "]");
			return;
		}

		// neither eye is rendered
		return;
	}

	// release our swapchain image if we acquired it
	if (image_acquired) {
		image_acquired = false; // whether we succeed or not, consider this released.

		release_image(swapchain);
	}

	for (uint32_t eye = 0; eye < view_count; eye++) {
		projection_views[eye].fov = views[eye].fov;
		projection_views[eye].pose = views[eye].pose;
	}

	Vector<const XrCompositionLayerBaseHeader *> layers_list;

	// Add composition layers from providers
	for (OpenXRCompositionLayerProvider *provider : composition_layer_providers) {
		XrCompositionLayerBaseHeader *layer = provider->get_composition_layer();
		if (layer) {
			layers_list.push_back(layer);
		}
	}

	XrCompositionLayerProjection projection_layer = {
		XR_TYPE_COMPOSITION_LAYER_PROJECTION, // type
		nullptr, // next
		layers_list.size() > 1 ? XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT : XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT, // layerFlags
		play_space, // space
		view_count, // viewCount
		projection_views, // views
	};
	layers_list.push_back((const XrCompositionLayerBaseHeader *)&projection_layer);

	XrFrameEndInfo frame_end_info = {
		XR_TYPE_FRAME_END_INFO, // type
		nullptr, // next
		frame_state.predictedDisplayTime, // displayTime
		XR_ENVIRONMENT_BLEND_MODE_OPAQUE, // environmentBlendMode
		static_cast<uint32_t>(layers_list.size()), // layerCount
		layers_list.ptr() // layers
	};
	result = xrEndFrame(session, &frame_end_info);
	if (XR_FAILED(result)) {
		print_line("OpenXR: failed to end frame! [", get_error_string(result), "]");
		return;
	}
}

OpenXRDevice::OpenXRDevice() {
	// OpenXRDevice is only constructed if OpenXR is enabled.
	// It will be constructed when the rendering device first accesses OpenXR (be it the Vulkan or OpenGL rendering system)

	if (Engine::get_singleton()->is_editor_hint()) {
		// Enabled OpenXR in the editor? Adjust our settings for the editor

	} else {
		// Load settings from project settings
	}

	// reset a few things that can't be done in our class definition
	frame_state.predictedDisplayTime = 0;
	frame_state.predictedDisplayPeriod = 0;

#ifdef ANDROID_ENABLED
	// our android wrapper will initialise our android loader at this point
	register_extension_wrapper(memnew(OpenXRAndroidExtension(this)));
#endif
}

OpenXRDevice::~OpenXRDevice() {
	// cleanup our composition layer providers
	for (OpenXRCompositionLayerProvider *provider : composition_layer_providers) {
		memdelete(provider);
	}
	composition_layer_providers.clear();

	// cleanup our extension wrappers
	for (OpenXRExtensionWrapper *extension_wrapper : registered_extension_wrappers) {
		memdelete(extension_wrapper);
	}
	registered_extension_wrappers.clear();

	if (supported_extensions != nullptr) {
		memfree(supported_extensions);
		supported_extensions = nullptr;
	}

	if (layer_properties != nullptr) {
		memfree(layer_properties);
		layer_properties = nullptr;
	}
}

Transform3D OpenXRDevice::transform_from_pose(const XrPosef &p_pose) {
	Quaternion q(p_pose.orientation.x, p_pose.orientation.y, p_pose.orientation.z, p_pose.orientation.w);
	Basis basis(q);
	Vector3 origin(p_pose.position.x, p_pose.position.y, p_pose.position.z);

	return Transform3D(basis, origin);
}

template <typename T>
OpenXRDevice::TrackingConfidence _transform_from_location(const T &p_location, Transform3D &r_transform) {
	Basis basis;
	Vector3 origin;
	OpenXRDevice::TrackingConfidence confidence = OpenXRDevice::TRACKING_CONFIDENCE_NO;
	const auto &pose = p_location.pose;

	// Check orientation
	if (p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
		Quaternion q(pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);
		r_transform.basis = Basis(q);

		if (p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) {
			// Fully valid orientation, so either 3DOF or 6DOF tracking with high confidence so default to HIGH_TRACKING
			confidence = OpenXRDevice::TRACKING_CONFIDENCE_HIGH;
		} else {
			// Orientation is being tracked but we're using old/predicted data, so low tracking confidence
			confidence = OpenXRDevice::TRACKING_CONFIDENCE_LOW;
		}
	} else {
		r_transform.basis = Basis();
	}

	// Check location
	if (p_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
		r_transform.origin = Vector3(pose.position.x, pose.position.y, pose.position.z);

		if (!(p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT)) {
			// Location is being tracked but we're using old/predicted data, so low tracking confidence
			confidence = OpenXRDevice::TRACKING_CONFIDENCE_LOW;
		} else if (confidence == OpenXRDevice::TRACKING_CONFIDENCE_NO) {
			// Position tracking without orientation tracking?
			confidence = OpenXRDevice::TRACKING_CONFIDENCE_HIGH;
		}
	} else {
		// No tracking or 3DOF I guess..
		r_transform.origin = Vector3();
	}

	return confidence;
}

OpenXRDevice::TrackingConfidence OpenXRDevice::transform_from_location(const XrSpaceLocation &p_location, Transform3D &r_transform) {
	return _transform_from_location(p_location, r_transform);
}

OpenXRDevice::TrackingConfidence OpenXRDevice::transform_from_location(const XrHandJointLocationEXT &p_location, Transform3D &r_transform) {
	return _transform_from_location(p_location, r_transform);
}

void OpenXRDevice::parse_velocities(const XrSpaceVelocity &p_velocity, Vector3 &r_linear_velocity, Vector3 r_angular_velocity) {
	if (p_velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT) {
		XrVector3f linear_velocity = p_velocity.linearVelocity;
		r_linear_velocity = Vector3(linear_velocity.x, linear_velocity.y, linear_velocity.z);
	} else {
		r_linear_velocity = Vector3();
	}
	if (p_velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT) {
		XrVector3f angular_velocity = p_velocity.angularVelocity;
		r_angular_velocity = Vector3(angular_velocity.x, angular_velocity.y, angular_velocity.z);
	} else {
		r_angular_velocity = Vector3();
	}
}
