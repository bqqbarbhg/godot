#pragma once

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

	~SecondaryGizmo();

	SecondaryGizmo(Node *parent) {
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

void SecondaryGizmo::_bind_methods() {
	ClassDB::bind_method(D_METHOD("draw_in_editor"), &SecondaryGizmo::draw_in_editor);
	ClassDB::bind_method(D_METHOD("draw_in_game"), &SecondaryGizmo::draw_in_game);
	ClassDB::bind_method(D_METHOD("draw_spring_bones", "color"), &SecondaryGizmo::draw_spring_bones);
	ClassDB::bind_method(D_METHOD("draw_collider_groups"), &SecondaryGizmo::draw_collider_groups);
	ClassDB::bind_method(D_METHOD("draw_line", "begin_pos", "end_pos", "color"), &SecondaryGizmo::draw_line);
	ClassDB::bind_method(D_METHOD("draw_sphere", "bas", "center", "radius", "color"), &SecondaryGizmo::draw_sphere);
}

void SecondaryGizmo::draw_spring_bones(const Color &color) {
	if (!secondary_node) {
		return;
	}
	set_material_override(m);

	for (int i = 0; i < secondary_node->spring_bones_internal.size(); ++i) {
		Ref<VRMSpringBone> spring_bone = secondary_node->spring_bones_internal[i];
		mesh->surface_begin(Mesh::PRIMITIVE_LINES);

		for (int j = 0; j < spring_bone->verlets.size(); ++j) {
			Ref<VRMSpringBoneLogic> v = spring_bone->verlets[j];
			Transform3D s_tr = Transform3D();
			Skeleton3D *s_sk = spring_bone->skel;

			if (Engine::get_singleton()->is_editor_hint()) {
				s_sk = Object::cast_to<Skeleton3D>(secondary_node->get_node_or_null(spring_bone->get_skeleton()));
				if (v->bone_idx != -1) {
					s_tr = s_sk->get_bone_global_pose(v->bone_idx);
				}
			} else {
				s_tr = spring_bone->skel->get_bone_global_pose_no_override(v->bone_idx);
			}

			draw_line(s_tr.origin, VRMUtil::inv_transform_point(s_sk->get_global_transform(), v->get_current_tail()), color);
		}

		mesh->surface_end();

		for (int j = 0; j < spring_bone->verlets.size(); ++j) {
			Ref<VRMSpringBoneLogic> v = spring_bone->verlets[j];
			Transform3D s_tr = Transform3D();
			Skeleton3D *s_sk = spring_bone->skel;

			if (Engine::get_singleton()->is_editor_hint()) {
				s_sk = Object::cast_to<Skeleton3D>(secondary_node->get_node_or_null(spring_bone->skeleton));
				if (v->bone_idx != -1) {
					s_tr = s_sk->get_bone_global_pose(v->bone_idx);
				}
			} else {
				s_tr = spring_bone->skel->get_bone_global_pose_no_override(v->bone_idx);
			}

			draw_sphere(s_tr.basis, VRMUtil::inv_transform_point(s_sk->get_global_transform(), v->get_current_tail()), spring_bone->get_hit_radius(), color);
		}

		mesh->surface_end();
	}
}

void SecondaryGizmo::draw_collider_groups() {
	if (!secondary_node) {
		return;
	}
	set_material_override(m);

	TypedArray<VRMColliderGroup> collider_groups = Engine::get_singleton()->is_editor_hint() ? secondary_node->get_collider_groups() : secondary_node->get_collider_groups_internal();

	for (int i = 0; i < collider_groups.size(); ++i) {
		Ref<VRMColliderGroup> collider_group = collider_groups[i];
		mesh->surface_begin(Mesh::PRIMITIVE_LINE_STRIP);

		Transform3D c_tr = Transform3D();

		if (Engine::get_singleton()->is_editor_hint()) {
			Node *c_node = secondary_node->get_node_or_null(collider_group->skeleton_or_node);
			Skeleton3D *c_sk = Object::cast_to<Skeleton3D>(c_node);
			if (c_sk) {
				if (collider_group->get_bone_idx() == -1) {
					collider_group->set_bone_idx(c_sk->find_bone(collider_group->bone));
				}
				c_tr = c_sk->get_bone_global_pose(collider_group->get_bone_idx());
			}
		} else if (Object::cast_to<Skeleton3D>(collider_group->get_parent())) {
			Skeleton3D *c_sk = Object::cast_to<Skeleton3D>(collider_group->get_parent());
			if (c_sk && collider_group->get_skel()) {
				c_tr = collider_group->get_skel()->get_bone_global_pose_no_override(c_sk->find_bone(collider_group->get_bone()));
			}
		}
		Array sphere_colliders = collider_group->get_sphere_colliders();
		for (int j = 0; j < sphere_colliders.size(); ++j) {
			Plane collider = sphere_colliders[j];
			Vector3 c_ps = Vector3(collider.normal.x, collider.normal.y, collider.normal.z);
			draw_sphere(c_tr.basis, VRMUtil::transform_point(c_tr, c_ps), collider.d, collider_group->get_gizmo_color());
		}

		mesh->surface_end();
	}
}
void SecondaryGizmo::draw_line(Vector3 begin_pos, Vector3 end_pos, Color color) {
	mesh->surface_set_color(color);
	mesh->surface_add_vertex(begin_pos);
	mesh->surface_set_color(color);
	mesh->surface_add_vertex(end_pos);
}

void SecondaryGizmo::draw_sphere(Basis bas, Vector3 center, float radius, const Color color) {
	int step = 15;
	float sppi = 2 * Math_PI / step;

	for (int i = 0; i <= step; ++i) {
		mesh->surface_set_color(color);
		mesh->surface_add_vertex(center + ((bas.xform(Vector3(0, radius, 0))).rotated(bas.xform(Vector3(1, 0, 0)), sppi * (i % step))));
	}

	for (int i = 0; i <= step; ++i) {
		mesh->surface_set_color(color);
		mesh->surface_add_vertex(center + ((bas.xform(Vector3(radius, 0, 0))).rotated(bas.xform(Vector3(0, 0, 1)), sppi * (i % step))));
	}

	for (int i = 0; i <= step; ++i) {
		mesh->surface_set_color(color);
		mesh->surface_add_vertex(center + ((bas.xform(Vector3(0, 0, radius))).rotated(bas.xform(Vector3(0, 1, 0)), sppi * (i % step))));
	}
}