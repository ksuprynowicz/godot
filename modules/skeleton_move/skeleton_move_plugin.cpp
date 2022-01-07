/*************************************************************************/
/*  skeleton_move_plugin.cpp                                             */
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

#if TOOLS_ENABLED
#include "skeleton_move_plugin.h"
#include "core/config/project_settings.h"
#include "core/error/error_list.h"
#include "core/object/object.h"
#include "core/templates/vector.h"
#include "editor/editor_file_system.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/gui/check_box.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"

#include "editor/editor_node.h"

String AnimationMoveRootPlugin::get_name() const {
	return "MoveAnimationPlayerToRoot";
}

bool AnimationMoveRootPlugin::has_main_screen() const {
	return false;
}

AnimationMoveRootPlugin::AnimationMoveRootPlugin(EditorNode *p_node) {
	editor = p_node;
	add_tool_menu_item("Move animations to root...", callable_mp(this, &AnimationMoveRootPlugin::animation_players_move_to_root));
}

Error AnimationMoveRootPlugin::animation_players_move_to_root() {
	Node *root = editor->get_tree()->get_edited_scene_root();
	if (!root) {
		editor->show_accept(TTR("This operation can't be done without a root."), TTR("OK"));
		return FAILED;
	}
	unique_names.clear();
	AnimationPlayer *new_ap = memnew(AnimationPlayer);
	List<Node *> queue;
	queue.push_back(root);
	while (!queue.is_empty()) {
		List<Node *>::Element *E = queue.front();
		Node *node = E->get();
		AnimationPlayer *ap = Object::cast_to<AnimationPlayer>(node);
		if (ap) {
			List<StringName> animations;
			ap->get_animation_list(&animations);
			for (List<StringName>::Element *A = animations.front(); A; A = A->next()) {
				Ref<Animation> animation = ap->get_animation(A->get())->duplicate();
				for (int32_t k = 0; k < animation->get_track_count(); k++) {
					const NodePath path = animation->track_get_path(k);
					if (!ap->get_parent()) {
						continue;
					}
					Node *current_node = ap->get_parent()->get_node_or_null(String(path).get_slicec(':', 0));
					ERR_CONTINUE(!current_node);
					if (current_node->get_class_name() == Node3D().get_class_name()) {
						return FAILED;
					}
					String property;
					String split_path = String(path).get_slicec(':', 0);
					if (String(path).get_slice_count(":") > 1) {
						property = String(path).trim_prefix(split_path + ":");
					}
					String name = current_node->get_name();
					MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(current_node);
					String track_path;
					Skeleton3D *skeleton = nullptr;
					if (mi) {
						String skeleton_path = mi->get_skeleton_path();
						if (!skeleton_path.is_empty()) {
							Node *skeleton_node = mi->get_node_or_null(skeleton_path);
							ERR_CONTINUE(!skeleton_node);
							skeleton = Object::cast_to<Skeleton3D>(skeleton_node);
							ERR_CONTINUE(!skeleton);
						}
					}
					if (mi && skeleton && property.find("blend_shapes/") != -1) {
						track_path = String(root->get_path_to(skeleton)) + "/" + String(name) + ":" + property;
					} else if (mi && !skeleton && property.find("blend_shapes/") != -1) {
						track_path = String(root->get_path_to(mi)) + ":" + property;
					} else if (current_node) {
						if (!property.is_empty()) {
							track_path = String(root->get_path_to(current_node)) + ":" + property;
						} else {
							track_path = String(root->get_path_to(current_node));
						}
					} else {
						continue;
					}
					animation->track_set_path(k, track_path);
				}
				String new_animation_name = A->get();
				new_ap->add_animation(_gen_unique_name(new_animation_name), animation);
			}
			if (ap->is_editable_instance(ap)) {
				Node *new_node = memnew(Node);
				ap->replace_by(new_node);
			}
		}
		int child_count = node->get_child_count();
		for (int i = 0; i < child_count; i++) {
			queue.push_back(node->get_child(i));
		}
		queue.pop_front();
	}
	root->add_child(new_ap, true);
	new_ap->set_owner(root);

	return OK;
}

String AnimationMoveRootPlugin::_gen_unique_name(const String &p_name) {
	const String s_name = p_name.validate_node_name();

	String name;
	int index = 1;
	while (true) {
		name = s_name;

		if (index > 1) {
			name += itos(index);
		}
		if (!unique_names.has(name)) {
			break;
		}
		index++;
	}

	unique_names.insert(name);

	return name;
}

#endif // TOOLS_ENABLED
