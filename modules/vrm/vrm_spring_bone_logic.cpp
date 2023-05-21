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
	ClassDB::bind_method(D_METHOD("global_pose_to_local_pose", "skeleton", "bone_idx", "global_pose"), &VRMSpringBoneLogic::global_pose_to_local_pose);
	ClassDB::bind_method(D_METHOD("local_pose_to_global_pose", "skeleton", "bone_idx", "local_pose"), &VRMSpringBoneLogic::local_pose_to_global_pose);

	ClassDB::bind_method(D_METHOD("get_transform", "skeleton"), &VRMSpringBoneLogic::get_transform);
	ClassDB::bind_method(D_METHOD("get_rotation_relative_to_origin", "skeleton"), &VRMSpringBoneLogic::get_rotation_relative_to_origin);
	ClassDB::bind_method(D_METHOD("get_global_pose", "skeleton"), &VRMSpringBoneLogic::get_global_pose);
	ClassDB::bind_method(D_METHOD("get_local_pose_rotation", "skeleton"), &VRMSpringBoneLogic::get_local_pose_rotation);
	ClassDB::bind_method(D_METHOD("reset", "skeleton"), &VRMSpringBoneLogic::reset);

	ClassDB::bind_method(D_METHOD("update", "skeleton", "center", "stiffness_force", "drag_force", "external", "colliders"), &VRMSpringBoneLogic::update);
	ClassDB::bind_method(D_METHOD("collision", "skeleton", "colliders", "next_tail"), &VRMSpringBoneLogic::collision);
}

Transform3D VRMSpringBoneLogic::global_pose_to_local_pose(Skeleton3D *p_skeleton, int p_bone_idx, Transform3D p_global_pose) {
	int bone_size = p_skeleton->get_bone_count();
	if (p_bone_idx < 0 || p_bone_idx >= bone_size) {
		return Transform3D();
	}
	if (p_skeleton->get_bone_parent(p_bone_idx) >= 0) {
		int parent_bone_idx = p_skeleton->get_bone_parent(p_bone_idx);
		Transform3D conversion_transform = p_skeleton->get_bone_global_pose(parent_bone_idx).affine_inverse();
		return conversion_transform * p_global_pose;
	} else {
		return p_global_pose;
	}
}

Transform3D VRMSpringBoneLogic::local_pose_to_global_pose(Skeleton3D *p_skeleton, int p_bone_idx, Transform3D p_local_pose) {
	int bone_size = p_skeleton->get_bone_count();
	if (p_bone_idx < 0 || p_bone_idx >= bone_size) {
		return Transform3D();
	}
	if (p_skeleton->get_bone_parent(p_bone_idx) >= 0) {
		int parent_bone_idx = p_skeleton->get_bone_parent(p_bone_idx);
		return p_skeleton->get_bone_global_pose(parent_bone_idx) * p_local_pose;
	} else {
		return p_local_pose;
	}
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

void VRMSpringBoneLogic::ready(Skeleton3D *skel, int idx, const Vector3 &center, const Vector3 &local_child_position, const Transform3D &default_pose) {
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

void VRMSpringBoneLogic::update(Skeleton3D *skel, const Vector3 &center, float stiffness_force, float drag_force, const Vector3 &external, const Array &colliders) {
	Vector3 tmp_current_tail, tmp_prev_tail;
	if (center != Vector3()) {
		tmp_current_tail = Transform3D(Basis(), center).xform(current_tail);
		tmp_prev_tail = Transform3D(Basis(), center).xform(prev_tail);
	} else {
		tmp_current_tail = current_tail;
		tmp_prev_tail = prev_tail;
	}

	// Integrate the velocity verlet.
	Vector3 next_tail = tmp_current_tail + (tmp_current_tail - tmp_prev_tail) * (1.0 - drag_force) + (get_rotation_relative_to_origin(skel).xform(bone_axis)) * stiffness_force + external;

	// Limit the bone length.
	Vector3 origin = get_transform(skel).origin;
	next_tail = origin + (next_tail - origin).normalized() * length;

	// Collide movement.
	next_tail = collision(skel, colliders, next_tail);

	// Recording the current tails for next process.
	if (center != Vector3()) {
		prev_tail = Transform3D(Basis(), center).xform_inv(current_tail);
		current_tail = Transform3D(Basis(), center).xform_inv(next_tail);
	} else {
		prev_tail = current_tail;
		current_tail = next_tail;
	}

	// Apply the rotation.
	Quaternion ft = Quaternion(get_rotation_relative_to_origin(skel).xform(bone_axis), next_tail - get_transform(skel).origin).normalized();
	ft = skel->get_global_transform().basis.get_rotation_quaternion().inverse() * ft;
	Quaternion qt = ft * get_rotation_relative_to_origin(skel);
	Transform3D global_pose_tr = get_global_pose(skel);
	global_pose_tr.basis = Basis(qt.normalized());
	skel->set_bone_global_pose_override(bone_idx, global_pose_tr, 1.0, true);
}

Vector3 VRMSpringBoneLogic::collision(Skeleton3D *skel, const Array &colliders, const Vector3 &_next_tail) {
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
