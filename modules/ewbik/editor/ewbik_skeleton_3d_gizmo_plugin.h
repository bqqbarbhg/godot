/*************************************************************************/
/*  ewbik_skeleton_3d_gizmo_plugin.h                                     */
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

#ifndef EWBIK_SKELETON_3D_GIZMO_PLUGIN_H
#define EWBIK_SKELETON_3D_GIZMO_PLUGIN_H

#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "editor/editor_node.h"
#include "editor/editor_properties.h"
#include "editor/plugins/node_3d_editor_plugin.h"
#include "modules/ewbik/src/ik_bone_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/label_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/immediate_mesh.h"

#include "../src/ik_ewbik.h"

class Joint;
class PhysicalBone3D;
class EWBIKSkeleton3DEditorPlugin;
class Button;

class EWBIK3DGizmoPlugin : public EditorNode3DGizmoPlugin {
	GDCLASS(EWBIK3DGizmoPlugin, EditorNode3DGizmoPlugin);

	SkeletonModification3DNBoneIK *ewbik = nullptr;
	Ref<Shader> kusudama_shader;
	
protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("_get_gizmo_name"), &EWBIK3DGizmoPlugin::get_gizmo_name);
	}

public:
	const Color bone_color = EditorSettings::get_singleton()->get("editors/3d_gizmos/gizmo_colors/skeleton");
	const int32_t KUSUDAMA_MAX_CONES = 30;
	bool has_gizmo(Node3D *p_spatial) override;
	String get_gizmo_name() const override;
	void redraw(EditorNode3DGizmo *p_gizmo) override;
	int get_priority() const override {
		return -2;
	}
	EWBIK3DGizmoPlugin() {
		// Enable vertex colors for the materials below as the gizmo color depends on the light color.
		create_material("lines_primary", bone_color, false, true, true);
		create_material("lines_secondary", Color(0, 0.63529413938522, 0.90980392694473), false, true, true);
		create_material("lines_tertiary", Color(0.93725490570068, 0.19215686619282, 0.22352941334248), true, true, true);

		// Need a textured2d handle for yellow dot, blue dot and turqouise dot and be icons.
		create_handle_material("handles");
		create_handle_material("handles_billboard", true);
	}
	void create_gizmo_mesh_handles(BoneId current_bone_idx, BoneId parent_idx, Ref<IKBone3D> ik_bone, EditorNode3DGizmo *p_gizmo, Color current_bone_color, Skeleton3D *ewbik_skeleton);
};

class EditorPluginEWBIK : public EditorPlugin {
	GDCLASS(EditorPluginEWBIK, EditorPlugin);

public:
	EditorPluginEWBIK() {
		Ref<EWBIK3DGizmoPlugin> ewbik_gizmo_plugin;
		ewbik_gizmo_plugin.instantiate();
		Node3DEditor::get_singleton()->add_gizmo_plugin(ewbik_gizmo_plugin);
	}
};

#endif // EWBIK_SKELETON_3D_GIZMO_PLUGIN_H
