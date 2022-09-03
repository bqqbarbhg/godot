/*************************************************************************/
/*  skeleton_modification_2d_twoboneik.h                                 */
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

#ifndef SKELETON_MODIFICATION_2D_TWOBONEIK_H
#define SKELETON_MODIFICATION_2D_TWOBONEIK_H

#include "scene/2d/skeleton_2d.h"
#include "scene/2d/skeleton_modification_2d.h"

///////////////////////////////////////
// SkeletonModification2DJIGGLE
///////////////////////////////////////

class SkeletonModification2DTwoBoneIK : public SkeletonModification2D {
	GDCLASS(SkeletonModification2DTwoBoneIK, SkeletonModification2D);

private:
	NodePath target_node;
	mutable Variant target_node_cache;
	float target_minimum_distance = 0;
	float target_maximum_distance = 0;
	bool flip_bend_direction = false;

	NodePath joint_one_bone_node;
	mutable Variant joint_one_bone_node_cache;

	NodePath joint_two_bone_node;
	mutable Variant joint_two_bone_node_cache;

protected:
	static void _bind_methods();
	void execute(real_t delta) override;
	TypedArray<String> get_configuration_warnings() const override;
	void draw_editor_gizmo() override;

public:
	void set_target_node(const NodePath &p_target_node);
	NodePath get_target_node() const;

	void set_target_minimum_distance(float p_minimum_distance);
	float get_target_minimum_distance() const;
	void set_target_maximum_distance(float p_maximum_distance);
	float get_target_maximum_distance() const;
	void set_flip_bend_direction(bool p_flip_direction);
	bool get_flip_bend_direction() const;

	void set_joint_one_bone_node(const NodePath &p_node);
	NodePath get_joint_one_bone_node() const;

	void set_joint_two_bone_node(const NodePath &p_node);
	NodePath get_joint_two_bone_node() const;

	SkeletonModification2DTwoBoneIK();
	~SkeletonModification2DTwoBoneIK();
};

#endif // SKELETON_MODIFICATION_2D_TWOBONEIK_H
