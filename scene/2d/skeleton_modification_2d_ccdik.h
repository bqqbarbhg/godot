/*************************************************************************/
/*  skeleton_modification_2d_ccdik.h                                     */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef SKELETON_MODIFICATION_2D_CCDIK_H
#define SKELETON_MODIFICATION_2D_CCDIK_H

#include "scene/2d/skeleton_2d.h"
#include "scene/2d/skeleton_modification_2d.h"

///////////////////////////////////////
// SkeletonModification2DCCDIK
///////////////////////////////////////

class SkeletonModification2DCCDIK : public SkeletonModification2D {
	GDCLASS(SkeletonModification2DCCDIK, SkeletonModification2D);

private:
	struct CCDIK_Joint_Data2D {
		NodePath bone_node;
		mutable Variant bone_node_cache;
		bool rotate_from_joint = false;

		bool enable_constraint = false;
		float constraint_angle_min = 0;
		float constraint_angle_max = (2.0 * Math_PI);
		bool constraint_angle_invert = false;
		bool constraint_in_localspace = true;
	};

	Vector<CCDIK_Joint_Data2D> ccdik_data_chain;

	NodePath target_node;
	mutable Variant target_node_cache;

	NodePath tip_node;
	mutable Variant tip_node_cache;
	void _execute_ccdik_joint(int p_joint_idx, Vector2 p_target_position, Vector2 p_tip_position);

protected:
	static void _bind_methods();
	bool _set(const StringName &p_path, const Variant &p_value);
	bool _get(const StringName &p_path, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	void execute(real_t p_delta) override;
	void draw_editor_gizmo() override;
	TypedArray<String> get_configuration_warnings() const override;

public:
	void set_target_node(const NodePath &p_target_node);
	NodePath get_target_node() const;
	void set_tip_node(const NodePath &p_tip_node);
	NodePath get_tip_node() const;

	int get_joint_count();
	void set_joint_count(int p_new_length);

	void set_joint_bone_node(int p_joint_idx, const NodePath &p_target_node);
	NodePath get_joint_bone_node(int p_joint_idx) const;

	void set_joint_rotate_from_joint(int p_joint_idx, bool p_rotate_from_joint);
	bool get_joint_rotate_from_joint(int p_joint_idx) const;
	void set_joint_enable_constraint(int p_joint_idx, bool p_constraint);
	bool get_joint_enable_constraint(int p_joint_idx) const;
	void set_joint_constraint_angle_min(int p_joint_idx, float p_angle_min);
	float get_joint_constraint_angle_min(int p_joint_idx) const;
	void set_joint_constraint_angle_max(int p_joint_idx, float p_angle_max);
	float get_joint_constraint_angle_max(int p_joint_idx) const;
	void set_joint_constraint_angle_invert(int p_joint_idx, bool p_invert);
	bool get_joint_constraint_angle_invert(int p_joint_idx) const;
	void set_joint_constraint_in_localspace(int p_joint_idx, bool p_constraint_in_localspace);
	bool get_joint_constraint_in_localspace(int p_joint_idx) const;

	SkeletonModification2DCCDIK();
	~SkeletonModification2DCCDIK();
};

#endif // SKELETON_MODIFICATION_2D_CCDIK_H
