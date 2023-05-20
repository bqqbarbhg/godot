#pragma once

#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/material.h"

#include "vrm_toplevel.h"

class SecondaryGizmo;

class VRMSecondary : public Node3D {
	GDCLASS(VRMSecondary, Node3D);

protected:
	static void _bind_methods();

public:
	Array spring_bones;
	Array collider_groups;

	bool update_secondary_fixed = false;
	bool update_in_editor = false;

	Array spring_bones_internal;
	Array collider_groups_internal;
	SecondaryGizmo *secondary_gizmo = nullptr;

	void set_spring_bones(const Array &new_spring_bones);
	void set_collider_groups(const Array &new_collider_groups);
	Array get_spring_bones() const;
	Array get_collider_groups() const;
	Array get_collider_groups_internal() const {
		return collider_groups_internal;
	}
	void set_collider_groups_internal(Array p_collider_groups_internal) {
		collider_groups_internal = p_collider_groups_internal;
	}
	bool check_for_editor_update();
	void _notification(int p_what);
	VRMSecondary() {}
};
