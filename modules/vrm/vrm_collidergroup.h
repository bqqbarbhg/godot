/**************************************************************************/
/*  vrm_collidergroup.h                                                   */
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

#ifndef VRM_COLLIDERGROUP_H
#define VRM_COLLIDERGROUP_H

#include "core/math/color.h"
#include "core/object/ref_counted.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"

#include "vrm_toplevel.h"

class SphereCollider : public Resource {
	GDCLASS(SphereCollider, Resource);

public:
	int idx = -1;
	Vector3 offset;
	float radius = 0;
	Vector3 position;
	SphereCollider(int bone_idx, const Vector3 &collider_offset = Vector3(0, 0, 0), float collider_radius = 0.1) :
			idx(bone_idx), offset(collider_offset), radius(collider_radius) {}
	void update(Node3D *parent, Skeleton3D *skel);
	float get_radius() const;
	Vector3 get_position() const;
};

class VRMColliderGroup : public Resource {
	GDCLASS(VRMColliderGroup, Resource);

protected:
	static void _bind_methods();

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

	void ready(Skeleton3D *ready_skel) {
		parent = ready_skel;
		if (ready_skel) {
			skel = cast_to<Skeleton3D>(ready_skel);
			bone_idx = cast_to<Skeleton3D>(ready_skel)->find_bone(bone);
		}
		setup();
	}

	void process() {
		for (int i = 0; i < colliders.size(); ++i) {
			Ref<SphereCollider> collider = Object::cast_to<SphereCollider>(colliders[i]);
			if (collider.is_null()) {
				continue;
			}
			collider->update(parent, skel);
		}
	}
};

#endif // VRM_COLLIDER_GROUP_H