/*************************************************************************/
/*  engine.h                                                             */
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

#ifndef ANIMATION_NODE_RETARGETER_H
#define ANIMATION_NODE_RETARGETER_H

#include "scene/animation/animation_blend_tree.h"
#include "skeleton_retarget.h"

class AnimationNodeRetarget : public AnimationNodeAnimation {
	GDCLASS(AnimationNodeRetarget, AnimationNodeAnimation);
	friend class AnimationTree;
	Ref<RetargetProfile> profile;
	Ref<RetargetBoneOption> option;
	Ref<RetargetBoneMap> source_map;
	NodePath source_skeleton_path;
	Ref<RetargetBoneMap> target_map;
	NodePath target_skeleton_path;
	double time = 0.0f;

protected:
	static void _bind_methods();

public:
	void set_time(double p_time) {
		time = p_time;
	}
	double get_time() {
		return time;
	}
	void set_profile(Ref<RetargetProfile> p_profile) {
		profile = p_profile;
	}
	Ref<RetargetProfile> get_profile() {
		return profile;
	}
	void set_option(Ref<RetargetBoneOption> p_option) {
		option = p_option;
	}
	Ref<RetargetBoneOption> get_option() {
		return option;
	}
	void set_source_map(Ref<RetargetBoneMap> p_map) {
		source_map = p_map;
	}
	Ref<RetargetBoneMap> get_source_map() {
		return source_map;
	}
	void set_source_skeleton(NodePath p_skeleton) {
		source_skeleton_path = p_skeleton;
	}
	NodePath get_source_skeleton() {
		return source_skeleton_path;
	}
	void set_target_map(Ref<RetargetBoneMap> p_map) {
		target_map = p_map;
	}
	Ref<RetargetBoneMap> get_target_map() {
		return target_map;
	}
	void set_target_skeleton(NodePath p_skeleton) {
		target_skeleton_path = p_skeleton;
	}
	NodePath get_target_skeleton() {
		return target_skeleton_path;
	}

	virtual void get_parameter_list(List<PropertyInfo> *r_list) const override;
	virtual Variant get_parameter_default_value(const StringName &p_parameter) const override;
	virtual String get_caption() const override;
	virtual double process(double p_time, bool p_seek) override;
	AnimationNodeRetarget();
};

#endif