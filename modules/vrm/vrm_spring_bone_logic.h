#ifndef VRM_BONE_LOGIC_H
#define VRM_BONE_LOGIC_H

#include "core/object/ref_counted.h"
#include "modules/vrm/vrm_constants.h"
#include "scene/3d/skeleton_3d.h"
#include "vrm_collidergroup.h"
#include "vrm_toplevel.h"

class VRMSpringBoneLogic : public RefCounted {
	GDCLASS(VRMSpringBoneLogic, RefCounted);

protected:
	static void _bind_methods();

public:
	bool force_update = true;
	int bone_idx = -1;

	float radius = 0;
	float length = 0;

	Vector3 bone_axis;
	Vector3 current_tail;
	Vector3 get_current_tail() const {
		return current_tail;
	}
	Vector3 prev_tail;

	Transform3D initial_transform;

	Transform3D global_pose_to_local_pose(Skeleton3D *p_skeleton, int p_bone_idx, Transform3D p_global_pose);
	Transform3D local_pose_to_global_pose(Skeleton3D *p_skeleton, int p_bone_idx, Transform3D p_local_pose);

	Transform3D get_transform(Skeleton3D *skel);
	Quaternion get_rotation_relative_to_origin(Skeleton3D *skel);
	Transform3D get_global_pose(Skeleton3D *skel);
	Quaternion get_local_pose_rotation(Skeleton3D *skel);
	void reset(Skeleton3D *skel);

	VRMSpringBoneLogic(Skeleton3D *skel, int idx, const Vector3 &center, const Vector3 &local_child_position, const Transform3D &default_pose);
	void update(Skeleton3D *skel, const Vector3 &center, float stiffness_force, float drag_force, const Vector3 &external, const Array &colliders);
	Vector3 collision(Skeleton3D *skel, const Array &colliders, const Vector3 &_next_tail);
	VRMSpringBoneLogic() {}
};

void VRMSpringBoneLogic::_bind_methods() {
	ClassDB::bind_method(D_METHOD("global_pose_to_local_pose", "p_skeleton", "p_bone_idx", "p_global_pose"), &VRMSpringBoneLogic::global_pose_to_local_pose);
	ClassDB::bind_method(D_METHOD("local_pose_to_global_pose", "p_skeleton", "p_bone_idx", "p_local_pose"), &VRMSpringBoneLogic::local_pose_to_global_pose);

	ClassDB::bind_method(D_METHOD("get_transform", "skel"), &VRMSpringBoneLogic::get_transform);
	ClassDB::bind_method(D_METHOD("get_rotation_relative_to_origin", "skel"), &VRMSpringBoneLogic::get_rotation_relative_to_origin);
	ClassDB::bind_method(D_METHOD("get_global_pose", "skel"), &VRMSpringBoneLogic::get_global_pose);
	ClassDB::bind_method(D_METHOD("get_local_pose_rotation", "skel"), &VRMSpringBoneLogic::get_local_pose_rotation);
	ClassDB::bind_method(D_METHOD("reset", "skel"), &VRMSpringBoneLogic::reset);

	ClassDB::bind_method(D_METHOD("update", "skel", "center", "stiffness_force", "drag_force", "external", "colliders"), &VRMSpringBoneLogic::update);
	ClassDB::bind_method(D_METHOD("collision", "skel", "colliders", "_next_tail"), &VRMSpringBoneLogic::collision);
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

VRMSpringBoneLogic::VRMSpringBoneLogic(Skeleton3D *skel, int idx, const Vector3 &center, const Vector3 &local_child_position, const Transform3D &default_pose) {
	initial_transform = default_pose;
	bone_idx = idx;
	Vector3 world_child_position = VRMUtil::transform_point(get_transform(skel), local_child_position);
	if (center != Vector3()) {
		current_tail = VRMUtil::inv_transform_point(Transform3D(Basis(), center), world_child_position);
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
		tmp_current_tail = VRMUtil::transform_point(Transform3D(Basis(), center), current_tail);
		tmp_prev_tail = VRMUtil::transform_point(Transform3D(Basis(), center), prev_tail);
	} else {
		tmp_current_tail = current_tail;
		tmp_prev_tail = prev_tail;
	}

	// Integration of velocity verlet
	Vector3 next_tail = tmp_current_tail + (tmp_current_tail - tmp_prev_tail) * (1.0 - drag_force) + (get_rotation_relative_to_origin(skel).xform(bone_axis)) * stiffness_force + external;

	// Limiting bone length
	Vector3 origin = get_transform(skel).origin;
	next_tail = origin + (next_tail - origin).normalized() * length;

	// Collision movement
	next_tail = collision(skel, colliders, next_tail);

	// Recording current tails for next process
	if (center != Vector3()) {
		prev_tail = VRMUtil::inv_transform_point(Transform3D(Basis(), center), current_tail);
		current_tail = VRMUtil::inv_transform_point(Transform3D(Basis(), center), next_tail);
	} else {
		prev_tail = current_tail;
		current_tail = next_tail;
	}

	// Apply rotation
	Quaternion ft = VRMUtil::from_to_rotation(get_rotation_relative_to_origin(skel).xform(bone_axis), next_tail - get_transform(skel).origin);
	if (ft != Quaternion()) {
		ft = skel->get_global_transform().basis.get_rotation_quaternion().inverse() * ft;
		Quaternion qt = ft * get_rotation_relative_to_origin(skel);
		Transform3D global_pose_tr = get_global_pose(skel);
		global_pose_tr.basis = Basis(qt);
		skel->set_bone_global_pose_override(bone_idx, global_pose_tr, 1.0, true);
	}
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

#endif // VRM_BONE_LOGIC_H