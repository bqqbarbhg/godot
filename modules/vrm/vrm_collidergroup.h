#pragma once

#include "core/math/color.h"
#include "core/object/ref_counted.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"

#include "vrm_collidergroup.h"
#include "vrm_toplevel.h"

class SphereCollider : public Resource {
	GDCLASS(SphereCollider, Resource);

protected:
	static void _bind_methods() {
	}

public:
	int idx = -1;
	Vector3 offset;
	float radius = 0;
	Vector3 position;

	SphereCollider(int bone_idx, const Vector3 &collider_offset = Vector3(0, 0, 0), float collider_radius = 0.1) :
			idx(bone_idx), offset(collider_offset), radius(collider_radius) {}

	void update(Node3D *parent, Skeleton3D *skel) {
		if (parent->get_class() == "Skeleton3D" && idx != -1) {
			Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(parent);
			position = VRMUtil::transform_point(Transform3D(Basis(), skeleton->get_global_transform().xform(skel->get_bone_global_pose(idx).origin)), offset);
		} else {
			position = VRMUtil::transform_point(parent->get_global_transform(), offset);
		}
	}
	float get_radius() const {
		return radius;
	}
	Vector3 get_position() const {
		return position;
	}
};

class VRMColliderGroup : public Resource {
	GDCLASS(VRMColliderGroup, Resource);

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("setup"), &VRMColliderGroup::setup);
		ClassDB::bind_method(D_METHOD("_ready", "ready_parent", "ready_skeleton"), &VRMColliderGroup::_ready);
		ClassDB::bind_method(D_METHOD("_process"), &VRMColliderGroup::_process);

		ClassDB::bind_method(D_METHOD("get_skeleton_or_node"), &VRMColliderGroup::get_skeleton_or_node);
		ClassDB::bind_method(D_METHOD("set_skeleton_or_node", "skeleton_or_node"), &VRMColliderGroup::set_skeleton_or_node);
		ClassDB::bind_method(D_METHOD("get_bone"), &VRMColliderGroup::get_bone);
		ClassDB::bind_method(D_METHOD("set_bone", "bone"), &VRMColliderGroup::set_bone);
		ClassDB::bind_method(D_METHOD("get_sphere_colliders"), &VRMColliderGroup::get_sphere_colliders);
		ClassDB::bind_method(D_METHOD("set_sphere_colliders", "sphere_colliders"), &VRMColliderGroup::set_sphere_colliders);
		ClassDB::bind_method(D_METHOD("get_gizmo_color"), &VRMColliderGroup::get_gizmo_color);
		ClassDB::bind_method(D_METHOD("set_gizmo_color", "gizmo_color"), &VRMColliderGroup::set_gizmo_color);

		// Bind properties
		ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "skeleton_or_node"), "set_skeleton_or_node", "get_skeleton_or_node");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "bone"), "set_bone", "get_bone");
		ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "sphere_colliders"), "set_sphere_colliders", "get_sphere_colliders");
		ADD_PROPERTY(PropertyInfo(Variant::COLOR, "gizmo_color"), "set_gizmo_color", "get_gizmo_color");
	}

public:
	NodePath skeleton_or_node;
	String bone;
	Array sphere_colliders;
	Color gizmo_color = Color::named("lightyellow");

	NodePath get_skeleton_or_node() const { return skeleton_or_node; }
	String get_bone() const { return bone; }
	Array get_sphere_colliders() const { return sphere_colliders; }
	Color get_gizmo_color() const { return gizmo_color; }

	void set_skeleton_or_node(const NodePath &p_skeleton_or_node) { skeleton_or_node = p_skeleton_or_node; }
	void set_bone(const String &p_bone) { bone = p_bone; }
	void set_sphere_colliders(const Array &p_sphere_colliders) { sphere_colliders = p_sphere_colliders; }
	void set_gizmo_color(const Color &p_gizmo_color) { gizmo_color = p_gizmo_color; }

	Array colliders;
	int bone_idx = -1;
	int get_bone_idx() const { return bone_idx; }
	void set_bone_idx(int p_bone_idx) { bone_idx = p_bone_idx; }
	Node3D *parent = nullptr;
	Skeleton3D *skel = nullptr;
	Skeleton3D *get_skel() const { return skel; }

	void set_parent(Node3D *p_parent) {
		parent = p_parent;
	}

	Node3D *get_parent() {
		return parent;
	}

	void setup() {
		if (parent) {
			colliders.clear();
			for (int i = 0; i < sphere_colliders.size(); ++i) {
				Plane collider = sphere_colliders[i];
				Ref<SphereCollider> new_collider = memnew(SphereCollider(bone_idx, collider.normal, collider.d));
				colliders.append(new_collider);
			}
		}
	}

	void _ready(Node3D *ready_parent, Object *ready_skel) {
		parent = ready_parent;
		if (cast_to<Skeleton3D>(ready_parent)) {
			skel = cast_to<Skeleton3D>(ready_skel);
			bone_idx = cast_to<Skeleton3D>(ready_parent)->find_bone(bone);
		}
		setup();
	}

	void _process() {
		for (int i = 0; i < colliders.size(); ++i) {
			Ref<SphereCollider> collider = Object::cast_to<SphereCollider>(colliders[i]);
			collider->update(parent, skel);
		}
	}
};
