/**************************************************************************/
/*  vrm_springbone.cpp                                                    */
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

#include "vrm_springbone.h"

String VRMSpringBone::get_comment() const {
	return comment;
}

void VRMSpringBone::set_comment(const String &p_comment) {
	comment = p_comment;
}

float VRMSpringBone::get_stiffness_force() const {
	return stiffness_force;
}

void VRMSpringBone::set_stiffness_force(float p_stiffness_force) {
	stiffness_force = p_stiffness_force;
}

float VRMSpringBone::get_gravity_power() const {
	return gravity_power;
}

void VRMSpringBone::set_gravity_power(float p_gravity_power) {
	gravity_power = p_gravity_power;
}

Vector3 VRMSpringBone::get_gravity_dir() const {
	return gravity_dir;
}

float VRMSpringBone::get_drag_force() const {
	return drag_force;
}

void VRMSpringBone::set_gravity_dir(const Vector3 &p_gravity_dir) {
	gravity_dir = p_gravity_dir;
}

void VRMSpringBone::set_drag_force(float p_drag_force) {
	drag_force = p_drag_force;
}

void VRMSpringBone::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_comment"), &VRMSpringBone::get_comment);
	ClassDB::bind_method(D_METHOD("set_comment", "value"), &VRMSpringBone::set_comment);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "comment", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_comment", "get_comment");

	ClassDB::bind_method(D_METHOD("get_stiffness_force"), &VRMSpringBone::get_stiffness_force);
	ClassDB::bind_method(D_METHOD("set_stiffness_force", "value"), &VRMSpringBone::set_stiffness_force);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "stiffness_force", PROPERTY_HINT_RANGE, "0,4,0.01", PROPERTY_USAGE_DEFAULT), "set_stiffness_force", "get_stiffness_force");

	// Add other exported variables here using the same pattern
	ClassDB::bind_method(D_METHOD("get_gravity_power"), &VRMSpringBone::get_gravity_power);
	ClassDB::bind_method(D_METHOD("set_gravity_power", "value"), &VRMSpringBone::set_gravity_power);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gravity_power", PROPERTY_HINT_RANGE, "0,10,0.1", PROPERTY_USAGE_DEFAULT), "set_gravity_power", "get_gravity_power");

	ClassDB::bind_method(D_METHOD("get_gravity_dir"), &VRMSpringBone::get_gravity_dir);
	ClassDB::bind_method(D_METHOD("set_gravity_dir", "value"), &VRMSpringBone::set_gravity_dir);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gravity_dir", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_gravity_dir", "get_gravity_dir");

	ClassDB::bind_method(D_METHOD("get_drag_force"), &VRMSpringBone::get_drag_force);
	ClassDB::bind_method(D_METHOD("set_drag_force", "value"), &VRMSpringBone::set_drag_force);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drag_force", PROPERTY_HINT_RANGE, "0,4,0.01", PROPERTY_USAGE_DEFAULT), "set_drag_force", "get_drag_force");

	ClassDB::bind_method(D_METHOD("get_center_bone"), &VRMSpringBone::get_center_bone);
	ClassDB::bind_method(D_METHOD("set_center_bone", "value"), &VRMSpringBone::set_center_bone);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "center_bone", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_center_bone", "get_center_bone");

	ClassDB::bind_method(D_METHOD("get_center_node"), &VRMSpringBone::get_center_node);
	ClassDB::bind_method(D_METHOD("set_center_node", "value"), &VRMSpringBone::set_center_node);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "center_node", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_center_node", "get_center_node");

	ClassDB::bind_method(D_METHOD("get_hit_radius"), &VRMSpringBone::get_hit_radius);
	ClassDB::bind_method(D_METHOD("set_hit_radius", "value"), &VRMSpringBone::set_hit_radius);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hit_radius", PROPERTY_HINT_RANGE, "0,10,0.1", PROPERTY_USAGE_DEFAULT), "set_hit_radius", "get_hit_radius");

	ClassDB::bind_method(D_METHOD("get_root_bones"), &VRMSpringBone::get_root_bones);
	ClassDB::bind_method(D_METHOD("set_root_bones", "value"), &VRMSpringBone::set_root_bones);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "root_bones", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_root_bones", "get_root_bones");

	ClassDB::bind_method(D_METHOD("get_collider_groups"), &VRMSpringBone::get_collider_groups);
	ClassDB::bind_method(D_METHOD("set_collider_groups", "value"), &VRMSpringBone::set_collider_groups);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "collider_groups", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_collider_groups", "get_collider_groups");
}

void VRMSpringBone::setup(bool force) {
	if (!root_bones.is_empty() && skeleton != nullptr) {
		if (force || verlets.is_empty()) {
			if (!verlets.is_empty()) {
				for (int i = 0; i < verlets.size(); ++i) {
					VRMSpringBoneLogic *verlet = Object::cast_to<VRMSpringBoneLogic>(verlets[i]);
					if (verlet) {
						verlet->reset(skeleton);
					}
				}
			}
			verlets.clear();
			for (int i = 0; i < root_bones.size(); ++i) {
				String bone_name = root_bones[i];
				if (!bone_name.is_empty()) {
					setup_recursive(skeleton->find_bone(bone_name), Transform3D());
				}
			}
		}
	}
}

void VRMSpringBone::process(float delta) {
	if (verlets.is_empty()) {
		if (root_bones.is_empty()) {
			return;
		}
		setup();
	}

	float stiffness = stiffness_force * delta;
	Vector3 external = gravity_dir * (gravity_power * delta);

	for (int32_t verlet_i = 0; verlet_i < verlets.size(); verlet_i++) {
		Ref<VRMSpringBoneLogic> verlet = verlets[verlet_i];
		verlet->radius = hit_radius;
		verlet->update(skeleton, center, stiffness, drag_force, external, colliders);
	}
}

void VRMSpringBone::setup_recursive(int id, Transform3D center_tr) {
	Vector<int> bone_children = skeleton->get_bone_children(id);
	if (bone_children.is_empty()) {
		Vector3 delta = skeleton->get_bone_rest(id).origin;
		Vector3 child_position = delta.normalized() * 0.07f;
		Ref<VRMSpringBoneLogic> spring_bone_logic = memnew(VRMSpringBoneLogic(skeleton, id, center_tr.origin, child_position, skeleton->get_bone_global_pose_no_override(id)));
		verlets.append(spring_bone_logic);
	} else {
		int first_child = bone_children[0];
		Vector3 local_position = skeleton->get_bone_rest(first_child).origin;
		Vector3 sca = skeleton->get_bone_rest(first_child).basis.get_scale();
		Vector3 pos(local_position.x * sca.x, local_position.y * sca.y, local_position.z * sca.z);
		Ref<VRMSpringBoneLogic> spring_bone_logic = memnew(VRMSpringBoneLogic(skeleton, id, center_tr.origin, pos, skeleton->get_bone_global_pose_no_override(id)));
		verlets.append(spring_bone_logic);
	}
	for (int i = 0; i < bone_children.size(); ++i) {
		setup_recursive(bone_children[i], center_tr);
	}
}

void VRMSpringBone::ready(Skeleton3D *ready_skel, Array colliders_ref) {
    skeleton = ready_skel;
	setup();
	colliders = colliders_ref.duplicate(false);
}

String VRMSpringBone::get_center_bone() const {
	return center_bone;
}

void VRMSpringBone::set_center_bone(const String &p_center_bone) {
	center_bone = p_center_bone;
}

NodePath VRMSpringBone::get_center_node() const {
	return center_node;
}

void VRMSpringBone::set_center_node(const NodePath &p_center_node) {
	center_node = p_center_node;
}

float VRMSpringBone::get_hit_radius() const {
	return hit_radius;
}

void VRMSpringBone::set_hit_radius(float p_hit_radius) {
	hit_radius = p_hit_radius;
}

Array VRMSpringBone::get_root_bones() const {
	return root_bones;
}

void VRMSpringBone::set_root_bones(const Array &p_root_bones) {
	root_bones = p_root_bones;
}

Array VRMSpringBone::get_collider_groups() const {
	return collider_groups;
}

void VRMSpringBone::set_collider_groups(const Array &p_collider_groups) {
	collider_groups = p_collider_groups;
}
