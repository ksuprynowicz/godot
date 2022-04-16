#include "animation_node_retarget.h"
#include "editor/animation_track_editor_plugins.h"
#include "scene/animation/animation_tree.h"

AnimationNodeRetarget::AnimationNodeRetarget() {
}

void AnimationNodeRetarget::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_profile", "profile"), &AnimationNodeRetarget::set_profile);
	ClassDB::bind_method(D_METHOD("get_profile"), &AnimationNodeRetarget::get_profile);
	ClassDB::bind_method(D_METHOD("set_option", "option"), &AnimationNodeRetarget::set_option);
	ClassDB::bind_method(D_METHOD("get_option"), &AnimationNodeRetarget::get_option);
	ClassDB::bind_method(D_METHOD("set_source_map", "map"), &AnimationNodeRetarget::set_source_map);
	ClassDB::bind_method(D_METHOD("get_source_map"), &AnimationNodeRetarget::get_source_map);
	ClassDB::bind_method(D_METHOD("set_source_skeleton", "path"), &AnimationNodeRetarget::set_source_skeleton);
	ClassDB::bind_method(D_METHOD("get_source_skeleton"), &AnimationNodeRetarget::get_source_skeleton);
	ClassDB::bind_method(D_METHOD("set_target_map", "map"), &AnimationNodeRetarget::set_target_map);
	ClassDB::bind_method(D_METHOD("get_target_map"), &AnimationNodeRetarget::get_target_map);
	ClassDB::bind_method(D_METHOD("set_target_skeleton", "path"), &AnimationNodeRetarget::set_target_skeleton);
	ClassDB::bind_method(D_METHOD("get_target_skeleton"), &AnimationNodeRetarget::get_target_skeleton);
	ClassDB::bind_method(D_METHOD("set_time", "time"), &AnimationNodeRetarget::set_time);
	ClassDB::bind_method(D_METHOD("get_time"), &AnimationNodeRetarget::get_time);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time"), "set_time", "get_time");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "profile", PROPERTY_HINT_RESOURCE_TYPE, "RetargetProfile"), "set_profile", "get_profile");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "option", PROPERTY_HINT_RESOURCE_TYPE, "RetargetBoneOption"), "set_option", "get_option");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source_map", PROPERTY_HINT_RESOURCE_TYPE, "RetargetBoneMap"), "set_source_map", "get_source_map");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "source_skeleton", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton3D"), "set_source_skeleton", "get_source_skeleton");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target_map", PROPERTY_HINT_RESOURCE_TYPE, "RetargetBoneMap"), "set_target_map", "get_target_map");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_skeleton", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton3D"), "set_target_skeleton", "get_target_skeleton");
}

double AnimationNodeRetarget::process(double p_time, bool p_seek) {
	AnimationPlayer *ap = state->player;
	ERR_FAIL_COND_V(!ap, 0);

	if (!ap->has_animation(get_animation())) {
		AnimationNodeBlendTree *tree = Object::cast_to<AnimationNodeBlendTree>(parent);
		if (tree) {
			String name = tree->get_node_name(Ref<AnimationNodeAnimation>(this));
			make_invalid(vformat(RTR("On BlendTree node '%s', animation not found: '%s'"), name, get_animation()));

		} else {
			make_invalid(vformat(RTR("Animation not found: '%s'"), get_animation()));
		}

		return 0;
	}

	Ref<Animation> anim = ap->get_animation(get_animation());
	double anim_size = (double)anim->get_length();
	double step = 0.0;
	double prev_time = time;
	int pingponged = 0;
	bool current_backward = signbit(p_time);

	if (p_seek) {
		step = p_time - time;
		time = p_time;
	} else {
		p_time *= is_backward() ? -1.0 : 1.0;
		if (!(time == anim_size && !current_backward) && !(time == 0 && current_backward)) {
			time = time + p_time;
			step = p_time;
		}
	}

	if (anim->get_loop_mode() == Animation::LoopMode::LOOP_PINGPONG) {
		if (anim_size) {
			if ((int)Math::floor(abs(time - prev_time) / anim_size) % 2 == 0) {
				if (prev_time > 0 && time <= 0) {
					set_backward(!is_backward());
					pingponged = -1;
				}
				if (prev_time < anim_size && time >= anim_size) {
					set_backward(!is_backward());
					pingponged = 1;
				}
			}
			time = Math::pingpong(time, anim_size);
		}
	} else {
		if (anim->get_loop_mode() == Animation::LoopMode::LOOP_LINEAR) {
			if (anim_size) {
				time = Math::fposmod(time, anim_size);
			}
		} else if (time < 0) {
			time = 0;
		} else if (time > anim_size) {
			time = anim_size;
		}
		set_backward(false);
	}
	// TODO implement blend_retargeted_animation
	if (get_play_mode() == PLAY_MODE_FORWARD) {
		blend_animation(get_animation(), time, step, p_seek, 1.0, pingponged);
	} else {
		blend_animation(get_animation(), anim_size - time, -step, p_seek, 1.0, pingponged);
	}
	Skeleton3D *source_skeleton = nullptr;
	Skeleton3D *target_skeleton = nullptr;
	AnimationTree *tree = state->tree;
	if (tree) {
		source_skeleton = tree->cast_to<Skeleton3D>(tree->get_node_or_null("../" + source_skeleton_path));
		target_skeleton = tree->cast_to<Skeleton3D>(tree->get_node_or_null("../" + target_skeleton_path));
	}
	if (profile.is_null()) {
		return time;
	}
	if (option.is_null()) {
		return time;
	}
	if (source_map.is_null()) {
		return time;
	}
	if (target_map.is_null()) {
		return time;
	}
	if (!source_skeleton) {
		return time;
	}
	if (!target_skeleton) {
		return time;
	}
	RetargetBoneOption::RetargetMode retarget_mode = RetargetBoneOption::RETARGET_MODE_GLOBAL;
	int32_t size = profile->get_intermediate_bones_size();
	for (int32_t bone_i = 0; bone_i < size; bone_i++) {
		StringName immediate_bone = profile->get_intermediate_bone_name(bone_i);
		if (!source_map->has_key(immediate_bone)) {
			continue;
		}
		StringName source_bone_name = source_map->get_bone_name(immediate_bone);
		BoneId source_bone = source_skeleton->find_bone(String(source_bone_name));
		if (!target_map->has_key(immediate_bone)) {
			continue;
		}
		StringName target_bone_name = target_map->get_bone_name(immediate_bone);
		BoneId target_bone = target_skeleton->find_bone(String(target_bone_name));
		if (source_bone == -1) {
			continue;
		}
		if (target_bone == -1) {
			continue;
		}
		if (retarget_mode == RetargetBoneOption::RETARGET_MODE_GLOBAL) {
			Vector3 loc = source_skeleton->extract_global_retarget_position(source_bone);
			loc = target_skeleton->global_retarget_position_to_local_pose(target_bone, loc);
			Quaternion rot = source_skeleton->extract_global_retarget_rotation(source_bone);
			rot = target_skeleton->global_retarget_rotation_to_local_pose(target_bone, rot);
			Vector3 scale = source_skeleton->extract_global_retarget_scale(source_bone);
			scale = target_skeleton->global_retarget_scale_to_local_pose(target_bone, scale);
			target_skeleton->set_bone_pose_position(target_bone, loc);
			target_skeleton->set_bone_pose_rotation(target_bone, rot);
			target_skeleton->set_bone_pose_scale(target_bone, scale);
		} else if (retarget_mode == RetargetBoneOption::RETARGET_MODE_LOCAL) {
			Vector3 loc = source_skeleton->extract_local_retarget_position(source_bone);
			loc = target_skeleton->local_retarget_position_to_local_pose(target_bone, loc);
			Quaternion rot = source_skeleton->extract_local_retarget_rotation(source_bone);
			rot = target_skeleton->local_retarget_rotation_to_local_pose(target_bone, rot);
			Vector3 scale = source_skeleton->extract_local_retarget_scale(source_bone);
			scale = target_skeleton->local_retarget_scale_to_local_pose(target_bone, scale);
			target_skeleton->set_bone_pose_position(target_bone, loc);
			target_skeleton->set_bone_pose_rotation(target_bone, rot);
			target_skeleton->set_bone_pose_scale(target_bone, scale);
		} else {
			Vector3 loc = source_skeleton->get_bone_pose_position(source_bone);
			Quaternion rot = source_skeleton->get_bone_pose_rotation(source_bone);
			Vector3 scale = source_skeleton->get_bone_pose_scale(source_bone);
			target_skeleton->set_bone_pose_position(target_bone, loc);
			target_skeleton->set_bone_pose_rotation(target_bone, rot);
			target_skeleton->set_bone_pose_scale(target_bone, scale);
		}
	}
	// Blend through the tree.
	return time;
}

String AnimationNodeRetarget::get_caption() const {
	return "Retargeting";
}

Variant AnimationNodeRetarget::get_parameter_default_value(const StringName &p_parameter) const {
	return 0;
}

void AnimationNodeRetarget::get_parameter_list(List<PropertyInfo> *r_list) const {
	r_list->push_back(PropertyInfo(Variant::FLOAT, "time", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE));
}