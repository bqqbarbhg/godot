/**************************************************************************/
/*  vrm_collidergroup.cpp                                                 */
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

#include "vrm_collidergroup.h"

void SphereCollider::update(Node3D *parent, Skeleton3D *skel) {
	if (parent->get_class() == "Skeleton3D" && idx != -1) {
		Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(parent);
		position = VRMUtil::transform_point(Transform3D(Basis(), skeleton->get_global_transform().xform(skel->get_bone_global_pose(idx).origin)), offset);
	} else {
		position = VRMUtil::transform_point(parent->get_global_transform(), offset);
	}
}

float SphereCollider::get_radius() const {
	return radius;
}

Vector3 SphereCollider::get_position() const {
	return position;
}

void VRMColliderGroup::_bind_methods() {
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

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "skeleton_or_node"), "set_skeleton_or_node", "get_skeleton_or_node");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "bone"), "set_bone", "get_bone");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "sphere_colliders"), "set_sphere_colliders", "get_sphere_colliders");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "gizmo_color"), "set_gizmo_color", "get_gizmo_color");
}