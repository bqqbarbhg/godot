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
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/animation.h"
#include "scene/resources/animation_library.h"
#include "scene/resources/bone_map.h"
#include "scene/resources/importer_mesh.h"
#include "scene/resources/material.h"

#include "vrm_constants.h"
#include "vrm_meta.h"
#include "vrm_toplevel.h"

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

	Basis ROTATE_180_BASIS = Basis(Vector3(-1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, -1));
	Transform3D ROTATE_180_TRANSFORM = Transform3D(ROTATE_180_BASIS, Vector3(0, 0, 0));

public:
	VRMExtension();
	~VRMExtension() {}
	void adjust_mesh_z_forward(Ref<ImporterMesh> p_mesh);
	void skeleton_rename(Ref<GLTFState> p_gltf_state, Node *p_base_scene, Skeleton3D *p_skeleton, Ref<BoneMap> p_bone_map);
	void rotate_scene_180_inner(Node3D *p_node, Dictionary p_mesh_set, Dictionary p_skin_set);
	void rotate_scene_180(Node3D *p_scene);
	TypedArray<Basis> skeleton_rotate(Node *p_base_scene, Skeleton3D *p_src_skeleton, Ref<BoneMap> p_bone_map);
	void apply_rotation(Node *p_base_scene, Skeleton3D *p_src_skeleton);
	Ref<Material> process_khr_material(Ref<StandardMaterial3D> p_original_materials, Dictionary p_gltf_material_properties);
	Dictionary vrm_get_texture_info(Array p_gltf_images, Dictionary p_vrm_material_properties, String p_texture_name);
	float vrm_get_float(Dictionary p_vrm_material_properties, String p_key, float p_default);
	Ref<Material> _process_vrm_material(Ref<Material> p_original_material, Array p_gltf_images, Dictionary p_vrm_material_properties);
	void _update_materials(Dictionary p_vrm_extension, Ref<GLTFState> p_gltf_state);
	Node *_get_skeleton_godot_node(Ref<GLTFState> p_gltf_state, Array p_nodes, Array p_skeletons, int p_skeleton_id);
	Ref<Resource> _create_metadata(Node *p_root_node, Dictionary p_vrm_extension, Ref<GLTFState> p_gltf_state, Ref<BoneMap> p_human_bones, Dictionary p_human_bone_to_index, TypedArray<Basis> p_pose_differences);
	AnimationPlayer *create_animation_player(AnimationPlayer *p_animation_player, Dictionary p_vrm_extension, Ref<GLTFState> p_gltf_state, Dictionary p_human_bone_to_index, TypedArray<Basis> p_pose_differences);
	void add_joints_recursive(Dictionary &p_new_joints_set, Array p_gltf_nodes, int p_bone, bool p_include_child_meshes = false);
	void add_joint_set_as_skin(Dictionary p_object, Dictionary p_new_joints_set);
	bool add_vrm_nodes_to_skin(Dictionary p_object);
	TypedArray<Basis> apply_retarget(Ref<GLTFState> p_gltf_state, Node *p_root_node, Skeleton3D *p_skeleton, Ref<BoneMap> p_bone_map);
	Error import_preflight(Ref<GLTFState> p_state, Vector<String> p_extensions) override;
	Error import_post(Ref<GLTFState> p_state, Node *p_node) override;
};

#endif // VRM_EXTENSION_H