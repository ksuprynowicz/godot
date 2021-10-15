/*************************************************************************/
/*  editor_importer_point_parent_to_child.h                              */
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

#ifndef RESOURCE_IMPORTER_POINT_PARENT_TO_CHILD_H
#define RESOURCE_IMPORTER_POINT_PARENT_TO_CHILD_H
#include "core/math/transform_3d.h"
#include "resource_importer_scene.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_player.h"

class PointParentToChild {
	struct RestBone {
		Transform3D rest_local_before;
		Transform3D rest_local_after;
		Basis rest_delta;
		Vector3 children_centroid_direction;
		int parent_index;
		Vector<int> children;
	};

public:
	void _parent_to_child(Node *scene);

private:
	void _fix_meshes(Map<StringName, PointParentToChild::RestBone> &r_rest_bones, Vector<Node3D *> p_meshes);
	void _fix_skeleton(Skeleton3D *p_skeleton, Map<StringName, PointParentToChild::RestBone> &r_rest_bones);
	void _align_animations(AnimationPlayer *p_ap, const Map<StringName, PointParentToChild::RestBone> &p_rest_bones);
	Vector3 _get_perpendicular_vector(Vector3 v);
	Quaternion _align_vectors(Vector3 a, Vector3 b);
};

#endif
