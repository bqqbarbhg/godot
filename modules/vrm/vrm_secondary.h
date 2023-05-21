/**************************************************************************/
/*  vrm_secondary.h                                                       */
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

#ifndef VRM_SECONDARY_H
#define VRM_SECONDARY_H

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

	void set_spring_bones(Array new_spring_bones);
	void set_collider_groups(Array new_collider_groups);
	Array get_spring_bones() const;
	Array get_collider_groups() const;
	Array get_collider_groups_internal() const;
	void set_collider_groups_internal(Array p_collider_groups_internal);
	bool check_for_editor_update();
	void _notification(int p_what);
	VRMSecondary() {}
	~VRMSecondary() {}
};

#endif // VRM_SECONDARY_H