/**************************************************************************/
/*  vrm_spring_bone_logic.cpp                                             */
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

#include "vrm_spring_bone_logic.h"
#include "core/object/class_db.h"

void VRMSpringBoneLogic::_bind_methods() {
	ClassDB::bind_method(D_METHOD("ready", "skeleton", "bone_idx", "center", "local_child_position", "default_pose"), &VRMSpringBoneLogic::ready);
	ClassDB::bind_method(D_METHOD("get_transform", "skeleton"), &VRMSpringBoneLogic::get_transform);
	ClassDB::bind_method(D_METHOD("get_rotation_relative_to_origin", "skeleton"), &VRMSpringBoneLogic::get_rotation_relative_to_origin);
	ClassDB::bind_method(D_METHOD("get_global_pose", "skeleton"), &VRMSpringBoneLogic::get_global_pose);
	ClassDB::bind_method(D_METHOD("get_local_pose_rotation", "skeleton"), &VRMSpringBoneLogic::get_local_pose_rotation);
	ClassDB::bind_method(D_METHOD("reset", "skeleton"), &VRMSpringBoneLogic::reset);
	ClassDB::bind_method(D_METHOD("update", "skeleton", "center", "stiffness_force", "drag_force", "external", "colliders"), &VRMSpringBoneLogic::update);
	ClassDB::bind_method(D_METHOD("collision", "skeleton", "colliders", "next_tail"), &VRMSpringBoneLogic::collision);
}

Transform3D VRMSpringBoneLogic::get_transform(Skeleton3D *skel) {
	return skel->get_global_transform() * skel->get_bone_global_pose_no_override(bone_idx);
}

Quaternion VRMSpringBoneLogic::get_rotation_relative_to_origin(Skeleton3D *skel) {
	return get_transform(skel).basis.get_rotation_quaternion();
}

Transform3D VRMSpringBoneLogic::get_global_pose(Skeleton3D *skel) {
	return skel->get_bone_global_pose_no_override(bone_idx);
}

Quaternion VRMSpringBoneLogic::get_local_pose_rotation(Skeleton3D *skel) {
	return get_global_pose(skel).basis.get_rotation_quaternion();
}

void VRMSpringBoneLogic::reset(Skeleton3D *skel) {
	skel->set_bone_global_pose_override(bone_idx, initial_transform, 1.0, true);
}

void VRMSpringBoneLogic::ready(Skeleton3D *skel, int idx, Vector3 center, Vector3 local_child_position, Transform3D default_pose) {
	initial_transform = default_pose;
	bone_idx = idx;
	Vector3 world_child_position = get_transform(skel).xform(local_child_position);
	if (center != Vector3()) {
		current_tail = Transform3D(Basis(), center).xform_inv(world_child_position);
	} else {
		current_tail = world_child_position;
	}
	prev_tail = current_tail;
	bone_axis = local_child_position.normalized();
	length = local_child_position.length();
}

void VRMSpringBoneLogic::update(Skeleton3D *skel, Vector3 center, float stiffness_force, float drag_force, Vector3 external, Array colliders) {
	Vector3 tmp_current_tail = Transform3D(Basis(), center).xform(current_tail);
	Vector3 tmp_prev_tail = Transform3D(Basis(), center).xform(prev_tail);

	// Integrate the velocity verlet.
	Vector3 next_tail = tmp_current_tail + (tmp_current_tail - tmp_prev_tail) * (1.0 - drag_force) + (get_rotation_relative_to_origin(skel).xform(bone_axis)) * stiffness_force + external;

	// Limit the bone length.
	Vector3 origin = get_transform(skel).origin;
	next_tail = origin + (next_tail - origin).normalized() * length;

	// Collide movement.
	next_tail = collision(skel, colliders, next_tail);

	// Recording the current tails for next process.
	
	prev_tail = Transform3D(Basis(), center).xform_inv(current_tail);
	current_tail = Transform3D(Basis(), center).xform_inv(next_tail);

	// Apply the rotation.
	Quaternion ft = Quaternion(get_rotation_relative_to_origin(skel).xform(bone_axis), next_tail - get_transform(skel).origin).normalized();
	ft = skel->get_global_transform().basis.get_rotation_quaternion().inverse() * ft;
	Quaternion qt = ft * get_rotation_relative_to_origin(skel);
	Transform3D global_pose_tr = get_global_pose(skel);
	global_pose_tr.basis = Basis(qt.normalized());
	BoneId bone_parent = skel->get_bone_parent(bone_idx);
	Transform3D parent_global_pose = skel->get_bone_global_pose_no_override(bone_parent);
	Transform3D local_pose = parent_global_pose.affine_inverse() * global_pose_tr;
	skel->set_bone_pose_position(bone_idx, local_pose.origin);
	skel->set_bone_pose_rotation(bone_idx, local_pose.basis.get_rotation_quaternion());
	skel->set_bone_pose_scale(bone_idx, local_pose.basis.get_scale());
}

Vector3 VRMSpringBoneLogic::collision(Skeleton3D *skel, const Array colliders, const Vector3 _next_tail) {
	Vector3 out = _next_tail;
	for (int i = 0; i < colliders.size(); ++i) {
		Ref<SphereCollider> collider = colliders[i];
		if (collider.is_null()) {
			continue;
		}
		float r = radius + collider->get_radius();
		Vector3 diff = out - collider->get_position();
		if (diff.length_squared() <= r * r) {
			// Hit, move to orientation of normal
			Vector3 normal = (out - collider->get_position()).normalized();
			Vector3 pos_from_collider = collider->get_position() + normal * (radius + collider->get_radius());
			// Limiting bone length
			Vector3 origin = get_transform(skel).origin;
			out = origin + (pos_from_collider - origin).normalized() * length;
		}
	}
	return out;
}
