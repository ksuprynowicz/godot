/* register_types.cpp */

#include "register_types.h"

#include "scene/animation/animation_blend_tree.h"
#include "animation_node_retarget.h"
#include "core/object/class_db.h"
#include "skeleton_retarget.h"
#include "skeleton_retarget_editor_plugin.h"

#ifdef TOOLS_ENABLED
#include "editor/plugins/animation_blend_tree_editor_plugin.h"
#endif

static void _editor_init() {
	EDITOR_DEF("editors/retarget_mapper/button_colors/set", Color(0.1, 0.6, 0.25));
	EDITOR_DEF("editors/retarget_mapper/button_colors/error", Color(0.8, 0.2, 0.2));
	EDITOR_DEF("editors/retarget_mapper/button_colors/unset", Color(0.3, 0.3, 0.3));
	EditorNode::get_singleton()->add_editor_plugin(memnew(RetargetProfileEditorPlugin));
	EditorNode::get_singleton()->add_editor_plugin(memnew(RetargetBoneOptionEditorPlugin));
	EditorNode::get_singleton()->add_editor_plugin(memnew(RetargetBoneMapEditorPlugin));
	AnimationNodeBlendTreeEditor::get_singleton()->add_custom_extension_type("Retarget", AnimationNodeRetarget::get_class_static());
}

void register_retargeter_types() {
	GDREGISTER_CLASS(AnimationNodeRetarget);
	GDREGISTER_CLASS(SkeletonRetarget);
	GDREGISTER_CLASS(RetargetProfile);
	GDREGISTER_CLASS(RetargetRichProfile);
	GDREGISTER_CLASS(RetargetBoneOption);
	GDREGISTER_CLASS(RetargetBoneMap);

#ifdef TOOLS_ENABLED
	// Editor-specific API.
	ClassDB::APIType prev_api = ClassDB::get_current_api();
	ClassDB::set_current_api(ClassDB::API_EDITOR);
	
	GDREGISTER_CLASS(RetargetProfileEditorPlugin);
	GDREGISTER_CLASS(RetargetBoneOptionEditorPlugin);
	GDREGISTER_CLASS(RetargetBoneMapEditorPlugin);

	ClassDB::set_current_api(prev_api);
	EditorNode::add_init_callback(_editor_init);

#endif // TOOLS_ENABLED
}

void unregister_retargeter_types() {
}
