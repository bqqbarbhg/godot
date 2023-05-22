/**************************************************************************/
/*  vrm_extension.cpp                                                     */
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

#include "vrm_extension.h"
#include "core/error/error_list.h"
#include "core/error/error_macros.h"
#include "core/object/object.h"
#include "core/string/node_path.h"
#include "core/variant/dictionary.h"
#include "modules/gltf/gltf_defines.h"
#include "modules/gltf/gltf_document.h"
#include "modules/vrm/vrm_meta.h"
#include "modules/vrm/vrm_toplevel.h"
#include "scene/resources/texture.h"

void VRMExtension::adjust_mesh_zforward(Ref<ImporterMesh> mesh) {
	int surf_count = mesh->get_surface_count();
	Array surf_data_by_mesh;
	Vector<String> blendshapes;

	for (int bsidx = 0; bsidx < mesh->get_blend_shape_count(); ++bsidx) {
		blendshapes.append(mesh->get_blend_shape_name(bsidx));
	}

	for (int surf_idx = 0; surf_idx < surf_count; ++surf_idx) {
		int prim = mesh->get_surface_primitive_type(surf_idx);
		int fmt_compress_flags = mesh->get_surface_format(surf_idx);
		Array arr = mesh->get_surface_arrays(surf_idx);
		String surface_name = mesh->get_surface_name(surf_idx);
		int bscount = mesh->get_blend_shape_count();
		Array bsarr;

		for (int bsidx = 0; bsidx < bscount; ++bsidx) {
			bsarr.append(mesh->get_surface_blend_shape_arrays(surf_idx, bsidx));
		}

		Dictionary lods; // mesh.surface_get_lods(surf_idx) // get_lods(mesh, surf_idx)
		Ref<Material> mat = mesh->get_surface_material(surf_idx);
		PackedVector3Array vertarr = arr[ArrayMesh::ARRAY_VERTEX];

		for (int vertex_i = 0; vertex_i < vertarr.size(); ++vertex_i) {
			vertarr.set(vertex_i, ROTATE_180_TRANSFORM.xform(vertarr[vertex_i]));
		}
		arr[ArrayMesh::ARRAY_VERTEX] = vertarr;

		if (arr[ArrayMesh::ARRAY_NORMAL].get_type() == Variant::PACKED_VECTOR3_ARRAY) {
			PackedVector3Array normarr = arr[ArrayMesh::ARRAY_NORMAL];
			for (int normal_i = 0; normal_i < vertarr.size(); ++normal_i) {
				normarr.set(normal_i, ROTATE_180_TRANSFORM.basis.xform(normarr[normal_i]));
			}
			arr[ArrayMesh::ARRAY_NORMAL] = normarr;
		}

		if (arr[ArrayMesh::ARRAY_TANGENT].get_type() == Variant::PACKED_FLOAT32_ARRAY) {
			PackedFloat32Array tangarr = arr[ArrayMesh::ARRAY_TANGENT];
			for (int tangent_i = 0; tangent_i < vertarr.size(); ++tangent_i) {
				tangarr.set(tangent_i * 4, -tangarr[tangent_i * 4]);
				tangarr.set(tangent_i * 4 + 2, -tangarr[tangent_i * 4 + 2]);
			}
			arr[ArrayMesh::ARRAY_TANGENT] = tangarr;
		}

		for (int bsidx = 0; bsidx < bsarr.size(); ++bsidx) {
			vertarr = bsarr[bsidx].get(ArrayMesh::ARRAY_VERTEX);
			for (int vertex_i = 0; vertex_i < vertarr.size(); ++vertex_i) {
				vertarr.set(vertex_i, ROTATE_180_TRANSFORM.xform(vertarr[vertex_i]));
			}
			bsarr[bsidx].set(ArrayMesh::ARRAY_VERTEX, vertarr);

			if (bsarr[bsidx].get(ArrayMesh::ARRAY_NORMAL).get_type() == Variant::PACKED_VECTOR3_ARRAY) {
				PackedVector3Array normarr = bsarr[bsidx].get(ArrayMesh::ARRAY_NORMAL);
				for (int vertex_i = 0; vertex_i < vertarr.size(); ++vertex_i) {
					normarr.set(vertex_i, ROTATE_180_TRANSFORM.basis.xform(normarr[vertex_i]));
				}
				bsarr[bsidx].set(ArrayMesh::ARRAY_NORMAL, normarr);
			}

			if (bsarr[bsidx].get(ArrayMesh::ARRAY_TANGENT).get_type() == Variant::PACKED_FLOAT32_ARRAY) {
				PackedFloat32Array tangarr = bsarr[bsidx].get(ArrayMesh::ARRAY_TANGENT);
				for (int tangent_i = 0; tangent_i < vertarr.size(); ++tangent_i) {
					tangarr.set(tangent_i * 4, -tangarr[tangent_i * 4]);
					tangarr.set(tangent_i * 4 + 2, -tangarr[tangent_i * 4 + 2]);
				}
				bsarr[bsidx].set(ArrayMesh::ARRAY_TANGENT, tangarr);
			}
			Array array_mesh = bsarr[bsidx];
			array_mesh.resize(ArrayMesh::ARRAY_MAX);
			bsarr[bsidx] = array_mesh;
		}
		Dictionary surf_data_dict;
		surf_data_dict["prim"] = prim;
		surf_data_dict["arr"] = arr;
		surf_data_dict["bsarr"] = bsarr;
		surf_data_dict["lods"] = lods;
		surf_data_dict["fmt_compress_flags"] = fmt_compress_flags;
		surf_data_dict["name"] = surface_name;
		surf_data_dict["mat"] = mat;
		surf_data_by_mesh.push_back(surf_data_dict);
	}

	mesh->clear();

	for (Variant blend_name : blendshapes) {
		mesh->add_blend_shape(blend_name);
	}

	for (int surf_idx = 0; surf_idx < surf_count; ++surf_idx) {
		int prim = surf_data_by_mesh[surf_idx].get("prim");
		Array arr = surf_data_by_mesh[surf_idx].get("arr");
		Array bsarr = surf_data_by_mesh[surf_idx].get("bsarr");
		Dictionary lods = surf_data_by_mesh[surf_idx].get("lods");
		int fmt_compress_flags = surf_data_by_mesh[surf_idx].get("fmt_compress_flags");
		String surface_name = surf_data_by_mesh[surf_idx].get("name");
		Ref<Material> mat = surf_data_by_mesh[surf_idx].get("mat");

		mesh->add_surface(Mesh::PrimitiveType(prim), arr, bsarr, lods, mat, surface_name, fmt_compress_flags);
	}
}

void VRMExtension::skeleton_rename(Ref<GLTFState> gstate, Node *p_base_scene, Skeleton3D *p_skeleton, Ref<BoneMap> p_bone_map) {
	HashMap<StringName, int> original_bone_names_to_indices;
	HashMap<int, StringName> original_indices_to_bone_names;
	HashMap<int, StringName> original_indices_to_new_bone_names;

	// Rename bones to their humanoid equivalents.
	for (int bone_i = 0; bone_i < p_skeleton->get_bone_count(); ++bone_i) {
		StringName bn = p_bone_map->find_profile_bone_name(p_skeleton->get_bone_name(bone_i));
		original_bone_names_to_indices[p_skeleton->get_bone_name(bone_i)] = bone_i;
		original_indices_to_bone_names[bone_i] = p_skeleton->get_bone_name(bone_i);
		original_indices_to_new_bone_names[bone_i] = bn;
		if (bn != StringName()) {
			p_skeleton->set_bone_name(bone_i, bn);
		}
	}

	TypedArray<GLTFNode> gnodes = gstate->get_nodes();
	String root_bone_name = "Root";
	if (p_skeleton->find_bone(root_bone_name) == -1) {
		p_skeleton->add_bone(root_bone_name);
		int new_root_bone_id = p_skeleton->find_bone(root_bone_name);
		for (int root_bone_id : p_skeleton->get_parentless_bones()) {
			if (root_bone_id != new_root_bone_id) {
				p_skeleton->set_bone_parent(root_bone_id, new_root_bone_id);
			}
		}
	}

	for (int gltf_node_i = 0; gltf_node_i < gnodes.size(); ++gltf_node_i) {
		Ref<GLTFNode> gnode = gnodes[gltf_node_i];
		StringName bn = p_bone_map->find_profile_bone_name(gnode->get_name());
		if (bn != StringName()) {
			gnode->set_name(bn);
		}
	}

	TypedArray<Node> nodes = p_base_scene->find_children("*", "ImporterMeshInstance3D");
	while (!nodes.is_empty()) {
		ImporterMeshInstance3D *mi = Object::cast_to<ImporterMeshInstance3D>(nodes.pop_back());
		Ref<Skin> skin = mi->get_skin();
		if (skin.is_valid()) {
			Node *node = mi->get_node(mi->get_skeleton_path());
			if (node && Object::cast_to<Skeleton3D>(node) && node == p_skeleton) {
				for (int bind_i = 0; bind_i < skin->get_bind_count(); ++bind_i) {
					String bind_bone_name = skin->get_bind_name(bind_i);
					StringName bone_name_from_skel = p_bone_map->find_profile_bone_name(bind_bone_name);
					if (bone_name_from_skel != StringName()) {
						skin->set_bind_name(bind_i, bone_name_from_skel);
					}
				}
			}
		}
	}

	// Rename bones in all nodes by calling the _notify_skeleton_bones_renamed method.
	nodes = p_base_scene->find_children("*");
	while (!nodes.is_empty()) {
		Node *nd = cast_to<Node>(nodes.pop_back());
		if (nd && nd->has_method("_notify_skeleton_bones_renamed")) {
			nd->call("_notify_skeleton_bones_renamed", p_base_scene, p_skeleton, p_bone_map);
		}
	}

	p_skeleton->set_name("GeneralSkeleton");
	p_skeleton->set_unique_name_in_owner(true);
}

void VRMExtension::rotate_scene_180_inner(Node3D *p_node, Dictionary mesh_set, Dictionary skin_set) {
	Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(p_node);
	if (skeleton) {
		for (int bone_idx = 0; bone_idx < skeleton->get_bone_count(); ++bone_idx) {
			Transform3D rest = ROTATE_180_TRANSFORM * skeleton->get_bone_rest(bone_idx) * ROTATE_180_TRANSFORM;
			skeleton->set_bone_rest(bone_idx, rest);
			skeleton->set_bone_pose_rotation(bone_idx, Quaternion(ROTATE_180_BASIS) * skeleton->get_bone_pose_rotation(bone_idx) * Quaternion(ROTATE_180_BASIS));
			skeleton->set_bone_pose_scale(bone_idx, Vector3(1, 1, 1));
			skeleton->set_bone_pose_position(bone_idx, rest.origin);
		}
	}

	p_node->set_transform(ROTATE_180_TRANSFORM * p_node->get_transform() * ROTATE_180_TRANSFORM);

	ImporterMeshInstance3D *importer_mesh_instance = Object::cast_to<ImporterMeshInstance3D>(p_node);
	if (importer_mesh_instance) {
		mesh_set[importer_mesh_instance->get_mesh()] = true;
		skin_set[importer_mesh_instance->get_skin()] = true;
	}

	for (int child_i = 0; child_i < p_node->get_child_count(); ++child_i) {
		Node3D *child = Object::cast_to<Node3D>(p_node->get_child(child_i));
		if (child) {
			rotate_scene_180_inner(child, mesh_set, skin_set);
		}
	}
}

void VRMExtension::rotate_scene_180(Node3D *p_scene) {
	Dictionary mesh_set;
	Dictionary skin_set;

	rotate_scene_180_inner(p_scene, mesh_set, skin_set);
	Array mesh_set_keys = mesh_set.keys();
	for (int32_t mesh_i = 0; mesh_i < mesh_set_keys.size(); mesh_i++) {
		Ref<ImporterMesh> mesh = mesh_set_keys[mesh_i];
		ERR_CONTINUE(mesh.is_null());
		adjust_mesh_zforward(mesh);
	}
	for (int32_t skin_set_i = 0; skin_set_i < skin_set.size(); skin_set_i++) {
		Ref<Skin> skin = skin_set.keys()[skin_set_i];
		ERR_CONTINUE(skin.is_null());
		for (int32_t bind_i = 0; bind_i < skin->get_bind_count(); bind_i++) {
			skin->set_bind_pose(bind_i, skin->get_bind_pose(bind_i) * ROTATE_180_TRANSFORM);
		}
	}
}

TypedArray<Basis> VRMExtension::skeleton_rotate(Node *p_base_scene, Skeleton3D *src_skeleton, Ref<BoneMap> p_bone_map) {
	Ref<SkeletonProfile> profile = p_bone_map->get_profile();
	Skeleton3D *prof_skeleton = memnew(Skeleton3D);

	for (int bone_i = 0; bone_i < profile->get_bone_size(); ++bone_i) {
		prof_skeleton->add_bone(profile->get_bone_name(bone_i));
		prof_skeleton->set_bone_rest(bone_i, profile->get_reference_pose(bone_i));
	}

	for (int bone_i = 0; bone_i < profile->get_bone_size(); ++bone_i) {
		int parent = profile->find_bone(profile->get_bone_parent(bone_i));
		if (parent >= 0) {
			prof_skeleton->set_bone_parent(bone_i, parent);
		}
	}

	TypedArray<Transform3D> old_skeleton_rest;
	TypedArray<Transform3D> old_skeleton_global_rest;

	for (int bone_i = 0; bone_i < src_skeleton->get_bone_count(); ++bone_i) {
		old_skeleton_rest.push_back(src_skeleton->get_bone_rest(bone_i));
		old_skeleton_global_rest.push_back(src_skeleton->get_bone_global_rest(bone_i));
	}

	TypedArray<Basis> diffs;
	diffs.resize(src_skeleton->get_bone_count());
	diffs.fill(Basis());

	Vector<int32_t> bones_to_process = src_skeleton->get_parentless_bones();
	int bpidx = 0;

	while (bpidx < bones_to_process.size()) {
		int src_idx = bones_to_process[bpidx];
		bpidx++;

		Vector<int32_t> src_children = src_skeleton->get_bone_children(src_idx);
		for (int bone_idx : src_children) {
			bones_to_process.push_back(bone_idx);
		}

		Basis tgt_rot;
		StringName src_bone_name = src_skeleton->get_bone_name(src_idx);

		if (src_bone_name != StringName()) {
			Basis src_pg;
			int src_parent_idx = src_skeleton->get_bone_parent(src_idx);

			if (src_parent_idx >= 0) {
				src_pg = src_skeleton->get_bone_global_rest(src_parent_idx).basis;
			}

			int prof_idx = profile->find_bone(src_bone_name);

			if (prof_idx >= 0) {
				tgt_rot = src_pg.inverse() * prof_skeleton->get_bone_global_rest(prof_idx).basis;
			}
		}

		if (src_skeleton->get_bone_parent(src_idx) >= 0) {
			diffs[src_idx] = (tgt_rot.inverse() * Basis(diffs[src_skeleton->get_bone_parent(src_idx)]) * src_skeleton->get_bone_rest(src_idx).basis);
		} else {
			diffs[src_idx] = tgt_rot.inverse() * src_skeleton->get_bone_rest(src_idx).basis;
		}

		Basis diff;

		if (src_skeleton->get_bone_parent(src_idx) >= 0) {
			diff = diffs[src_skeleton->get_bone_parent(src_idx)];
		}

		// Get the source bone rest origin
		Vector3 src_bone_rest_origin = src_skeleton->get_bone_rest(src_idx).origin;

		// Transform the source bone rest origin using the difference
		Vector3 transformed_src_bone_rest_origin = diff.xform(src_bone_rest_origin);

		// Create a new Transform3D with the target rotation and transformed origin
		Transform3D new_transform = Transform3D(tgt_rot, transformed_src_bone_rest_origin);

		// Set the new bone rest transform for the source skeleton
		src_skeleton->set_bone_rest(src_idx, new_transform);
	}

	prof_skeleton->queue_free();
	return diffs;
}

void VRMExtension::apply_rotation(Node *p_base_scene, Skeleton3D *src_skeleton) {
	TypedArray<Node> nodes = p_base_scene->find_children("*", "ImporterMeshInstance3D");

	while (!nodes.is_empty()) {
		Node *this_node = Object::cast_to<Node>(nodes.back());
		nodes.pop_back();

		if (ImporterMeshInstance3D *mi = Object::cast_to<ImporterMeshInstance3D>(this_node)) {
			Ref<Skin> skin = mi->get_skin();
			Node *node = mi->get_node_or_null(mi->get_skeleton_path());

			if (skin.is_valid() && node && Object::cast_to<Skeleton3D>(node) == src_skeleton) {
				for (int bind_i = 0; bind_i < skin->get_bind_count(); ++bind_i) {
					StringName bn = skin->get_bind_name(bind_i);
					int bone_idx = src_skeleton->find_bone(bn);

					if (bone_idx >= 0) {
						Transform3D new_rest = src_skeleton->get_bone_global_rest(bone_idx);
						skin->set_bind_pose(bind_i, new_rest.inverse());
					}
				}
			}
		}
	}

	for (int bone_i = 0; bone_i < src_skeleton->get_bone_count(); ++bone_i) {
		Transform3D fixed_rest = src_skeleton->get_bone_rest(bone_i);
		src_skeleton->set_bone_pose_position(bone_i, fixed_rest.origin);
		src_skeleton->set_bone_pose_rotation(bone_i, fixed_rest.basis.get_rotation_quaternion());
		src_skeleton->set_bone_pose_scale(bone_i, fixed_rest.basis.get_scale());
	}
}

VRMExtension::VRMExtension() {
	vrm_constants_class.instantiate();
	vrm_meta_class.instantiate();
	FirstPersonParser.insert("Auto", FirstPersonFlag::Auto);
	FirstPersonParser.insert("Both", FirstPersonFlag::Both);
	FirstPersonParser.insert("FirstPersonOnly", FirstPersonFlag::FirstPersonOnly);
	FirstPersonParser.insert("ThirdPersonOnly", FirstPersonFlag::ThirdPersonOnly);
}

Ref<Material> VRMExtension::process_khr_material(Ref<StandardMaterial3D> orig_mat, Dictionary gltf_mat_props) {
	// VRM spec requires support for the KHR_materials_unlit extension.
	if (gltf_mat_props.has("extensions")) {
		Dictionary extensions = gltf_mat_props["extensions"];
		if (extensions.has("KHR_materials_unlit")) {
			orig_mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
		}
	}
	return orig_mat;
}

Dictionary VRMExtension::vrm_get_texture_info(Array gltf_images, Dictionary vrm_mat_props, String tex_name) {
	Dictionary texture_info;
	texture_info["tex"] = Ref<Texture2D>();
	texture_info["offset"] = Vector3(0.0, 0.0, 0.0);
	texture_info["scale"] = Vector3(1.0, 1.0, 1.0);

	Dictionary texture_properties_dict = vrm_mat_props["textureProperties"];
	if (texture_properties_dict.has(tex_name)) {
		int mainTexId = texture_properties_dict[tex_name];
		Ref<Texture2D> mainTexImage = gltf_images[mainTexId];
		texture_info["tex"] = mainTexImage;
	}
	Dictionary vector_properties_dict = vrm_mat_props["vectorProperties"];
	if (vector_properties_dict.has(tex_name)) {
		Array offsetScale = vector_properties_dict[tex_name];
		texture_info["offset"] = Vector3(offsetScale[0], offsetScale[1], 0.0);
		texture_info["scale"] = Vector3(offsetScale[2], offsetScale[3], 1.0);
	}

	return texture_info;
}

float VRMExtension::vrm_get_float(Dictionary vrm_mat_props, String key, float def) {
	Dictionary float_properties_dict = vrm_mat_props["floatProperties"];
	if (float_properties_dict.has(key)) {
		return float_properties_dict[key];
	}
	return def;
}

Ref<Material> VRMExtension::_process_vrm_material(Ref<Material> orig_mat, Array gltf_images, Dictionary vrm_mat_props) {
	String vrm_shader_name = vrm_mat_props["shader"];

	if (vrm_shader_name == "VRM_USE_GLTFSHADER") {
		return orig_mat;
	}

	if (vrm_shader_name == "Standard" || vrm_shader_name == "UniGLTF/UniUnlit") {
		print_error("Unsupported legacy VRM shader " + vrm_shader_name + " on material " + orig_mat->get_name());
		return orig_mat;
	}

	Dictionary maintex_info = vrm_get_texture_info(gltf_images, vrm_mat_props, "_MainTex");

	if (
			vrm_shader_name == "VRM/UnlitTransparentZWrite" ||
			vrm_shader_name == "VRM/UnlitTransparent" ||
			vrm_shader_name == "VRM/UnlitTexture" ||
			vrm_shader_name == "VRM/UnlitCutout") {
		if (maintex_info["tex"] != Variant()) {
			orig_mat->set("albedo_texture", maintex_info["tex"]);
			orig_mat->set("uv1_offset", maintex_info["offset"]);
			orig_mat->set("uv1_scale", maintex_info["scale"]);
		}
		orig_mat->set("shading_mode", BaseMaterial3D::SHADING_MODE_UNSHADED);

		if (vrm_shader_name == "VRM/UnlitTransparentZWrite") {
			orig_mat->set("depth_draw_mode", StandardMaterial3D::DEPTH_DRAW_ALWAYS);
		}

		orig_mat->set("no_depth_test", false);

		if (vrm_shader_name == "VRM/UnlitTransparent" || vrm_shader_name == "VRM/UnlitTransparentZWrite") {
			orig_mat->set("transparency", BaseMaterial3D::TRANSPARENCY_ALPHA);
			orig_mat->set("blend_mode", StandardMaterial3D::BLEND_MODE_MIX);
		}

		if (vrm_shader_name == "VRM/UnlitCutout") {
			orig_mat->set("transparency", BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR);
			orig_mat->set("alpha_scissor_threshold", vrm_get_float(vrm_mat_props, "_Cutoff", 0.5));
		}

		return orig_mat;
	}

	if (vrm_shader_name != "VRM/MToon") {
		print_error("Unknown VRM shader " + vrm_shader_name + " on material " + orig_mat->get_name());
		return orig_mat;
	}
	Dictionary mtoon_dict_float_properties = vrm_mat_props["floatProperties"];
	int outline_width_mode = 0;
	if (mtoon_dict_float_properties.has("_OutlineWidthMode")) {
		outline_width_mode = mtoon_dict_float_properties["_OutlineWidthMode"];
	}
	int blend_mode = 0;
	if (mtoon_dict_float_properties.has("_BlendMode")) {
		blend_mode = mtoon_dict_float_properties["_BlendMode"];
	}
	int cull_mode = 2;
	if (mtoon_dict_float_properties.has("_CullMode")) {
		cull_mode = mtoon_dict_float_properties["_CullMode"];
	}
	int outl_cull_mode = 1;
	if (mtoon_dict_float_properties.has("_OutlineCullMode")) {
		outl_cull_mode = mtoon_dict_float_properties["_OutlineCullMode"];
	}

	if (cull_mode == int(CullMode::Front) || (outl_cull_mode != int(CullMode::Front) && outline_width_mode != int(OutlineWidthMode::None))) {
		print_error("VRM Material " + orig_mat->get_name() + " has unsupported front-face culling mode: " + itos(cull_mode) + "/" + itos(outl_cull_mode));
	}

	String mtoon_shader_base_path = "res://addons/Godot-MToon-Shader/mtoon";

	String godot_outline_shader_name;
	if (outline_width_mode != int(OutlineWidthMode::None)) {
		godot_outline_shader_name = mtoon_shader_base_path + "_outline";
	}

	String godot_shader_name = mtoon_shader_base_path;

	if (blend_mode == int(RenderMode::Opaque) || blend_mode == int(RenderMode::Cutout)) {
		if (cull_mode == int(CullMode::Off)) {
			godot_shader_name = mtoon_shader_base_path + "_cull_off";
		}
	} else if (blend_mode == int(RenderMode::Transparent)) {
		godot_shader_name = mtoon_shader_base_path + "_trans";
		if (cull_mode == int(CullMode::Off)) {
			godot_shader_name = mtoon_shader_base_path + "_trans_cull_off";
		}
	} else if (blend_mode == int(RenderMode::TransparentWithZWrite)) {
		godot_shader_name = mtoon_shader_base_path + "_trans_zwrite";
		if (cull_mode == int(CullMode::Off)) {
			godot_shader_name = mtoon_shader_base_path + "_trans_zwrite_cull_off";
		}
	}

	Ref<Shader> godot_shader = ResourceLoader::load(godot_shader_name + ".gdshader");
	Ref<Shader> godot_shader_outline = nullptr;

	if (godot_outline_shader_name.is_valid_filename()) {
		godot_shader_outline = ResourceLoader::load(godot_outline_shader_name + ".gdshader");
	}

	Ref<ShaderMaterial> new_mat = memnew(ShaderMaterial);
	new_mat->set_name(orig_mat->get_name());
	new_mat->set_shader(godot_shader);

	if (godot_shader_outline.is_null()) {
		new_mat->set_next_pass(Ref<Material>());
	} else {
		if (new_mat->get_next_pass().is_null()) {
			new_mat->set_next_pass(Ref<Material>());
		} else {
			new_mat->set_next_pass(new_mat->get_next_pass()->duplicate());
		}
	}

	Ref<ShaderMaterial> outline_mat = new_mat->get_next_pass();

	Vector4 texture_repeat = Vector4(maintex_info["scale"].get("x"), maintex_info["scale"].get("y"), maintex_info["offset"].get("x"), maintex_info["offset"].get("y"));
	new_mat->set_shader_parameter("_MainTex_ST", texture_repeat);

	if (outline_mat != nullptr) {
		outline_mat->set_shader_parameter("_MainTex_ST", texture_repeat);
	}

	for (String param_name : { "_MainTex", "_ShadeTexture", "_BumpMap", "_RimTexture", "_SphereAdd", "_EmissionMap", "_OutlineWidthTexture", "_UvAnimMaskTexture" }) {
		Dictionary tex_info = vrm_get_texture_info(gltf_images, vrm_mat_props, param_name);
		if (tex_info.has("tex")) {
			new_mat->set_shader_parameter(param_name, tex_info["tex"]);
			if (outline_mat != nullptr) {
				outline_mat->set_shader_parameter(param_name, tex_info["tex"]);
			}
		}
	}
	Dictionary float_properties = vrm_mat_props["floatProperties"];
	Array float_properties_keys = float_properties.keys();
	for (int32_t float_property_i = 0; float_property_i < float_properties_keys.size(); float_property_i++) {
		String param_name = float_properties_keys[float_property_i];
		new_mat->set_shader_parameter(param_name, float_properties[param_name]);
		if (outline_mat != nullptr) {
			outline_mat->set_shader_parameter(param_name, float_properties[param_name]);
		}
	}
	Array vector_properties_types;
	vector_properties_types.push_back("_Color");
	vector_properties_types.push_back("_ShadeColor");
	vector_properties_types.push_back("_RimColor");
	vector_properties_types.push_back("_EmissionColor");
	vector_properties_types.push_back("_OutlineColor");

	Dictionary vector_properties = vrm_mat_props["vectorProperties"];
	Array vector_properties_keys = vector_properties.keys();
	for (int32_t vector_property_i = 0; vector_property_i < vector_properties_types.size(); vector_property_i++) {
		String param_name = vector_properties_types[vector_property_i];
		if (vector_properties.has(param_name)) {
			Array param_val = vrm_mat_props["vectorProperties"].get(param_name);
			Vector4 color_param = Vector4(param_val[0], param_val[1], param_val[2], param_val[3]);
			new_mat->set_shader_parameter(param_name, color_param);
			if (outline_mat != nullptr) {
				outline_mat->set_shader_parameter(param_name, color_param);
			}
		}
	}

	if (blend_mode == int(RenderMode::Cutout)) {
		new_mat->set_shader_parameter("_AlphaCutoutEnable", 1.0);
		if (outline_mat != nullptr) {
			outline_mat->set_shader_parameter("_AlphaCutoutEnable", 1.0);
		}
	}

	return new_mat;
}

void VRMExtension::_update_materials(Dictionary vrm_extension, Ref<GLTFState> gstate) {
	TypedArray<Texture2D> images = gstate->get_images();
	TypedArray<Material> materials = gstate->get_materials();
	Dictionary spatial_to_shader_mat;

	// Render priority setup
	Array render_queue_to_priority;
	Array negative_render_queue_to_priority;
	Dictionary uniq_render_queues;
	negative_render_queue_to_priority.push_back(0);
	render_queue_to_priority.push_back(0);
	uniq_render_queues[0] = true;

	for (int material_i = 0; material_i < materials.size(); ++material_i) {
		Ref<Material> oldmat = materials[material_i];
		Dictionary vrm_mat = vrm_extension["materialProperties"].get(material_i);
		int delta_render_queue = 3000;
		if (vrm_mat.has("renderQueue")) {
			delta_render_queue = vrm_mat["renderQueue"];
		}
		delta_render_queue = delta_render_queue - 3000;

		if (!uniq_render_queues.has(delta_render_queue)) {
			uniq_render_queues[delta_render_queue] = true;
			if (delta_render_queue < 0) {
				negative_render_queue_to_priority.push_back(-delta_render_queue);
			} else {
				render_queue_to_priority.push_back(delta_render_queue);
			}
		}
	}

	negative_render_queue_to_priority.sort();
	render_queue_to_priority.sort();

	// Material conversions
	for (int material_i = 0; material_i < materials.size(); ++material_i) {
		Ref<Material> oldmat = materials[material_i];

		if (oldmat->is_class("ShaderMaterial")) {
			print_line("Material " + itos(material_i) + ": " + oldmat->get_name() + " already is shader.");
			continue;
		}

		Ref<Material> newmat = process_khr_material(oldmat, gstate->get_json()["materials"].get(material_i));
		Dictionary vrm_mat_props = vrm_extension["materialProperties"].get(material_i);
		newmat = _process_vrm_material(newmat, images, vrm_mat_props);
		spatial_to_shader_mat[oldmat] = newmat;
		spatial_to_shader_mat[newmat] = newmat;

		int target_render_priority = 0;
		int delta_render_queue = 3000;
		if (vrm_mat_props.has("renderQueue")) {
			delta_render_queue = vrm_mat_props["renderQueue"];
		}
		delta_render_queue = delta_render_queue - 3000;

		if (delta_render_queue >= 0) {
			target_render_priority = render_queue_to_priority.find(delta_render_queue);
			if (target_render_priority > 100) {
				target_render_priority = 100;
			}
		} else {
			target_render_priority = -negative_render_queue_to_priority.find(-delta_render_queue);
			if (target_render_priority < -100) {
				target_render_priority = -100;
			}
		}

		// Render_priority only makes sense for transparent materials.
		if (newmat->get_class() == "StandardMaterial3D") {
			if (int(newmat->get("transparency")) > 0) {
				newmat->set_render_priority(target_render_priority);
			}
		} else {
			Dictionary vrm_mat = vrm_extension["materialProperties"];
			int blend_mode = 0;
			if (vrm_mat.has("_BlendMode")) {
				blend_mode = vrm_mat["_BlendMode"];
			}
			if (blend_mode == int(RenderMode::Transparent) || blend_mode == int(RenderMode::TransparentWithZWrite)) {
				newmat->set_render_priority(target_render_priority);
			}
		}

		materials[material_i] = newmat;
		String oldpath = oldmat->get_path();

		if (oldpath.is_empty()) {
			continue;
		}
		newmat->set_path(oldpath, true);
		ResourceSaver::save(newmat, oldpath);
	}

	gstate->set_materials(materials);

	TypedArray<GLTFMesh> meshes = gstate->get_meshes();

	for (int mesh_i = 0; mesh_i < meshes.size(); ++mesh_i) {
		Ref<GLTFMesh> gltfmesh = meshes[mesh_i];
		Ref<ImporterMesh> mesh = gltfmesh->get_mesh();
		mesh->set_blend_shape_mode(Mesh::BLEND_SHAPE_MODE_NORMALIZED);

		for (int surf_idx = 0; surf_idx < mesh->get_surface_count(); ++surf_idx) {
			Ref<Material> surfmat = mesh->get_surface_material(surf_idx);

			if (spatial_to_shader_mat.has(surfmat)) {
				mesh->set_surface_material(surf_idx, spatial_to_shader_mat[surfmat]);
			} else {
				print_error("Mesh " + itos(mesh_i) + " material " + itos(surf_idx) + " name " + surfmat->get_name() + " has no replacement material.");
			}
		}
	}
}

Node *VRMExtension::_get_skel_godot_node(Ref<GLTFState> gstate, Array nodes, Array skeletons, int skel_id) {
	for (int node_i = 0; node_i < nodes.size(); ++node_i) {
		Ref<GLTFNode> gltf_node = nodes[node_i];
		if (gltf_node->get_skeleton() == skel_id) {
			return gstate->get_scene_node(node_i);
		}
	}

	return nullptr;
}

Ref<Resource> VRMExtension::_create_meta(Node *root_node, Dictionary vrm_extension, Ref<GLTFState> gstate, Ref<BoneMap> humanBones, Dictionary human_bone_to_idx, TypedArray<Basis> pose_diffs) {
	TypedArray<GLTFNode> nodes = gstate->get_nodes();

	Dictionary firstperson = vrm_extension.get("firstPerson", Variant());
	Vector3 eyeOffset;

	if (!firstperson.is_empty()) {
		Dictionary fpboneoffsetxyz = firstperson["firstPersonBoneOffset"];
		eyeOffset = Vector3(fpboneoffsetxyz["x"], fpboneoffsetxyz["y"], fpboneoffsetxyz["z"]);
		int32_t human_bone_idx = human_bone_to_idx["head"];
		if (human_bone_idx != -1) {
			Basis pose = pose_diffs[human_bone_idx];
			eyeOffset = pose.xform(eyeOffset);
		}
	}

	Ref<VRMMeta> vrm_meta;
	vrm_meta.instantiate();

	vrm_meta->set_name("CLICK TO SEE METADATA");

	vrm_meta->set_humanoid_bone_mapping(humanBones);

	if (vrm_extension.has("exporterVersion")) {
		vrm_meta->set_exporter_version(vrm_extension["exporterVersion"]);
	}

	if (vrm_extension.has("specVersion")) {
		vrm_meta->set_spec_version(vrm_extension["specVersion"]);
	}

	Dictionary vrm_extension_meta = vrm_extension["meta"];
	if (!vrm_extension_meta.is_empty()) {
		if (vrm_extension_meta.has("title")) {
			vrm_meta->set_title(vrm_extension_meta["title"]);
		}

		if (vrm_extension_meta.has("version")) {
			vrm_meta->set_version(vrm_extension_meta["version"]);
		}

		if (vrm_extension_meta.has("author")) {
			vrm_meta->set_author(vrm_extension_meta["author"]);
		}

		if (vrm_extension_meta.has("contactInformation")) {
			vrm_meta->set_contact_information(vrm_extension_meta["contactInformation"]);
		}

		if (vrm_extension_meta.has("reference")) {
			vrm_meta->set_reference_information(vrm_extension_meta["reference"]);
		}

		if (vrm_extension_meta.has("texture")) {
			int32_t tex = vrm_extension_meta["texture"];
			Ref<GLTFTexture> gltftex = gstate->get_textures()[tex];
			vrm_meta->set_texture(gstate->get_images()[gltftex->get_src_image()]);
		}
		if (vrm_extension_meta.has("allowedUserName")) {
			vrm_meta->set_allowed_user_name(vrm_extension_meta["allowedUserName"]);
		}

		if (vrm_extension_meta.has("violentUssageName")) {
			vrm_meta->set_violent_usage(vrm_extension_meta["violentUssageName"]); // Ussage(sic.) in VRM spec
		}

		if (vrm_extension_meta.has("sexualUssageName")) {
			vrm_meta->set_sexual_usage(vrm_extension_meta["sexualUssageName"]); // Ussage(sic.) in VRM spec
		}

		if (vrm_extension_meta.has("commercialUssageName")) {
			vrm_meta->set_commercial_usage(vrm_extension_meta["commercialUssageName"]); // Ussage(sic.) in VRM spec
		}

		if (vrm_extension_meta.has("otherPermissionUrl")) {
			vrm_meta->set_other_permission_url(vrm_extension_meta["otherPermissionUrl"]);
		}

		if (vrm_extension_meta.has("licenseName")) {
			vrm_meta->set_license_name(vrm_extension_meta["licenseName"]);
		}

		if (vrm_extension_meta.has("otherLicenseUrl")) {
			vrm_meta->set_other_license_url(vrm_extension_meta["otherLicenseUrl"]);
		}
	}

	return vrm_meta;
}

AnimationPlayer *VRMExtension::create_animation_player(AnimationPlayer *animplayer, Dictionary vrm_extension, Ref<GLTFState> gstate, Dictionary human_bone_to_idx, TypedArray<Basis> pose_diffs) {
	// Remove all glTF animation players for safety.
	// VRM does not support animation import in this way.
	for (int player_i = 0; player_i < gstate->get_animation_players_count(0); ++player_i) {
		AnimationPlayer *node = gstate->get_animation_player(player_i);
		node->get_parent()->remove_child(node);
	}

	Ref<AnimationLibrary> animation_library;
	animation_library.instantiate();

	Array meshes = gstate->get_meshes();
	Array nodes = gstate->get_nodes();
	Dictionary blend_shape_master = vrm_extension["blendShapeMaster"];
	Array blend_shape_groups = blend_shape_master["blendShapeGroups"];

	Dictionary mesh_idx_to_meshinstance;
	Dictionary material_name_to_mesh_and_surface_idx;

	for (int mesh_i = 0; mesh_i < meshes.size(); ++mesh_i) {
		Ref<GLTFMesh> gltfmesh = meshes[mesh_i];
		for (int j = 0; j < gltfmesh->get_mesh()->get_surface_count(); ++j) {
			Array i_j;
			i_j.push_back(mesh_i);
			i_j.push_back(j);
			material_name_to_mesh_and_surface_idx[gltfmesh->get_mesh()->get_surface_material(j)->get_name()] = i_j;
		}
	}

	for (int node_i = 0; node_i < nodes.size(); ++node_i) {
		Ref<GLTFNode> gltfnode = nodes[node_i];
		if (gltfnode.is_null()) {
			continue;
		}
		int mesh_idx = gltfnode->get_mesh();

		if (mesh_idx != -1) {
			ImporterMeshInstance3D *scenenode = cast_to<ImporterMeshInstance3D>(gstate->get_scene_node(node_i));
			mesh_idx_to_meshinstance[mesh_idx] = scenenode;
		}
	}
	Array blend_shape_groups_array = blend_shape_groups;
	for (int blend_shape_group_i = 0; blend_shape_group_i < blend_shape_groups_array.size(); ++blend_shape_group_i) {
		Dictionary shape = blend_shape_groups_array[blend_shape_group_i];

		Ref<Animation> anim;
		anim.instantiate();

		Array material_values = shape["materialValues"];
		for (int material_i = 0; material_i < material_values.size(); ++material_i) {
			Dictionary matbind = material_values[material_i];
			Array mesh_and_surface_idx = material_name_to_mesh_and_surface_idx[matbind["materialName"]];
			ImporterMeshInstance3D *node = cast_to<ImporterMeshInstance3D>(mesh_idx_to_meshinstance[mesh_and_surface_idx[0]]);
			int surface_idx = mesh_and_surface_idx[1];

			Ref<Material> mat = node->get_surface_material(surface_idx);
			String paramprop = "shader_uniform/" + String(matbind["parameterName"]);
			Variant origvalue = Variant();
			Array tv = matbind["targetValue"];
			Variant newvalue = tv[0];

			if (Object::cast_to<ShaderMaterial>(mat.ptr())) {
				Ref<ShaderMaterial> smat = mat;
				Variant param = smat->get_shader_parameter(matbind["parameterName"]);

				if (param.get_type() == Variant::COLOR) {
					origvalue = param;
					newvalue = Color(tv[0], tv[1], tv[2], tv[3]);
				} else if (matbind["parameterName"] == "_MainTex" || matbind["parameterName"] == "_MainTex_ST") {
					origvalue = param;
					newvalue = (matbind["parameterName"] == "_MainTex" ? Vector4(tv[2], tv[3], tv[0], tv[1]) : Vector4(tv[0], tv[1], tv[2], tv[3]));
				} else if (param.get_type() == Variant::FLOAT) {
					origvalue = param;
					newvalue = tv[0];
				} else {
					ERR_PRINT(String("Unknown type for parameter ") + String(matbind["parameterName"]) + " surface " + node->get_name() + "/" + itos(surface_idx));
				}
			}

			if (origvalue.get_type() != Variant::NIL) {
				int animtrack = anim->add_track(Animation::TYPE_VALUE);
				anim->track_set_path(animtrack, String(animplayer->get_parent()->get_path_to(node)) + ":mesh:surface_" + itos(surface_idx) + "/material:" + paramprop);
				anim->track_set_interpolation_type(animtrack, shape["isBinary"] ? Animation::INTERPOLATION_NEAREST : Animation::INTERPOLATION_LINEAR);
				anim->track_insert_key(animtrack, 0.0, origvalue);
				anim->track_insert_key(animtrack, 0.0, newvalue);
			}
		}

		Array binds = shape["binds"];
		for (int bind_i = 0; bind_i < binds.size(); bind_i++) {
			Dictionary bind = binds[bind_i];

			ImporterMeshInstance3D *node = cast_to<ImporterMeshInstance3D>(mesh_idx_to_meshinstance[int(bind["mesh"])]);
			Ref<ImporterMesh> nodeMesh = node->get_mesh();

			if (int(bind["index"]) < 0 || int(bind["index"]) >= nodeMesh->get_blend_shape_count()) {
				ERR_PRINT(vformat("Invalid blend shape index in bind %s for mesh %s", shape, node->get_name()));
				continue;
			}

			int animtrack = anim->add_track(Animation::TYPE_BLEND_SHAPE);
			anim->track_set_path(animtrack, String(animplayer->get_parent()->get_path_to(node)) + ":" + nodeMesh->get_blend_shape_name(int(bind["index"])));
			int interpolation = Animation::INTERPOLATION_LINEAR;

			if (shape.has("isBinary") && bool(shape["isBinary"])) {
				interpolation = Animation::INTERPOLATION_NEAREST;
			}

			anim->track_set_interpolation_type(animtrack, Animation::InterpolationType(interpolation));
			anim->track_insert_key(animtrack, 0.0, float(0.0));
			anim->track_insert_key(animtrack, 1.0, 0.99999 * float(bind["weight"]) / 100.0);
		}

		String animation_name = shape["presetName"] == "unknown" ? String(shape["name"]).to_upper() : String(shape["presetName"]).to_upper();
		animation_library->add_animation(animation_name, anim);
	}

	Dictionary firstperson = vrm_extension["firstPerson"];

	Ref<Animation> firstpersanim = memnew(Animation);
	animation_library->add_animation("FirstPerson", firstpersanim);

	Ref<Animation> thirdpersanim = memnew(Animation);
	animation_library->add_animation("ThirdPerson", thirdpersanim);

	Array skeletons = gstate->get_skeletons();
	int head_bone_idx = -1;
	if (firstperson.has("firstPersonBone")) {
		head_bone_idx = firstperson["firstPersonBone"];
	}
	if (head_bone_idx >= 0) {
		Ref<GLTFNode> headNode = nodes[head_bone_idx];
		NodePath skeletonPath = animplayer->get_parent()->get_path_to(_get_skel_godot_node(gstate, nodes, skeletons, headNode->get_skeleton()));
		String headBone = headNode->get_name();
		String headPath = String(skeletonPath) + ":" + headBone;
		int firstperstrack = firstpersanim->add_track(Animation::TYPE_SCALE_3D);
		firstpersanim->track_set_path(firstperstrack, headPath);
		firstpersanim->scale_track_insert_key(firstperstrack, 0.0, Vector3(0.00001, 0.00001, 0.00001));
		int thirdperstrack = thirdpersanim->add_track(Animation::TYPE_SCALE_3D);
		thirdpersanim->track_set_path(thirdperstrack, headPath);
		thirdpersanim->scale_track_insert_key(thirdperstrack, 0.0, Vector3(1, 1, 1));
	}
	Dictionary mesh_annotations;
	if (firstperson.has("meshAnnotations")) {
		mesh_annotations = firstperson["meshAnnotations"];
	}
	for (int annotation_i = 0; annotation_i < mesh_annotations.size(); ++annotation_i) {
		Dictionary mesh_annotation = mesh_annotations[annotation_i];
		int flag = int(FirstPersonParser[mesh_annotation["firstPersonFlag"]]);
		float first_person_visibility;
		float third_person_visibility;

		if (flag == int(FirstPersonFlag::ThirdPersonOnly)) {
			first_person_visibility = 0.0;
			third_person_visibility = 1.0;
		} else if (flag == int(FirstPersonFlag::FirstPersonOnly)) {
			first_person_visibility = 1.0;
			third_person_visibility = 0.0;
		} else {
			continue;
		}

		ImporterMeshInstance3D *node = cast_to<ImporterMeshInstance3D>(mesh_idx_to_meshinstance[int(mesh_annotation["mesh"])]);
		int firstperstrack = firstpersanim->add_track(Animation::TYPE_VALUE);
		firstpersanim->track_set_path(firstperstrack, String(animplayer->get_parent()->get_path_to(node)) + ":visible");
		firstpersanim->track_insert_key(firstperstrack, 0.0, first_person_visibility);

		int thirdperstrack = thirdpersanim->add_track(Animation::TYPE_VALUE);
		thirdpersanim->track_set_path(thirdperstrack, String(animplayer->get_parent()->get_path_to(node)) + ":visible");
		thirdpersanim->track_insert_key(thirdperstrack, 0.0, third_person_visibility);
	}
	String look_at_type_name;
	if (firstperson.has("lookAtTypeName")) {
		look_at_type_name = firstperson["lookAtTypeName"];
	}
	if (look_at_type_name == "Bone") {
		Dictionary horizout = firstperson["lookAtHorizontalOuter"];
		Dictionary horizin = firstperson["lookAtHorizontalInner"];
		Dictionary vertup = firstperson["lookAtVerticalUp"];
		Dictionary vertdown = firstperson["lookAtVerticalDown"];
		int lefteye = -1;
		if (human_bone_to_idx.has("leftEye")) {
			lefteye = human_bone_to_idx["leftEye"];
		}
		int righteye = -1;
		if (human_bone_to_idx.has("rightEye")) {
			righteye = human_bone_to_idx["rightEye"];
		}
		String leftEyePath;
		String rightEyePath;

		if (lefteye > 0) {
			Ref<GLTFNode> leftEyeNode = nodes[lefteye];
			Skeleton3D *skeleton = cast_to<Skeleton3D>(_get_skel_godot_node(gstate, nodes, skeletons, leftEyeNode->get_skeleton()));
			NodePath skeletonPath = animplayer->get_parent()->get_path_to(skeleton);
			leftEyePath = String(skeletonPath) + ":" + leftEyeNode->get_name();
		}

		if (righteye > 0) {
			Ref<GLTFNode> rightEyeNode = nodes[righteye];
			Skeleton3D *skeleton = cast_to<Skeleton3D>(_get_skel_godot_node(gstate, nodes, skeletons, rightEyeNode->get_skeleton()));
			NodePath skeletonPath = animplayer->get_parent()->get_path_to(skeleton);
			rightEyePath = String(skeletonPath) + ":" + rightEyeNode->get_name();
		}
		Ref<Animation> anim;
		if (!animplayer->has_animation("LOOKRIGHT")) {
			anim.instantiate();
			animation_library->add_animation("LOOKRIGHT", anim);
		} else {
			anim = animplayer->get_animation("LOOKRIGHT");
		}

		if (anim.is_valid() && lefteye > 0 && righteye > 0) {
			int animtrack = anim->add_track(Animation::TYPE_ROTATION_3D);
			anim->track_set_path(animtrack, leftEyePath);
			anim->track_set_interpolation_type(animtrack, Animation::INTERPOLATION_LINEAR);
			anim->rotation_track_insert_key(animtrack, 0.0, Quaternion());
			anim->rotation_track_insert_key(
					animtrack,
					float(horizin["xRange"]) / 90.0,
					(pose_diffs[lefteye] * Basis(Vector3(0, 1, 0), -float(horizin["yRange"]) * Math_PI / 180.0)));

			animtrack = anim->add_track(Animation::TYPE_ROTATION_3D);
			anim->track_set_path(animtrack, rightEyePath);
			anim->track_set_interpolation_type(animtrack, Animation::INTERPOLATION_LINEAR);
			anim->rotation_track_insert_key(animtrack, 0.0, Quaternion());
			anim->rotation_track_insert_key(
					animtrack,
					float(horizout["xRange"]) / 90.0,
					(pose_diffs[righteye] * Basis(Vector3(0, 1, 0), -float(horizout["yRange"]) * Math_PI / 180.0)));
		}

		if (!animplayer->has_animation("LOOKUP")) {
			anim.instantiate();
			animation_library->add_animation("LOOKUP", anim);
		} else {
			anim = animplayer->get_animation("LOOKUP");
		}

		if (anim.is_valid() && lefteye > 0 && righteye > 0) {
			int animtrack = anim->add_track(Animation::TYPE_ROTATION_3D);
			anim->track_set_path(animtrack, leftEyePath);
			anim->track_set_interpolation_type(animtrack, Animation::INTERPOLATION_LINEAR);
			anim->rotation_track_insert_key(animtrack, 0.0, Quaternion());
			anim->rotation_track_insert_key(
					animtrack,
					float(vertup["xRange"]) / 90.0,
					(pose_diffs[lefteye] * Basis(Vector3(1, 0, 0), float(vertup["yRange"]) * Math_PI / 180.0)));

			animtrack = anim->add_track(Animation::TYPE_ROTATION_3D);
			anim->track_set_path(animtrack, rightEyePath);
			anim->track_set_interpolation_type(animtrack, Animation::INTERPOLATION_LINEAR);
			anim->rotation_track_insert_key(animtrack, 0.0, Quaternion());
			anim->rotation_track_insert_key(
					animtrack,
					float(vertup["xRange"]) / 90.0,
					(pose_diffs[righteye] * Basis(Vector3(1, 0, 0), float(vertup["yRange"]) * Math_PI / 180.0)));
		}

		if (!animplayer->has_animation("LOOKDOWN")) {
			anim.instantiate();
			animation_library->add_animation("LOOKDOWN", anim);
		} else {
			anim = animplayer->get_animation("LOOKDOWN");
		}

		if (anim.is_valid() && lefteye > 0 && righteye > 0) {
			int animtrack = anim->add_track(Animation::TYPE_ROTATION_3D);
			anim->track_set_path(animtrack, leftEyePath);
			anim->track_set_interpolation_type(animtrack, Animation::INTERPOLATION_LINEAR);
			anim->rotation_track_insert_key(animtrack, 0.0, Quaternion());
			anim->rotation_track_insert_key(
					animtrack,
					float(vertdown["xRange"]) / 90.0,
					(pose_diffs[lefteye] * Basis(Vector3(1, 0, 0), -float(vertdown["yRange"]) * Math_PI / 180.0)));

			animtrack = anim->add_track(Animation::TYPE_ROTATION_3D);
			anim->track_set_path(animtrack, rightEyePath);
			anim->track_set_interpolation_type(animtrack, Animation::INTERPOLATION_LINEAR);
			anim->rotation_track_insert_key(animtrack, 0.0, Quaternion());
			anim->rotation_track_insert_key(
					animtrack,
					float(vertdown["xRange"]) / 90.0,
					(pose_diffs[righteye] * Basis(Vector3(1, 0, 0), -float(vertdown["yRange"]) * Math_PI / 180.0)));
		}
	}
	animplayer->add_animation_library("vrm", animation_library);
	return animplayer;
}


void VRMExtension::add_joints_recursive(Dictionary &new_joints_set, Array gltf_nodes, int bone, bool include_child_meshes) {
	if (bone < 0) {
		return;
	}
	Dictionary gltf_node = gltf_nodes[bone];
	int mesh_idx = gltf_node.get("mesh", -1);
	if (!include_child_meshes && mesh_idx != -1) {
		return;
	}
	new_joints_set[bone] = true;
	Array children = gltf_node.get("children", Array());
	for (int child_i = 0; child_i < children.size(); ++child_i) {
		const Variant &child_node = children[child_i];
		if (!new_joints_set.has(child_node)) {
			add_joints_recursive(new_joints_set, gltf_nodes, static_cast<int>(child_node));
		}
	}
}

void VRMExtension::add_joint_set_as_skin(Dictionary obj, Dictionary new_joints_set) {
	Array new_joints;
	Array joints_set_array = new_joints_set.keys();
	for (int array_i = 0; array_i < joints_set_array.size(); ++array_i) {
		const int32_t node = joints_set_array[array_i];
		new_joints.push_back(node);
	}
	new_joints.sort();

	Dictionary new_skin;
	new_skin["joints"] = new_joints;

	if (!obj.has("skins")) {
		obj["skins"] = Array();
	}

	Array skins = obj["skins"];
	skins.push_back(new_skin);
}

bool VRMExtension::add_vrm_nodes_to_skin(Dictionary obj) {
	Dictionary vrm_extension;
	if (obj.has("extensions") && Dictionary(obj["extensions"]).has("VRM")) {
		vrm_extension = Dictionary(obj["extensions"])["VRM"];
	}
	if (!vrm_extension.has("humanoid")) {
		return false;
	}
	Dictionary new_joints_set = Dictionary().duplicate();

	Dictionary secondaryAnimation = vrm_extension.get("secondaryAnimation", Dictionary());
	Array boneGroups = secondaryAnimation.get("boneGroups", Array());
	for (int group_i = 0; group_i < boneGroups.size(); ++group_i) {
		Dictionary bone_group = boneGroups[group_i];
		Array bones = bone_group["bones"];
		for (int j = 0; j < bones.size(); ++j) {
			int bone = bones[j];
			add_joints_recursive(new_joints_set, obj["nodes"], bone, true);
		}
	}

	Array colliderGroups = secondaryAnimation.get("colliderGroups", Array());
	for (int collider_group_i = 0; collider_group_i < colliderGroups.size(); ++collider_group_i) {
		Dictionary collider_group = colliderGroups[collider_group_i];
		int node = collider_group["node"];
		if (node >= 0) {
			new_joints_set[node] = true;
		}
	}

	Dictionary firstPerson = vrm_extension.get("firstPerson", Dictionary());
	int firstPersonBone = firstPerson.get("firstPersonBone", -1);
	if (firstPersonBone >= 0) {
		new_joints_set[firstPersonBone] = true;
	}
	Dictionary vrm_extension_humanoid = vrm_extension["humanoid"];
	Array humanBones = vrm_extension_humanoid["humanBones"];
	for (int bone_i = 0; bone_i < humanBones.size(); ++bone_i) {
		Dictionary human_bone = humanBones[bone_i];
		int node = human_bone["node"];
		add_joints_recursive(new_joints_set, obj["nodes"], node, false);
	}

	add_joint_set_as_skin(obj, new_joints_set);

	return true;
}

Error VRMExtension::import_preflight(Ref<GLTFState> p_state, Vector<String> p_extensions) {
	Dictionary gltf_json_parsed = p_state->get_json();
	if (!add_vrm_nodes_to_skin(gltf_json_parsed)) {
		ERR_PRINT("Failed to find required VRM keys in json");
		return ERR_INVALID_DATA;
	}
	return OK;
}

TypedArray<Basis> VRMExtension::apply_retarget(Ref<GLTFState> gstate, Node *root_node, Skeleton3D *skeleton, Ref<BoneMap> bone_map) {
	NodePath skeleton_path = root_node->get_path_to(skeleton);

	skeleton_rename(gstate, root_node, skeleton, bone_map);

	int hips_bone_idx = skeleton->find_bone("Hips");
	if (hips_bone_idx != -1) {
		skeleton->set_motion_scale(Math::abs(skeleton->get_bone_global_rest(hips_bone_idx).origin.y));
		if (skeleton->get_motion_scale() < 0.0001) {
			skeleton->set_motion_scale(1.0);
		}
	}

	TypedArray<Basis> poses = skeleton_rotate(root_node, skeleton, bone_map);
	apply_rotation(root_node, skeleton);

	return poses;
}

Error VRMExtension::import_post(Ref<GLTFState> gstate, Node *node) {
	Dictionary gltf_json = gstate->get_json();
	Dictionary gltf_vrm_extension = gltf_json["extensions"];
	Dictionary vrm_extension = gltf_vrm_extension["VRM"];

	bool is_vrm_0 = true;

	Node3D *root_node = Object::cast_to<Node3D>(node);

	Dictionary human_bone_to_idx;
	// Ignoring in ["humanoid"]: armStretch, legStretch, upperArmTwist,
	// lowerArmTwist, upperLegTwist, lowerLegTwist, feetSpacing,
	// and hasTranslationDoF
	Dictionary humanoid = vrm_extension["humanoid"];
	Array human_bones = humanoid["humanBones"];
	for (int bone_i = 0; bone_i < human_bones.size(); ++bone_i) {
		Dictionary human_bone = human_bones[bone_i];
		human_bone_to_idx[human_bone["bone"]] = static_cast<int>(human_bone["node"]);
		// Ignoring: useDefaultValues
		// Ignoring: min
		// Ignoring: max
		// Ignoring: center
		// Ingoring: axisLength
	}

	TypedArray<GLTFSkeleton> skeletons = gstate->get_skeletons();
	Ref<GLTFNode> hips_node = gstate->get_nodes()[human_bone_to_idx["hips"]];
	Skeleton3D *skeleton = cast_to<Skeleton3D>(_get_skel_godot_node(gstate, gstate->get_nodes(), skeletons, hips_node->get_skeleton()));
	if (!skeleton) {
		ERR_PRINT("Failed to find the skeleton.");
		return ERR_INVALID_DATA;
	}

	VRMTopLevel *vrm_top_level = memnew(VRMTopLevel);
	vrm_top_level->set_name("VRMTopLevel");
	root_node->add_child(vrm_top_level);
	vrm_top_level->set_owner(root_node);
	skeleton->reparent(vrm_top_level, true);

	Array gltfnodes = gstate->get_nodes();

	Ref<BoneMap> human_bones_map;
	human_bones_map.instantiate();
	human_bones_map->set_profile(Ref<SkeletonProfile>(memnew(SkeletonProfileHumanoid)));

	Ref<VRMConstants> vrmconst_inst = memnew(VRMConstants(is_vrm_0)); // vrm 0.0
	Array human_bone_keys = human_bone_to_idx.keys();

	for (int bone_key_i = 0; bone_key_i < human_bone_keys.size(); ++bone_key_i) {
		const String &human_bone_name = human_bone_keys[bone_key_i];
		Ref<GLTFNode> gltf_node = gltfnodes[human_bone_to_idx[human_bone_name]];
		human_bones_map->set_skeleton_bone_name(vrmconst_inst->get_vrm_to_human_bone()[human_bone_name], gltf_node->get_name());
	}

	if (is_vrm_0) {
		// VRM 0.0 has models facing backwards due to a spec error (flipped z instead of x)
		print_line("Pre-rotate");
		rotate_scene_180(root_node);
		print_line("Post-rotate");
	}

	bool do_retarget = true;

	TypedArray<Basis> pose_diffs;
	pose_diffs.resize(skeleton->get_bone_count());
	pose_diffs.fill(Basis());
	if (do_retarget) {
		pose_diffs = apply_retarget(gstate, root_node, skeleton, human_bones_map);
	}

	_update_materials(vrm_extension, gstate);

	AnimationPlayer *animplayer = memnew(AnimationPlayer);
	animplayer->set_name("anim");
	root_node->add_child(animplayer);
	animplayer->set_owner(root_node);
	create_animation_player(animplayer, vrm_extension, gstate, human_bone_to_idx, pose_diffs);

	NodePath animation_path = String(vrm_top_level->get_path_to(root_node)) + "/" + root_node->get_path_to(animplayer);
	vrm_top_level->set_vrm_animplayer(animation_path);
	NodePath skeleton_path = String(vrm_top_level->get_path_to(root_node)) + "/" + root_node->get_path_to(skeleton);
	vrm_top_level->set_vrm_skeleton(skeleton_path);

	Ref<VRMMeta> vrm_meta = _create_meta(root_node, vrm_extension, gstate, human_bones_map, human_bone_to_idx, pose_diffs);
	NodePath humanoid_skeleton_path = root_node->get_path_to(skeleton);
	vrm_meta->set_humanoid_skeleton_path(humanoid_skeleton_path);
	vrm_top_level->set_vrm_meta(vrm_meta);

	vrm_top_level->set_unique_name_in_owner(true);
	return OK;
}
 