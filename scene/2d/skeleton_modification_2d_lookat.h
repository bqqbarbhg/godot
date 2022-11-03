/*************************************************************************/
/*  skeleton_modification_2d_lookat.h                                    */
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

#ifndef SKELETON_MODIFICATION_2D_LOOKAT_H
#define SKELETON_MODIFICATION_2D_LOOKAT_H

#include "scene/2d/skeleton_2d.h"
#include "scene/2d/skeleton_modification_2d.h"

///////////////////////////////////////
// SkeletonModification2DLookAt
///////////////////////////////////////

class SkeletonModification2DLookAt : public SkeletonModification2D {
	GDCLASS(SkeletonModification2DLookAt, SkeletonModification2D);

private:
	NodePath bone_node;
	mutable Variant bone_node_cache;

	NodePath target_node;
	mutable Variant target_node_cache;

	float additional_rotation = 0;
	bool enable_constraint = false;
	float constraint_angle_min = 0;
	float constraint_angle_max = (2.0 * Math_PI);
	bool constraint_angle_invert = false;
	bool constraint_in_localspace = true;

protected:
	static void _bind_methods();
	void execute(real_t delta) override;
	void draw_editor_gizmo() override;
	bool is_property_hidden(String property_name) const override;
	PackedStringArray get_configuration_warnings() const override;

public:
	void set_bone_node(const NodePath &p_target_node);
	NodePath get_bone_node() const;

	void set_target_node(const NodePath &p_target_node);
	NodePath get_target_node() const;

	void set_additional_rotation(float p_rotation);
	float get_additional_rotation() const;

	void set_enable_constraint(bool p_constraint);
	bool get_enable_constraint() const;
	void set_constraint_angle_min(float p_angle_min);
	float get_constraint_angle_min() const;
	void set_constraint_angle_max(float p_angle_max);
	float get_constraint_angle_max() const;
	void set_constraint_angle_invert(bool p_invert);
	bool get_constraint_angle_invert() const;
	void set_constraint_in_localspace(bool p_constraint_in_localspace);
	bool get_constraint_in_localspace() const;

	SkeletonModification2DLookAt();
	~SkeletonModification2DLookAt();
};

#endif // SKELETON_MODIFICATION_2D_LOOKAT_H
