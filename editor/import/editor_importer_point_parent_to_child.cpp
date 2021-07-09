/*************************************************************************/
/*  editor_importer_point_parent_to_child.cpp                            */
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

#include "editor/import/editor_importer_point_parent_to_child.h"
#include "core/math/transform_3d.h"
#include "editor/import/scene_importer_mesh_node_3d.h"
#include "resource_importer_scene.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_player.h"

void PointParentToChild::_parent_to_child(Node *scene) {
	Map<int, RestBone> r_rest_bones;
	Vector<Node3D *> r_meshes;
	List<Node *> queue;
	queue.push_back(scene);
	while (!queue.is_empty()) {
		List<Node *>::Element *E = queue.front();
		Node *node = E->get();
		if (Object::cast_to<Skeleton3D>(node)) {
			Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(node);
			_point_parent_to_child_fix_skeleton(skeleton, r_rest_bones);
		}
		EditorSceneImporterMeshNode3D *editor_mesh_3d = Object::cast_to<EditorSceneImporterMeshNode3D>(node);
		MeshInstance3D *mesh_3d = Object::cast_to<MeshInstance3D>(node);
		if (editor_mesh_3d) {
			NodePath path = editor_mesh_3d->get_skeleton_path();
			if (!path.is_empty() && editor_mesh_3d->get_node_or_null(path) && Object::cast_to<Skeleton3D>(editor_mesh_3d->get_node_or_null(path))) {
				r_meshes.push_back(editor_mesh_3d);
			}
		} else if (mesh_3d) {
			NodePath path = mesh_3d->get_skeleton_path();
			if (!path.is_empty() && mesh_3d->get_node_or_null(path) && Object::cast_to<Skeleton3D>(mesh_3d->get_node_or_null(path))) {
				r_meshes.push_back(mesh_3d);
			}
		}
		int child_count = node->get_child_count();
		for (int i = 0; i < child_count; i++) {
			queue.push_back(node->get_child(i));
		}
		queue.pop_front();
	}
	_point_parent_to_child_fix_meshes(r_rest_bones, r_meshes);

	queue.clear();
	queue.push_back(scene);
	while (!queue.is_empty()) {
		List<Node *>::Element *E = queue.front();
		Node *node = E->get();
		AnimationPlayer *ap = Object::cast_to<AnimationPlayer>(node);
		_point_parent_to_child_align_animations(ap, r_rest_bones);

		int child_count = node->get_child_count();
		for (int i = 0; i < child_count; i++) {
			queue.push_back(node->get_child(i));
		}
		queue.pop_front();
	}
}

Vector3 PointParentToChild::_point_parent_to_child_get_perpendicular_vector(Vector3 v) {
	Vector3 perpendicular;
	if (v[0] != 0 && v[1] != 0) {
		perpendicular = Vector3(0, 0, 1).cross(v).normalized();
	} else {
		perpendicular = Vector3(1, 0, 0);
	}
	return perpendicular;
}

Quaternion PointParentToChild::_point_parent_to_child_align_vectors(Vector3 a, Vector3 b) {
	a.normalize();
	b.normalize();
	if (a.length_squared() != 0 && b.length_squared() != 0) {
		//Find the axis perpendicular to both vectors and rotate along it by the angular difference
		Vector3 perpendicular = a.cross(b).normalized();
		float angleDiff = a.angle_to(b);
		if (perpendicular.length_squared() == 0) {
			perpendicular = _point_parent_to_child_get_perpendicular_vector(a);
		}
		return Quaternion(perpendicular, angleDiff);
	} else {
		return Quaternion();
	}
}

void PointParentToChild::_point_parent_to_child_fix_skeleton(Skeleton3D *p_skeleton, Map<int, PointParentToChild::RestBone> &r_rest_bones) {
	int bone_count = p_skeleton->get_bone_count();

	//First iterate through all the bones and create a RestBone for it with an empty centroid
	for (int j = 0; j < bone_count; j++) {
		RestBone rest_bone;

		String path = p_skeleton->get_name();
		Node *current_node = p_skeleton->get_parent();
		while (current_node && current_node != p_skeleton->get_owner()) {
			path = String(current_node->get_name()) + "/" + path;
			current_node = current_node->get_parent();
		}
		rest_bone.path = String(path) + String(":") + p_skeleton->get_bone_name(j);
		rest_bone.parent_index = p_skeleton->get_bone_parent(j);
		rest_bone.rest_local_before = p_skeleton->get_bone_rest(j);
		rest_bone.rest_local_after = rest_bone.rest_local_before;
		r_rest_bones.insert(j, rest_bone);
	}

	//We iterate through again, and add the child's position to the centroid of its parent.
	//These position are local to the parent which means (0, 0, 0) is right where the parent is.
	for (int i = 0; i < bone_count; i++) {
		int parent_bone = p_skeleton->get_bone_parent(i);
		if (parent_bone >= 0) {
			r_rest_bones[parent_bone].children_centroid_direction = r_rest_bones[parent_bone].children_centroid_direction + p_skeleton->get_bone_rest(i).origin;
			r_rest_bones[parent_bone].children.push_back(i);
		}
	}

	//Point leaf bones to parent
	for (int i = 0; i < bone_count; i++) {
		PointParentToChild::RestBone &leaf_bone = r_rest_bones[i];
		if (!leaf_bone.children.size()) {
			leaf_bone.children_centroid_direction = r_rest_bones[leaf_bone.parent_index].children_centroid_direction;
		}
	}

	//We iterate again to point each bone to the centroid
	//When we rotate a bone, we also have to move all of its children in the opposite direction
	for (int i = 0; i < bone_count; i++) {
		r_rest_bones[i].rest_delta = _point_parent_to_child_align_vectors(Vector3(0, 1, 0), r_rest_bones[i].children_centroid_direction);
		r_rest_bones[i].rest_local_after.basis = r_rest_bones[i].rest_local_after.basis * r_rest_bones[i].rest_delta;

		//Iterate through the children and rotate them in the opposite direction.
		for (int j = 0; j < r_rest_bones[i].children.size(); j++) {
			int child_index = r_rest_bones[i].children[j];
			r_rest_bones[child_index].rest_local_after = Transform3D(r_rest_bones[i].rest_delta.inverse(), Vector3()) * r_rest_bones[child_index].rest_local_after;
		}
	}

	//One last iteration to apply the transforms we calculated
	for (int i = 0; i < bone_count; i++) {
		p_skeleton->set_bone_rest(i, r_rest_bones[i].rest_local_after);
	}
}

void PointParentToChild::_point_parent_to_child_fix_meshes(Map<int, PointParentToChild::RestBone> &r_rest_bones, Vector<Node3D *> p_meshes) {
	for (int32_t mesh_i = 0; mesh_i < p_meshes.size(); mesh_i++) {
		Node3D *mi = p_meshes.write[mesh_i];
		EditorSceneImporterMeshNode3D *importer_node_3d = Object::cast_to<EditorSceneImporterMeshNode3D>(mi);
		MeshInstance3D *mesh_3d = Object::cast_to<MeshInstance3D>(mi);

		Skeleton3D *skeleton = nullptr;
		if (importer_node_3d) {
			NodePath skeleton_path = importer_node_3d->get_skeleton_path();
			Node *node = importer_node_3d->get_node_or_null(skeleton_path);
			skeleton = Object::cast_to<Skeleton3D>(node);
		} else if (mesh_3d) {
			NodePath skeleton_path = mesh_3d->get_skeleton_path();
			Node *node = mesh_3d->get_node_or_null(skeleton_path);
			skeleton = Object::cast_to<Skeleton3D>(node);
		}
		ERR_CONTINUE(!skeleton);
		Ref<Skin> skin;
		if (importer_node_3d) {
			skin = importer_node_3d->get_skin();
		} else if (mesh_3d) {
			skin = mesh_3d->get_skin();
		}
		ERR_CONTINUE(!importer_node_3d && !mesh_3d);
		if (importer_node_3d && skin.is_null()) {
			skin = skeleton->register_skin(skin)->get_skin();
			importer_node_3d->set_skin(skin);
		} else if (mesh_3d && skin.is_null()) {
			skin = skeleton->register_skin(skin)->get_skin();
			mesh_3d->set_skin(skin);
		} else {
			skin = skin->duplicate();
			if (importer_node_3d) {
				importer_node_3d->set_skin(skin);
			} else if (mesh_3d) {
				mesh_3d->set_skin(skin);
			}
		}
		for (int32_t bind_i = 0; bind_i < skin->get_bind_count(); bind_i++) {
			String bind_name = skin->get_bind_name(bind_i);
			int32_t bone_index = -1;
			if (bind_name.is_empty()) {
				bone_index = skin->get_bind_bone(bind_i);
			} else {
				bone_index = skeleton->find_bone(bind_name);
			}
			if (bone_index == -1) {
				continue;
			}
			RestBone rest_bone = r_rest_bones[bone_index];
			Transform3D pose = skin->get_bind_pose(bind_i);
			skin->set_bind_pose(bind_i, Transform3D(rest_bone.rest_delta.inverse()) * pose);
		}
	}
}

Transform3D PointParentToChild::_point_parent_to_child_get_bone_global_transform(int p_id, Skeleton3D *p_skeleton, Vector<Vector<Transform3D>> p_local_transform_array) {
	Transform3D return_transform;
	int parent_id = p_skeleton->get_bone_parent(p_id);
	if (parent_id != -1) {
		return_transform = _point_parent_to_child_get_bone_global_transform(parent_id, p_skeleton, p_local_transform_array);
	}
	for (int i = 0; i < p_local_transform_array.size(); i++) {
		return_transform *= p_local_transform_array[i][p_id];
	}
	return return_transform;
}

void PointParentToChild::_point_parent_to_child_align_animations(AnimationPlayer *p_ap, const Map<int, RestBone> &p_rest_bones) {
	ERR_FAIL_NULL(p_ap);
	List<StringName> anim_names;
	p_ap->get_animation_list(&anim_names);
	for (List<StringName>::Element *anim_i = anim_names.front(); anim_i; anim_i = anim_i->next()) {
		Ref<Animation> a = p_ap->get_animation(anim_i->get());
		for (Map<int, RestBone>::Element *rest_bone_i = p_rest_bones.front(); rest_bone_i; rest_bone_i = rest_bone_i->next()) {
			int track = a->find_track(rest_bone_i->get().path);
			if (track == -1) {
				continue;
			}
			int new_track = a->add_track(Animation::TYPE_TRANSFORM3D);
			a->track_set_path(new_track, rest_bone_i->get().path);
			for (int key_i = 0; key_i < a->track_get_key_count(track); key_i++) {
				Vector3 loc;
				Quaternion rot;
				Vector3 scale;
				Error err = a->transform_track_get_key(track, key_i, &loc, &rot, &scale);
				ERR_CONTINUE(err);
				real_t time = a->track_get_key_time(track, key_i);
				rot.normalize();
				Vector3 axis = Vector3(0.0, 0.0, -1.0f);
				float angle = 0.0f;
				RestBone rest_bone = rest_bone_i->get();
				if (rot != Quaternion()) {
					rot.get_axis_angle(axis, angle);
				}
				axis = rest_bone.rest_delta.inverse().xform(axis);
				loc = rest_bone.rest_delta.inverse().xform(loc);
				rot = Quaternion(axis, angle);
				scale = Vector3(1, 1, 1) - rest_bone.rest_delta.inverse().xform(Vector3(1, 1, 1) - scale);
				a->transform_track_insert_key(new_track, time, loc, rot, scale);
			}
			a->remove_track(track);
		}
	}
}
