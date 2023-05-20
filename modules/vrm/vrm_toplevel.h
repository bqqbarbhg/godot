/**************************************************************************/
/*  vrm_toplevel.h                                                        */
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

#ifndef VRM_VRMTOPLEVEL_H
#define VRM_VRMTOPLEVEL_H

#include "core/math/quaternion.h"
#include "core/math/transform_3d.h"
#include "scene/3d/node_3d.h"

class VRMUtil {
public:
	static Quaternion from_to_rotation(const Vector3 &from, const Vector3 &to);
	static Vector3 transform_point(const Transform3D &transform, const Vector3 &point);
	static Vector3 inv_transform_point(const Transform3D &transform, const Vector3 &point);
};

Quaternion VRMUtil::from_to_rotation(const Vector3 &from, const Vector3 &to) {
	Vector3 axis = from.cross(to);
	if (Math::is_equal_approx(axis.x, 0.0) && Math::is_equal_approx(axis.y, 0.0) && Math::is_equal_approx(axis.z, 0.0)) {
		return Quaternion();
	}
	float angle = from.angle_to(to);
	if (Math::is_equal_approx(angle, 0.0f)) {
		angle = 0.0f;
	}
	return Quaternion(axis.normalized(), angle);
}

Vector3 VRMUtil::transform_point(const Transform3D &transform, const Vector3 &point) {
	Vector3 sc = transform.basis.get_scale();
	return (transform.basis.get_rotation_quaternion().xform(Vector3(point.x * sc.x, point.y * sc.y, point.z * sc.z)) + transform.origin);
}

Vector3 VRMUtil::inv_transform_point(const Transform3D &transform, const Vector3 &point) {
	Vector3 diff = point - transform.origin;
	Vector3 sc = transform.basis.get_scale();
	return transform.basis.get_rotation_quaternion().inverse().xform(Vector3(diff.x / sc.x, diff.y / sc.y, diff.z / sc.z));
}

class VRMTopLevel : public Node3D {
	GDCLASS(VRMTopLevel, Node3D);

private:
	NodePath vrm_skeleton;
	NodePath vrm_animplayer;
	NodePath vrm_secondary;

	Ref<Resource> vrm_meta;

	bool update_secondary_fixed = false;
	bool update_in_editor = false;
	bool gizmo_spring_bone = false;
	Color gizmo_spring_bone_color = Color::named("lightyellow");

protected:
	static void _bind_methods();

public:
	// Getters and setters for the exported variables
	void set_vrm_skeleton(const NodePath &path);
	NodePath get_vrm_skeleton() const;

	void set_vrm_animplayer(const NodePath &path);
	NodePath get_vrm_animplayer() const;

	void set_vrm_secondary(const NodePath &path);
	NodePath get_vrm_secondary() const;

	void set_vrm_meta(const Ref<Resource> &meta);
	Ref<Resource> get_vrm_meta() const;

	void set_update_secondary_fixed(bool value);
	bool get_update_secondary_fixed() const;

	void set_update_in_editor(bool value);
	bool get_update_in_editor() const;

	void set_gizmo_spring_bone(bool value);
	bool get_gizmo_spring_bone() const;

	void set_gizmo_spring_bone_color(const Color &color);
	Color get_gizmo_spring_bone_color() const;
};

void VRMTopLevel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_vrm_skeleton", "path"), &VRMTopLevel::set_vrm_skeleton);
	ClassDB::bind_method(D_METHOD("get_vrm_skeleton"), &VRMTopLevel::get_vrm_skeleton);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "vrm_skeleton"), "set_vrm_skeleton", "get_vrm_skeleton");

	ClassDB::bind_method(D_METHOD("set_vrm_animplayer", "path"), &VRMTopLevel::set_vrm_animplayer);
	ClassDB::bind_method(D_METHOD("get_vrm_animplayer"), &VRMTopLevel::get_vrm_animplayer);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "vrm_animplayer"), "set_vrm_animplayer", "get_vrm_animplayer");

	ClassDB::bind_method(D_METHOD("set_vrm_secondary", "path"), &VRMTopLevel::set_vrm_secondary);
	ClassDB::bind_method(D_METHOD("get_vrm_secondary"), &VRMTopLevel::get_vrm_secondary);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "vrm_secondary"), "set_vrm_secondary", "get_vrm_secondary");

	ClassDB::bind_method(D_METHOD("set_vrm_meta", "meta"), &VRMTopLevel::set_vrm_meta);
	ClassDB::bind_method(D_METHOD("get_vrm_meta"), &VRMTopLevel::get_vrm_meta);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "vrm_meta", PROPERTY_HINT_RESOURCE_TYPE, "Resource"), "set_vrm_meta", "get_vrm_meta");

	ClassDB::bind_method(D_METHOD("set_update_secondary_fixed", "value"), &VRMTopLevel::set_update_secondary_fixed);
	ClassDB::bind_method(D_METHOD("get_update_secondary_fixed"), &VRMTopLevel::get_update_secondary_fixed);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "update_secondary_fixed"), "set_update_secondary_fixed", "get_update_secondary_fixed");

	ClassDB::bind_method(D_METHOD("set_update_in_editor", "value"), &VRMTopLevel::set_update_in_editor);
	ClassDB::bind_method(D_METHOD("get_update_in_editor"), &VRMTopLevel::get_update_in_editor);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "update_in_editor"), "set_update_in_editor", "get_update_in_editor");

	ClassDB::bind_method(D_METHOD("set_gizmo_spring_bone", "value"), &VRMTopLevel::set_gizmo_spring_bone);
	ClassDB::bind_method(D_METHOD("get_gizmo_spring_bone"), &VRMTopLevel::get_gizmo_spring_bone);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "gizmo_spring_bone"), "set_gizmo_spring_bone", "get_gizmo_spring_bone");

	ClassDB::bind_method(D_METHOD("set_gizmo_spring_bone_color", "color"), &VRMTopLevel::set_gizmo_spring_bone_color);
	ClassDB::bind_method(D_METHOD("get_gizmo_spring_bone_color"), &VRMTopLevel::get_gizmo_spring_bone_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "gizmo_spring_bone_color"), "set_gizmo_spring_bone_color", "get_gizmo_spring_bone_color");
}

// Implement the getters and setters for the exported variables
void VRMTopLevel::set_vrm_skeleton(const NodePath &path) {
	vrm_skeleton = path;
}

NodePath VRMTopLevel::get_vrm_skeleton() const {
	return vrm_skeleton;
}

void VRMTopLevel::set_vrm_animplayer(const NodePath &path) {
	vrm_animplayer = path;
}

NodePath VRMTopLevel::get_vrm_animplayer() const {
	return vrm_animplayer;
}

void VRMTopLevel::set_vrm_secondary(const NodePath &path) {
	vrm_secondary = path;
}

NodePath VRMTopLevel::get_vrm_secondary() const {
	return vrm_secondary;
}

void VRMTopLevel::set_vrm_meta(const Ref<Resource> &meta) {
	vrm_meta = meta;
}

Ref<Resource> VRMTopLevel::get_vrm_meta() const {
	return vrm_meta;
}

void VRMTopLevel::set_update_secondary_fixed(bool value) {
	update_secondary_fixed = value;
}

bool VRMTopLevel::get_update_secondary_fixed() const {
	return update_secondary_fixed;
}

void VRMTopLevel::set_update_in_editor(bool value) {
	update_in_editor = value;
}

bool VRMTopLevel::get_update_in_editor() const {
	return update_in_editor;
}

void VRMTopLevel::set_gizmo_spring_bone(bool value) {
	gizmo_spring_bone = value;
}

bool VRMTopLevel::get_gizmo_spring_bone() const {
	return gizmo_spring_bone;
}

void VRMTopLevel::set_gizmo_spring_bone_color(const Color &color) {
	gizmo_spring_bone_color = color;
}

Color VRMTopLevel::get_gizmo_spring_bone_color() const {
	return gizmo_spring_bone_color;
}

#endif // VRM_VRMTOPLEVEL_H