/**************************************************************************/
/*  vrm_springbone.h                                                      */
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

#ifndef VRM_VRMSPRINGBONE_H
#define VRM_VRMSPRINGBONE_H

#include "core/io/resource.h"
#include "scene/3d/skeleton_3d.h"
#include "vrm_spring_bone_logic.h"

class VRMSpringBone : public Resource {
	GDCLASS(VRMSpringBone, Resource);

public:
	// Annotation comment
	String comment;

	// The resilience of the swaying object (the power of returning to the initial pose).
	float stiffness_force = 1.0f;
	// The strength of gravity.
	float gravity_power = 0.0f;

	// The direction of gravity.
	Vector3 gravity_dir = Vector3(0.0, -1.0, 0.0);

	// The resistance (deceleration) of automatic animation.
	float drag_force = 0.4f;

	// Bone name references are only valid within a given Skeleton.
	NodePath skeleton;

	// The reference point of a swaying object can be set at any location except the origin.
	String center_bone = "";
	NodePath center_node;

	// The radius of the sphere used for the collision detection with colliders.
	float hit_radius = 0.02f;

	// bone name of the root bone of the swaying object, within skeleton.
	Array root_bones;

	// Reference to the vrm_collidergroup for collisions with swaying objects.
	Array collider_groups;

	Array verlets;
	Array colliders;
	Vector3 center;
	Skeleton3D *skel = nullptr;

public:
	String get_comment() const;
	void set_comment(const String &p_comment);
	float get_stiffness_force() const;
	void set_stiffness_force(float p_stiffness_force);
	float get_gravity_power() const;
	void set_gravity_power(float p_gravity_power);
	Vector3 get_gravity_dir() const;
	void set_gravity_dir(const Vector3 &p_gravity_dir);
	float get_drag_force() const;
	void set_drag_force(float p_drag_force);
	NodePath get_skeleton() const;
	void set_skeleton(const NodePath &p_skeleton);
	String get_center_bone() const;
	void set_center_bone(const String &p_center_bone);
	NodePath get_center_node() const;
	void set_center_node(const NodePath &p_center_node);
	float get_hit_radius() const;
	void set_hit_radius(float p_hit_radius);
	Array get_root_bones() const;
	void set_root_bones(const Array &p_root_bones);
	Array get_collider_groups() const;
	void set_collider_groups(const Array &p_collider_groups);

protected:
	static void _bind_methods();

public:
	void setup(bool force = false);
	void process(float delta);
	void setup_recursive(int id, Transform3D center_tr);
	void ready(Skeleton3D *ready_skel, Array colliders_ref);
};

#endif // VRMSPRINGBONE_H