/*************************************************************************/
/*  editor_importer_bake_reset.cpp                                       */
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

#include "editor/import/editor_importer_bake_reset.h"

#include "core/error/error_list.h"
#include "core/error/error_macros.h"
#include "core/math/transform_3d.h"
#include "core/string/ustring.h"
#include "resource_importer_scene.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_player.h"

void BakeReset::_bake_animation_pose(Node *scene, const String &p_bake_anim) {
	Map<StringName, BakeResetRestBone> r_rest_bones;
	Vector<Node3D *> r_meshes;
	List<Node *> queue;
	queue.push_back(scene);
	while (!queue.is_empty()) {
		List<Node *>::Element *E = queue.front();
		Node *node = E->get();
		ImporterMeshInstance3D *editor_mesh_3d = scene->cast_to<ImporterMeshInstance3D>(node);
		MeshInstance3D *mesh_3d = scene->cast_to<MeshInstance3D>(node);
		if (scene->cast_to<Skeleton3D>(node)) {
			Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(node);
			_transform_bone(skeleton, "hips", Basis(), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "spine", Basis(), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "chest", Basis(), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "spine", Basis(), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "upperChest", Basis(), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "neck", Basis(), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "head", Basis(), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "spine", Basis(), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftUpperLeg", Basis(Vector3(-1.0, 0.0, 0.0), Vector3(0.0, -1.0, 0.0), Vector3(0.0, 0.0, 1.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftLowerLeg", Basis(Vector3(1.0, 0.0, 0.0), Vector3(0, -1.0, 0.0), Vector3(0.0, 0.0, -1.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftFoot", Basis(Vector3(1.0, 0.0, 0.0), Vector3(0, -1.0, 0.0), Vector3(0.0, 0.0, -1.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftLowerLeg", Basis(Vector3(-1.0, 0.0, 0.0), Vector3(0.0, 0.0, 1.0), Vector3(0, 1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftToes", Basis(Vector3(1.0, 0.0, 0.0), Vector3(0.0, 0.0, 1.0), Vector3(0, -1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightUpperLeg", Basis(Vector3(-1.0, 0.0, 0.0), Vector3(0, -1.0, 0.0), Vector3(0.0, 0.0, 1.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightLowerLeg", Basis(Vector3(1.0, 0.0, 0.0), Vector3(0, -1.0, 0.0), Vector3(0.0, 0.0, -1.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightFoot", Basis(Vector3(-1.0, 0.0, 0.0), Vector3(0.0, 0.0, 1.0), Vector3(0, 1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightToes", Basis(Vector3(1.0, 0.0, 0.0), Vector3(0.0, 0.0, 1.0), Vector3(0, -1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftShoulder", Basis(Vector3(0.0, 0.0, 1.0), Vector3(1.0, 0.0, 0.0), Vector3(0, 1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftUpperArm", Basis(Vector3(0.0, 0.0, -1.0), Vector3(1.0, 0.0, 0.0), Vector3(0, -1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftLowerArm", Basis(Vector3(0.0, -1.0, 0.0), Vector3(1.0, 0.0, 0.0), Vector3(0.0, 0.0, 1.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftHand", Basis(Vector3(0.0, 0.0, -1.0), Vector3(1.0, 0.0, 0.0), Vector3(0, -1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightShoulder", Basis(Vector3(0.0, 0.0, -1.0), Vector3(-1.0, 0.0, 0.0), Vector3(0, 1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightUpperArm", Basis(Vector3(0.0, 0.0, 1.0), Vector3(-1.0, 0.0, 0.0), Vector3(0, -1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightLowerArm", Basis(Vector3(0.0, 1.0, 0.0), Vector3(-1.0, 0.0, 0.0), Vector3(0.0, 0.0, 1.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightHand", Basis(Vector3(0.0, 0.0, 1.0), Vector3(-1.0, 0.0, 0.0), Vector3(0, -1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftEye", Basis(Vector3(1.0, 0.0, 0.0), Vector3(0.0, 0.0, 1.0), Vector3(0, -1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightEye", Basis(Vector3(1.0, 0.0, 0.0), Vector3(0.0, 0.0, 1.0), Vector3(0, -1.0, 0.0)), Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "jaw", Basis(Vector3(-1.0, 0.0, 0.0), Vector3(0, -0.383, 0.924), Vector3(0, 0.924, 0.383)), Basis(), Vector3(), r_rest_bones);
			Basis rightThumbProximal = Basis(Vector3(0.707, -0.5, 0.5), Vector3(-0.707, -0.5, 0.5), Vector3(0, -0.707, -0.707));
			_transform_bone(skeleton, "rightThumbProximal", rightThumbProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightThumbIntermediate", rightThumbProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightThumbDistal", rightThumbProximal, Basis(), Vector3(), r_rest_bones);
			Basis rightIndexProximal = Basis(Vector3(0.0, 0.0, 1.0), Vector3(-1.0, 0.0, 0.0), Vector3(0.0, -1.0, 0.0));
			_transform_bone(skeleton, "rightIndexProximal", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightIndexIntermediate", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightIndexDistal", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightMiddleProximal", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightMiddleIntermediate", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightMiddleDistal", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightRingProximal", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightRingIntermediate", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightRingDistal", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightLittleProximal", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightLittleIntermediate", rightIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "rightLittleDistal", rightIndexProximal, Basis(), Vector3(), r_rest_bones);

			Basis leftThumbProximal = Basis(Vector3(0.707, 0.5, -0.5), Vector3(0.707, -0.5, 0.5), Vector3(0, -0.707, -0.707));
			_transform_bone(skeleton, "leftThumbProximal", leftThumbProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftThumbIntermediate", leftThumbProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftThumbDistal", leftThumbProximal, Basis(), Vector3(), r_rest_bones);
			Basis leftIndexProximal = Basis(Vector3(0.0, 0.0, -1.0), Vector3(1.0, 0.0, 0.0), Vector3(0.0, -1.0, 0.0));
			_transform_bone(skeleton, "leftIndexProximal", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftIndexIntermediate", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftIndexDistal", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftMiddleProximal", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftMiddleIntermediate", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftMiddleDistal", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftRingProximal", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftRingIntermediate", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftRingDistal", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftLittleProximal", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftLittleIntermediate", leftIndexProximal, Basis(), Vector3(), r_rest_bones);
			_transform_bone(skeleton, "leftLittleDistal", leftIndexProximal, Basis(), Vector3(), r_rest_bones);

			// Step 2: Bake the RESET animation from the RestBone to the skeleton.
			_fix_skeleton(skeleton, r_rest_bones);
		}
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

	queue.push_back(scene);
	while (!queue.is_empty()) {
		List<Node *>::Element *E = queue.front();
		Node *node = E->get();
		AnimationPlayer *ap = Object::cast_to<AnimationPlayer>(node);
		if (ap) {
			// Step 3: Key all RESET animation frames to identity.
			_align_animations(ap, r_rest_bones);
		}

		int child_count = node->get_child_count();
		for (int i = 0; i < child_count; i++) {
			queue.push_back(node->get_child(i));
		}
		queue.pop_front();
	}
}

void BakeReset::_align_animations(AnimationPlayer *p_ap, const Map<StringName, BakeResetRestBone> &r_rest_bones) {
	ERR_FAIL_NULL(p_ap);
	List<StringName> anim_names;
	p_ap->get_animation_list(&anim_names);
	for (List<StringName>::Element *anim_i = anim_names.front(); anim_i; anim_i = anim_i->next()) {
		Ref<Animation> a = p_ap->get_animation(anim_i->get());
		ERR_CONTINUE(a.is_null());
		for (Map<StringName, BakeResetRestBone>::Element *rest_bone_i = r_rest_bones.front(); rest_bone_i; rest_bone_i = rest_bone_i->next()) {
			int track = a->find_track(NodePath(rest_bone_i->key()), Animation::TYPE_POSITION_3D);
			if (track != -1) {
				int new_track = a->add_track(Animation::TYPE_POSITION_3D);
				NodePath new_path = NodePath(rest_bone_i->key());
				BakeResetRestBone rest_bone = rest_bone_i->get();
				a->track_set_path(new_track, new_path);
				for (int key_i = 0; key_i < a->track_get_key_count(track); key_i++) {
					Vector3 loc;
					Error err = a->position_track_get_key(track, key_i, &loc);
					ERR_CONTINUE(err);
					real_t time = a->track_get_key_time(track, key_i);
					loc = loc + rest_bone.loc;
					// Apply the reverse of the rest changes to make the key be close to identity transform.
					a->position_track_insert_key(new_track, time, loc);
				}
				a->remove_track(track);
			}
			track = a->find_track(NodePath(rest_bone_i->key()), Animation::TYPE_ROTATION_3D);
			if (track != -1) {
				int new_track = a->add_track(Animation::TYPE_ROTATION_3D);
				NodePath new_path = NodePath(rest_bone_i->key());
				BakeResetRestBone rest_bone = rest_bone_i->get();
				a->track_set_path(new_track, new_path);
				for (int key_i = 0; key_i < a->track_get_key_count(track); key_i++) {
					Quaternion rot;
					Error err = a->rotation_track_get_key(track, key_i, &rot);
					ERR_CONTINUE(err);
					real_t time = a->track_get_key_time(track, key_i);
					rot.normalize();
					rot = rot * rest_bone.rest_delta.get_rotation_quaternion();
					rot.normalize();
					// Apply the reverse of the rest changes to make the key be close to identity transform.
					a->rotation_track_insert_key(new_track, time, rot);
				}
				a->remove_track(track);
			}
			track = a->find_track(NodePath(rest_bone_i->key()), Animation::TYPE_SCALE_3D);
			if (track != -1) {
				int new_track = a->add_track(Animation::TYPE_SCALE_3D);
				NodePath new_path = NodePath(rest_bone_i->key());
				BakeResetRestBone rest_bone = rest_bone_i->get();
				a->track_set_path(new_track, new_path);
				for (int key_i = 0; key_i < a->track_get_key_count(track); key_i++) {
					Vector3 scale;
					Error err = a->scale_track_get_key(track, key_i, &scale);
					ERR_CONTINUE(err);
					real_t time = a->track_get_key_time(track, key_i);
					scale = Vector3(1, 1, 1) + (rest_bone.rest_delta.get_scale() - scale);
					// Apply the reverse of the rest changes to make the key be close to identity transform.
					a->scale_track_insert_key(new_track, time, scale);
				}
				a->remove_track(track);
			}
		}
	}
}

void BakeReset::_fix_skeleton(Skeleton3D *p_skeleton, Map<StringName, BakeReset::BakeResetRestBone> &r_rest_bones) {
	int bone_count = p_skeleton->get_bone_count();

	// First iterate through all the bones and update the RestBone.
	for (int j = 0; j < bone_count; j++) {
		StringName final_path = String(p_skeleton->get_owner()->get_path_to(p_skeleton)) + String(":") + p_skeleton->get_bone_name(j);
	}

	for (int i = 0; i < bone_count; i++) {
		int parent_bone = p_skeleton->get_bone_parent(i);
		String path = p_skeleton->get_owner()->get_path_to(p_skeleton);
		StringName final_path = String(path) + String(":") + p_skeleton->get_bone_name(parent_bone);
		if (parent_bone >= 0) {
			r_rest_bones[final_path].children.push_back(i);
		}
	}

	//When we rotate a bone, we also have to move all of its children in the opposite direction
	for (int i = 0; i < bone_count; i++) {
		StringName final_path = String(p_skeleton->get_owner()->get_path_to(p_skeleton)) + String(":") + p_skeleton->get_bone_name(i);
		r_rest_bones[final_path].rest_local = r_rest_bones[final_path].rest_local * Transform3D(r_rest_bones[final_path].rest_delta, r_rest_bones[final_path].loc);

		//Iterate through the children and rotate them in the opposite direction.
		for (int j = 0; j < r_rest_bones[final_path].children.size(); j++) {
			int child_index = r_rest_bones[final_path].children[j];
			StringName child_path = String(p_skeleton->get_owner()->get_path_to(p_skeleton)) + String(":") + p_skeleton->get_bone_name(child_index);
			r_rest_bones[child_path].rest_local = Transform3D(r_rest_bones[final_path].rest_delta, r_rest_bones[final_path].loc).affine_inverse() * r_rest_bones[child_path].rest_local;
		}
	}

	for (int i = 0; i < bone_count; i++) {
		StringName final_path = String(p_skeleton->get_owner()->get_path_to(p_skeleton)) + String(":") + p_skeleton->get_bone_name(i);
		ERR_CONTINUE(!r_rest_bones.has(final_path));
		Transform3D rest_transform = r_rest_bones[final_path].rest_local;
		p_skeleton->set_bone_rest(i, p_skeleton->get_bone_rest(i) * rest_transform);
	}
}

void BakeReset::_transform_bone(Skeleton3D *p_skeleton, String bone_name, Basis vrm_rest_rot, Basis rot, Vector3 loc, Map<StringName, BakeReset::BakeResetRestBone> &r_rest_bones) {
	BakeResetRestBone rest_bone;
	String new_path = String(p_skeleton->get_owner()->get_path_to(p_skeleton)) + ":" + bone_name;
	rest_bone.rest_delta = vrm_rest_rot;
	rest_bone.loc = loc;
	r_rest_bones[new_path] = rest_bone;
}
