/**************************************************************************/
/*  vrm_spring_bone_logic.h                                               */
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

	void ready(Skeleton3D *skel, int idx, const Vector3 &center, const Vector3 &local_child_position, const Transform3D &default_pose);
	void update(Skeleton3D *skel, const Vector3 &center, float stiffness_force, float drag_force, const Vector3 &external, const Array &colliders);
	Vector3 collision(Skeleton3D *skel, const Array &colliders, const Vector3 &_next_tail);
	VRMSpringBoneLogic() {}
};

#endif // VRM_BONE_LOGIC_H