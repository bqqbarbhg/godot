/**************************************************************************/
/*  vrm_secondary.cpp                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "vrm_secondary.h"
#include "core/config/engine.h"
#include "vrm_constants.h"
#include "vrm_secondary_gizmo.h"
#include "vrm_springbone.h"

void VRMSecondary::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_spring_bones", "new_spring_bones"), &VRMSecondary::set_spring_bones);
	ClassDB::bind_method(D_METHOD("set_collider_groups", "new_collider_groups"), &VRMSecondary::set_collider_groups);
	ClassDB::bind_method(D_METHOD("get_spring_bones"), &VRMSecondary::get_spring_bones);
	ClassDB::bind_method(D_METHOD("get_collider_groups"), &VRMSecondary::get_collider_groups);

	ClassDB::bind_method(D_METHOD("check_for_editor_update"), &VRMSecondary::check_for_editor_update);

	ClassDB::bind_method(D_METHOD("get_collider_groups_internal"), &VRMSecondary::get_collider_groups_internal);
	ClassDB::bind_method(D_METHOD("set_collider_groups_internal", "collider_groups_internal"), &VRMSecondary::set_collider_groups_internal);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "collider_groups_internal"), "set_collider_groups_internal", "get_collider_groups_internal");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "spring_bones"), "set_spring_bones", "get_spring_bones");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "collider_groups"), "set_collider_groups", "get_collider_groups");
}

void VRMSecondary::set_spring_bones(Array new_spring_bones) {
	spring_bones = new_spring_bones;
}

void VRMSecondary::set_collider_groups(Array new_collider_groups) {
	collider_groups = new_collider_groups;
}

Array VRMSecondary::get_spring_bones() const {
	return spring_bones;
}

Array VRMSecondary::get_collider_groups() const {
	return collider_groups;
}

bool VRMSecondary::check_for_editor_update() {
	Node *parent = get_parent();
	VRMTopLevel *vrm_top_level = Object::cast_to<VRMTopLevel>(parent);

	if (vrm_top_level) {
		update_in_editor = vrm_top_level->get_update_in_editor();
		if (update_in_editor) {
			notification(NOTIFICATION_READY);
		} else {
			for (int i = 0; i < spring_bones_internal.size(); ++i) {
				Ref<VRMSpringBone> spring_bone = cast_to<VRMSpringBone>(spring_bones_internal[i]);
				if (spring_bone.is_null()) {
					continue;
				}
				spring_bone->skel->clear_bones_global_pose_override();
			}
		}
	}

	return update_in_editor;
}

void VRMSecondary::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			bool gizmo_spring_bone = false;
			if (Object::cast_to<VRMTopLevel>(get_parent())) {
				update_secondary_fixed = get_parent()->get("update_secondary_fixed");
				gizmo_spring_bone = get_parent()->get("gizmo_spring_bone");
			}

			if (!secondary_gizmo && (Engine::get_singleton()->is_editor_hint() || gizmo_spring_bone)) {
				secondary_gizmo = memnew(SecondaryGizmo(this));
				add_child(secondary_gizmo, true);
			}
			collider_groups_internal.clear();
			spring_bones_internal.clear();
			for (int i = 0; i < collider_groups.size(); ++i) {
				Ref<VRMColliderGroup> collider_group = collider_groups[i];
				Ref<VRMColliderGroup> new_collider_group = collider_group->duplicate(false);
				Node3D *parent = Object::cast_to<Node3D>(get_node_or_null(new_collider_group->skeleton_or_node));
				if (parent) {
					new_collider_group->_ready(parent, parent);
					collider_groups_internal.push_back(new_collider_group);
				}
			}
			for (int i = 0; i < spring_bones.size(); ++i) {
				Ref<VRMSpringBone> spring_bone = spring_bones[i];
				Ref<VRMSpringBone> new_spring_bone = spring_bone->duplicate(false);
				Array tmp_colliders;
				for (int i = 0; i < collider_groups.size(); ++i) {
					if (new_spring_bone->collider_groups.has(collider_groups[i])) {
						Ref<VRMColliderGroup> collider_group = cast_to<VRMColliderGroup>(collider_groups[i]);
						if (collider_group.is_null()) {
							continue;
						}
						tmp_colliders.append_array(collider_group->colliders);
					}
				}
				Skeleton3D *skel = Object::cast_to<Skeleton3D>(get_node_or_null(new_spring_bone->skeleton));
				if (skel) {
					new_spring_bone->_ready(skel, tmp_colliders);
					spring_bones_internal.push_back(new_spring_bone);
				}
			}
			if (update_secondary_fixed) {
				set_physics_process(true);
				set_process(false);
			} else {
				set_physics_process(false);
				set_process(true);
			}
			break;
		}
		case NOTIFICATION_PROCESS: {
			if (!check_for_editor_update()) {
				return;
			}
			double delta = get_process_delta_time();
			if (!update_secondary_fixed) {
				if (!Engine::get_singleton()->is_editor_hint() || check_for_editor_update()) {
					for (int i = 0; i < spring_bones_internal.size(); ++i) {
						Ref<VRMSpringBone> spring_bone = spring_bones_internal[i];
						if (spring_bone->skel) {
							spring_bone->skel->force_update_all_bone_transforms();
						}
					}
					for (int i = 0; i < collider_groups_internal.size(); ++i) {
						Ref<VRMColliderGroup> collider_group = collider_groups_internal[i];
						collider_group->_process();
					}
					for (int i = 0; i < spring_bones_internal.size(); ++i) {
						Ref<VRMSpringBone> spring_bone = spring_bones_internal[i];
						spring_bone->_process(delta);
					}
					if (secondary_gizmo) {
						if (Engine::get_singleton()->is_editor_hint()) {
							secondary_gizmo->draw_in_editor();
						} else {
							secondary_gizmo->draw_in_game();
						}
					}
				} else if (Engine::get_singleton()->is_editor_hint()) {
					if (secondary_gizmo) {
						secondary_gizmo->draw_in_editor();
					}
				}
			}
			break;
		}
		case NOTIFICATION_PHYSICS_PROCESS: {
			if (!check_for_editor_update()) {
				return;
			}
			double delta = get_physics_process_delta_time();
			if (update_secondary_fixed) {
				if (!Engine::get_singleton()->is_editor_hint() || check_for_editor_update()) {
					for (int i = 0; i < spring_bones_internal.size(); ++i) {
						Ref<VRMSpringBone> spring_bone = spring_bones_internal[i];
						if (spring_bone->skel) {
							spring_bone->skel->force_update_all_bone_transforms();
						}
					}
					for (int i = 0; i < collider_groups_internal.size(); ++i) {
						Ref<VRMColliderGroup> collider_group = collider_groups_internal[i];
						if (collider_group.is_null()) {
							continue;
						}
						collider_group->_process();
					}
					for (int i = 0; i < spring_bones_internal.size(); ++i) {
						Ref<VRMSpringBone> spring_bone = spring_bones_internal[i];
						if (spring_bone.is_null()) {
							continue;
						}
						spring_bone->_process(delta);
					}
					if (secondary_gizmo) {
						if (Engine::get_singleton()->is_editor_hint()) {
							secondary_gizmo->draw_in_editor();
						} else {
							secondary_gizmo->draw_in_game();
						}
					}
				} else if (Engine::get_singleton()->is_editor_hint()) {
					if (secondary_gizmo) {
						secondary_gizmo->draw_in_editor();
					}
				}
			}
			break;
		}
	}
}

Array VRMSecondary::get_collider_groups_internal() const {
	return collider_groups_internal;
}

void VRMSecondary::set_collider_groups_internal(Array p_collider_groups_internal) {
	collider_groups_internal = p_collider_groups_internal;
}
