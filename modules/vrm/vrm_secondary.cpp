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
#include "modules/vrm/vrm_toplevel.h"
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
		}
	}

	return update_in_editor;
}

void VRMSecondary::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			bool show_gizmo_spring_bone = false;
			collider_groups_internal.clear();
			spring_bones_internal.clear();
			VRMTopLevel *top_level = Object::cast_to<VRMTopLevel>(get_parent());
			if (!top_level) {
				return;
			}
			if (!get_parent()) {
				return;
			}
			update_secondary_fixed = get_parent()->get("update_secondary_fixed");
			show_gizmo_spring_bone = get_parent()->get("gizmo_spring_bone");

			Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(top_level->get_node_or_null(top_level->get_vrm_skeleton()));
			if (!skeleton) {
				return;
			}
			if (!secondary_gizmo && show_gizmo_spring_bone) {
				secondary_gizmo = memnew(SecondaryGizmo());
				secondary_gizmo->ready(this);
				skeleton->add_child(secondary_gizmo, false, InternalMode::INTERNAL_MODE_BACK);
				secondary_gizmo->set_owner(get_owner());
			}
			for (int collider_group_i = 0; collider_group_i < collider_groups.size(); ++collider_group_i) {
				Ref<VRMColliderGroup> collider_group = collider_groups[collider_group_i];
				if (collider_group.is_null()) {
					continue;
				}
				Ref<VRMColliderGroup> new_collider_group = collider_group->duplicate(false);
				new_collider_group->ready(skeleton);
				collider_groups_internal.push_back(new_collider_group);
			}
			for (int spring_bone_i = 0; spring_bone_i < spring_bones.size(); ++spring_bone_i) {
				Ref<VRMSpringBone> spring_bone = spring_bones[spring_bone_i];
				if (spring_bone.is_null()) {
					continue;
				}
				Ref<VRMSpringBone> new_spring_bone = spring_bone->duplicate(false);
				Array tmp_colliders;
				for (int collider_group_j = 0; collider_group_j < collider_groups.size(); ++collider_group_j) {
					if (new_spring_bone->collider_groups.has(collider_groups[collider_group_j])) {
						Ref<VRMColliderGroup> collider_group = cast_to<VRMColliderGroup>(collider_groups[collider_group_j]);
						if (collider_group.is_null()) {
							continue;
						}
						tmp_colliders.append_array(collider_group->colliders);
					}
				}
				new_spring_bone->ready(skeleton, tmp_colliders);
				spring_bones_internal.push_back(new_spring_bone);
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
					for (int i = 0; i < collider_groups_internal.size(); ++i) {
						Ref<VRMColliderGroup> collider_group = collider_groups_internal[i];
						if (collider_group.is_null()) {
							continue;
						}
						collider_group->process();
					}
					for (int i = 0; i < spring_bones_internal.size(); ++i) {
						Ref<VRMSpringBone> spring_bone = spring_bones_internal[i];
						if (spring_bone.is_null()) {
							continue;
						}
						spring_bone->process(delta);
					}
					if (secondary_gizmo) {
						secondary_gizmo->draw();
					}
				} else if (Engine::get_singleton()->is_editor_hint()) {
					if (secondary_gizmo) {
						secondary_gizmo->draw();
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
					for (int i = 0; i < collider_groups_internal.size(); ++i) {
						Ref<VRMColliderGroup> collider_group = collider_groups_internal[i];
						if (collider_group.is_null()) {
							continue;
						}
						collider_group->process();
					}
					for (int i = 0; i < spring_bones_internal.size(); ++i) {
						Ref<VRMSpringBone> spring_bone = spring_bones_internal[i];
						if (spring_bone.is_null()) {
							continue;
						}
						spring_bone->process(delta);
					}
					if (secondary_gizmo) {
						secondary_gizmo->draw();
					}
				} else if (Engine::get_singleton()->is_editor_hint()) {
					if (secondary_gizmo) {
						secondary_gizmo->draw();
					}
				}
			}
			break;
		}
		case NOTIFICATION_EXIT_TREE: {
			set_physics_process(false);
			set_process(false);
			secondary_gizmo->queue_free();
			secondary_gizmo = nullptr;
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
