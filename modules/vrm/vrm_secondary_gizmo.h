/**************************************************************************/
/*  vrm_secondary_gizmo.h                                                 */
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

#ifndef VRM_SECONDARY_GIZMO_H
#define VRM_SECONDARY_GIZMO_H

#include "modules/vrm/vrm_collidergroup.h"
#include "modules/vrm/vrm_secondary.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/immediate_mesh.h"
#include "scene/resources/material.h"
#include "vrm_spring_bone_logic.h"
#include "vrm_springbone.h"
#include "vrm_toplevel.h"

class SecondaryGizmo : public MeshInstance3D {
	GDCLASS(SecondaryGizmo, MeshInstance3D);

protected:
	static void _bind_methods();

public:
	VRMSecondary *secondary_node = nullptr;
	Ref<StandardMaterial3D> m;
	Ref<ImmediateMesh> mesh;

	~SecondaryGizmo() {}

	SecondaryGizmo(Node *parent = nullptr) {
		mesh.instantiate();
		secondary_node = cast_to<VRMSecondary>(parent);
		m->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_DISABLED);
		m->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
		m->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
	}

	void draw_in_editor() {
		mesh->clear_surfaces();
		if (secondary_node && Object::cast_to<VRMTopLevel>(secondary_node->get_parent())) {
			draw_spring_bones(Object::cast_to<VRMTopLevel>(secondary_node->get_parent())->get_gizmo_spring_bone_color());
			draw_collider_groups();
		}
	}

	void draw_in_game() {
		mesh->clear_surfaces();
		if (secondary_node && Object::cast_to<VRMTopLevel>(secondary_node->get_parent())) {
			draw_spring_bones(Object::cast_to<VRMTopLevel>(secondary_node->get_parent())->get_gizmo_spring_bone_color());
			draw_collider_groups();
		}
	}

	void draw_spring_bones(const Color &color);
	void draw_collider_groups();
	void draw_line(Vector3 begin_pos, Vector3 end_pos, Color color);
	void draw_sphere(Basis bas, Vector3 center, float radius, Color color);
};

#endif // VRM_SECONDARY_GIZMO_H