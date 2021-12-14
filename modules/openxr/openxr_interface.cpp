/*************************************************************************/
/*  openxr_interface.cpp                                                 */
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

#include "openxr_interface.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "servers/rendering/rendering_server_globals.h"

void OpenXRInterface::_bind_methods() {
	// todo
}

StringName OpenXRInterface::get_name() const {
	return StringName("OpenXR");
};

uint32_t OpenXRInterface::get_capabilities() const {
	return XRInterface::XR_VR + XRInterface::XR_STEREO;
};

XRInterface::TrackingStatus OpenXRInterface::get_tracking_status() const {
	return tracking_state;
}

void OpenXRInterface::_load_action_sets() {
	ERR_FAIL_NULL(openxr_device);

	Ref<OpenXRActionSets> action_sets;
	if (Engine::get_singleton()->is_editor_hint()) {
#ifdef TOOLS_ENABLED
		action_sets.instantiate();
		action_sets->create_editor_action_sets();
#endif
	} else {
		String default_tres_name = openxr_device->get_default_action_sets_resource_name();

		// Check if we can load our default
		if (ResourceLoader::exists(default_tres_name)) {
			action_sets = ResourceLoader::load(default_tres_name);
		}

		// Cechk if we need to create default action set
		if (action_sets.is_null()) {
			action_sets.instantiate();
			action_sets->create_default_action_sets();
#ifdef TOOLS_ENABLED
			// Save our action sets so our user can
			action_sets->set_path(default_tres_name, true);
			ResourceSaver::save(default_tres_name, action_sets);
#endif
		}
	}

	// Now register our action sets and build up our data structures so we can start tracking our controllers
}

bool OpenXRInterface::initialise_on_startup() const {
	if (openxr_device == nullptr) {
		return false;
	} else if (!openxr_device->is_initialized()) {
		return false;
	} else {
		return true;
	}
}

bool OpenXRInterface::is_initialized() const {
	return initialized;
};

bool OpenXRInterface::initialize() {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, false);

	if (openxr_device == nullptr) {
		return false;
	} else if (!openxr_device->is_initialized()) {
		return false;
	} else if (initialized) {
		return true;
	}

	// load up our action sets before setting up our session, note that our profiles are suggestions, OpenXR takes ownership of (re)binding
	_load_action_sets();

	if (!openxr_device->initialise_session()) {
		return false;
	}

	// we must create a tracker for our head
	head.instantiate();
	head->set_tracker_type(XRServer::TRACKER_HEAD);
	head->set_tracker_name("head");
	head->set_tracker_desc("Players head");
	xr_server->add_tracker(head);

	// make this our primary interface
	xr_server->set_primary_interface(this);

	initialized = true;

	return initialized;
}

void OpenXRInterface::uninitialize() {
	// Our OpenXR driver will clean itself up properly when Godot exits, so we just do some basic stuff here

	XRServer *xr_server = XRServer::get_singleton();
	if (xr_server) {
		if (head.is_valid()) {
			xr_server->remove_tracker(head);

			head.unref();
		}
	}

	initialized = false;
}

bool OpenXRInterface::supports_play_area_mode(XRInterface::PlayAreaMode p_mode) {
	return false;
}

XRInterface::PlayAreaMode OpenXRInterface::get_play_area_mode() const {
	return XRInterface::XR_PLAY_AREA_UNKNOWN;
}

bool OpenXRInterface::set_play_area_mode(XRInterface::PlayAreaMode p_mode) {
	return false;
}

Size2 OpenXRInterface::get_render_target_size() {
	if (openxr_device == nullptr) {
		return Size2();
	} else {
		return openxr_device->get_recommended_target_size();
	}
}

uint32_t OpenXRInterface::get_view_count() {
	// TODO set this based on our configuration
	return 2;
}

void OpenXRInterface::_set_default_pos(Transform3D &p_transform, double p_world_scale, uint64_t p_eye) {
	p_transform = Transform3D();

	// if we're not tracking, don't put our head on the floor...
	p_transform.origin.y = 1.5 * p_world_scale;

	// overkill but..
	if (p_eye == 1) {
		p_transform.origin.x = 0.03 * p_world_scale;
	} else if (p_eye == 2) {
		p_transform.origin.x = -0.03 * p_world_scale;
	}
}

Transform3D OpenXRInterface::get_camera_transform() {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, Transform3D());

	Transform3D hmd_transform;
	double world_scale = xr_server->get_world_scale();

	// head_transform should be updated in process

	hmd_transform.basis = head_transform.basis;
	hmd_transform.origin = head_transform.origin * world_scale;

	return hmd_transform;
}

Transform3D OpenXRInterface::get_transform_for_view(uint32_t p_view, const Transform3D &p_cam_transform) {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, Transform3D());

	Transform3D t;
	if (openxr_device && openxr_device->get_view_transform(p_view, t)) {
		// update our cached value if we have a valid transform
		transform_for_view[p_view] = t;
	} else {
		// reuse cached value
		t = transform_for_view[p_view];
	}

	// Apply our world scale
	double world_scale = xr_server->get_world_scale();
	t.origin *= world_scale;

	return p_cam_transform * xr_server->get_reference_frame() * t;
}

CameraMatrix OpenXRInterface::get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) {
	CameraMatrix cm;

	if (openxr_device) {
		if (openxr_device->get_view_projection(p_view, p_z_near, p_z_far, cm)) {
			return cm;
		}
	}

	// Failed to get from our OpenXR device? Default to some sort of sensible camera matrix..
	cm.set_for_hmd(p_view + 1, 1.0, 6.0, 14.5, 4.0, 1.5, p_z_near, p_z_far);

	return cm;
}

void OpenXRInterface::process() {
	if (openxr_device) {
		if (openxr_device->process()) {
			Transform3D t;
			Vector3 linear_velocity;
			Vector3 angular_velocity;
			OpenXRDevice::TrackingConfidence confidence = openxr_device->get_head_center(t, linear_velocity, angular_velocity);
			if (confidence != OpenXRDevice::TRACKING_CONFIDENCE_NO) {
				// Only update our transform if we have one to update it with
				// note that poses are stored without world scale and reference frame applied!
				head_transform = t;
				head_linear_velocity = linear_velocity;
				head_angular_velocity = angular_velocity;
			}
		}
	}

	if (head.is_valid()) {
		// TODO figure out how to get our velocities

		head->set_pose("default", head_transform, head_linear_velocity, head_angular_velocity);

		// TODO set confidence on pose once we support tracking this..
	}
}

void OpenXRInterface::pre_render() {
	if (openxr_device) {
		openxr_device->pre_render();
	}
}

bool OpenXRInterface::pre_draw_viewport(RID p_render_target) {
	if (openxr_device) {
		return openxr_device->pre_draw_viewport(p_render_target);
	} else {
		// don't render
		return false;
	}
}

Vector<BlitToScreen> OpenXRInterface::post_draw_viewport(RID p_render_target, const Rect2 &p_screen_rect) {
	Vector<BlitToScreen> blit_to_screen;

	// If separate HMD we should output one eye to screen
	if (p_screen_rect != Rect2()) {
		BlitToScreen blit;

		blit.render_target = p_render_target;
		blit.multi_view.use_layer = true;
		blit.multi_view.layer = 0;
		blit.lens_distortion.apply = false;

		Size2 render_size = get_render_target_size();
		Rect2 dst_rect = p_screen_rect;
		float new_height = dst_rect.size.x * (render_size.y / render_size.x);
		if (new_height > dst_rect.size.y) {
			dst_rect.position.y = (0.5 * dst_rect.size.y) - (0.5 * new_height);
			dst_rect.size.y = new_height;
		} else {
			float new_width = dst_rect.size.y * (render_size.x / render_size.y);

			dst_rect.position.x = (0.5 * dst_rect.size.x) - (0.5 * new_width);
			dst_rect.size.x = new_width;
		}

		blit.dst_rect = dst_rect;
		blit_to_screen.push_back(blit);
	}

	if (openxr_device) {
		openxr_device->post_draw_viewport(p_render_target);
	}

	return blit_to_screen;
}

void OpenXRInterface::end_frame() {
	if (openxr_device) {
		openxr_device->end_frame();
	}
}

OpenXRInterface::OpenXRInterface() {
	openxr_device = OpenXRDevice::get_singleton();

	// while we don't have head tracking, don't put the headset on the floor...
	_set_default_pos(head_transform, 1.0, 0);
	_set_default_pos(transform_for_view[0], 1.0, 1);
	_set_default_pos(transform_for_view[1], 1.0, 2);
}

OpenXRInterface::~OpenXRInterface() {
	openxr_device = nullptr;
}
