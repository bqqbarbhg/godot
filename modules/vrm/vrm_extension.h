/**************************************************************************/
/*  vrm_extension.h                                                       */
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

#ifndef VRM_EXTENSION_H
#define VRM_EXTENSION_H

#include "core/crypto/crypto.h"
#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "core/math/basis.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"
#include "core/variant/typed_array.h"
#include "core/variant/variant.h"
#include "modules/gltf/extensions/gltf_document_extension.h"
#include "modules/gltf/gltf_defines.h"
#include "modules/gltf/gltf_document.h"
#include "modules/gltf/structures/gltf_node.h"
#include "modules/vrm/vrm_constants.h"
#include "modules/vrm/vrm_secondary.h"
#include "modules/vrm/vrm_toplevel.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/animation.h"
#include "scene/resources/animation_library.h"
#include "scene/resources/bone_map.h"
#include "scene/resources/importer_mesh.h"
#include "scene/resources/material.h"

#include "vrm_collidergroup.h"
#include "vrm_meta.h"
#include "vrm_springbone.h"

class VRMExtension : public GLTFDocumentExtension {
	GDCLASS(VRMExtension, GLTFDocumentExtension);

public:
	enum class DebugMode {
		None = 0,
		Normal = 1,
		LitShadeRate = 2,
	};

	enum class OutlineColorMode {
		FixedColor = 0,
		MixedLight3Ding = 1,
	};

	enum class OutlineWidthMode {
		None = 0,
		WorldCoordinates = 1,
		ScreenCoordinates = 2,
	};

	enum class RenderMode {
		Opaque = 0,
		Cutout = 1,
		Transparent = 2,
		TransparentWithZWrite = 3,
	};

	enum class CullMode {
		Off = 0,
		Front = 1,
		Back = 2,
	};

	enum class FirstPersonFlag {
		Auto, // Create a headless model
		Both, // Default layer
		ThirdPersonOnly,
		FirstPersonOnly,
	};

	class SkelBone {
	public:
		Ref<Skeleton3D> skel;
		String bone_name;
	};

private:
	HashMap<String, FirstPersonFlag> FirstPersonParser;

	Ref<VRMConstants> vrm_constants_class;
	Ref<VRMMeta> vrm_meta_class;
	Ref<VRMColliderGroup> vrm_collidergroup;

	VRMSecondary *vrm_secondary = nullptr;

	Basis ROTATE_180_BASIS = Basis(Vector3(-1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, -1));
	Transform3D ROTATE_180_TRANSFORM = Transform3D(ROTATE_180_BASIS, Vector3(0, 0, 0));

public:
	VRMExtension();
	~VRMExtension() {}
	void adjust_mesh_zforward(Ref<ImporterMesh> mesh);
	void skeleton_rename(Ref<GLTFState> gstate, Node *p_base_scene, Skeleton3D *p_skeleton, Ref<BoneMap> p_bone_map);
	void rotate_scene_180_inner(Node3D *p_node, Dictionary mesh_set, Dictionary skin_set);
	void xtmp(Node3D *p_node, HashMap<Ref<Mesh>, bool> &mesh_set, HashMap<Ref<Skin>, bool> &skin_set);
	void rotate_scene_180(Node3D *p_scene);
	TypedArray<Basis> skeleton_rotate(Node *p_base_scene, Skeleton3D *src_skeleton, Ref<BoneMap> p_bone_map);
	void apply_rotation(Node *p_base_scene, Skeleton3D *src_skeleton);
	Ref<Material> process_khr_material(Ref<StandardMaterial3D> orig_mat, Dictionary gltf_mat_props);
	Dictionary vrm_get_texture_info(Array gltf_images, Dictionary vrm_mat_props, String unity_tex_name);
	float vrm_get_float(Dictionary vrm_mat_props, String key, float def);
	Ref<Material> _process_vrm_material(Ref<Material> orig_mat, Array gltf_images, Dictionary vrm_mat_props);
	void _update_materials(Dictionary vrm_extension, Ref<GLTFState> gstate);
	Node *_get_skel_godot_node(Ref<GLTFState> gstate, Array nodes, Array skeletons, int skel_id);
	Ref<Resource> _create_meta(Node *root_node, Dictionary vrm_extension, Ref<GLTFState> gstate, Ref<BoneMap> humanBones, Dictionary human_bone_to_idx, TypedArray<Basis> pose_diffs);
	AnimationPlayer *create_animation_player(AnimationPlayer *animplayer, Dictionary vrm_extension, Ref<GLTFState> gstate, Dictionary human_bone_to_idx, TypedArray<Basis> pose_diffs);
	void parse_secondary_node(Node3D *secondary_node, Dictionary vrm_extension, Ref<GLTFState> gstate, Array pose_diffs, bool is_vrm_0);
	void add_joints_recursive(Dictionary &new_joints_set, Array gltf_nodes, int bone, bool include_child_meshes = false);
	void add_joint_set_as_skin(Dictionary obj, Dictionary new_joints_set);
	bool add_vrm_nodes_to_skin(Dictionary obj);
	TypedArray<Basis> apply_retarget(Ref<GLTFState> gstate, Node *root_node, Skeleton3D *skeleton, Ref<BoneMap> bone_map);
	Error import_preflight(Ref<GLTFState> p_state, Vector<String> p_extensions) override;
	Error import_post(Ref<GLTFState> p_state, Node *p_node) override;
	Node3D *generate_scene_node(Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Node *p_scene_parent) override {
		if (p_gltf_node->get_mesh() != -1) {
			return nullptr;
		}
		if (p_gltf_node->get_name() == "secondary") {
			VRMSecondary *new_secondary = memnew(VRMSecondary);
			new_secondary->set_transform(p_gltf_node->get_xform());
			new_secondary->set_name(p_gltf_node->get_name());
			return new_secondary;
		}
	}
};

#endif // VRM_EXTENSION_H