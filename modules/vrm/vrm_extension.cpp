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
#include "modules/vrm/vrm_secondary.h"
#include "modules/vrm/vrm_toplevel.h"
#include "scene/resources/texture.h"

void VRMExtension::skeleton_rename(Ref<GLTFState> gstate, Node *p_base_scene, Skeleton3D *p_skeleton, Ref<BoneMap> p_bone_map) {
	HashMap<StringName, int> original_bone_names_to_indices;
	HashMap<int, StringName> original_indices_to_bone_names;
	HashMap<int, StringName> original_indices_to_new_bone_names;

	// Rename bones to their humanoid equivalents.
	for (int i = 0; i < p_skeleton->get_bone_count(); ++i) {
		StringName bn = p_bone_map->find_profile_bone_name(p_skeleton->get_bone_name(i));
		original_bone_names_to_indices[p_skeleton->get_bone_name(i)] = i;
		original_indices_to_bone_names[i] = p_skeleton->get_bone_name(i);
		original_indices_to_new_bone_names[i] = bn;
		if (bn != StringName()) {
			p_skeleton->set_bone_name(i, bn);
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

	for (int i = 0; i < gnodes.size(); ++i) {
		Ref<GLTFNode> gnode = gnodes[i];
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
				for (int i = 0; i < skin->get_bind_count(); ++i) {
					String bind_bone_name = skin->get_bind_name(i);
					StringName bone_name_from_skel = p_bone_map->find_profile_bone_name(bind_bone_name);
					if (bone_name_from_skel != StringName()) {
						skin->set_bind_name(i, bone_name_from_skel);
					}
				}
			}
		}
	}

	// Rename bones in all nodes by calling the _notify_skeleton_bones_renamed method.
	nodes = p_base_scene->find_children("*");
	while (!nodes.is_empty()) {
		Node *nd = cast_to<Node>(nodes.pop_back());
		if (nd) {
			nd->call("_notify_skeleton_bones_renamed", p_base_scene, p_skeleton, p_bone_map);
		}
	}
}

TypedArray<Basis> VRMExtension::skeleton_rotate(Node *p_base_scene, Skeleton3D *src_skeleton, Ref<BoneMap> p_bone_map) {
	Ref<SkeletonProfile> profile = p_bone_map->get_profile();
	Skeleton3D *prof_skeleton = memnew(Skeleton3D);

	for (int i = 0; i < profile->get_bone_size(); ++i) {
		prof_skeleton->add_bone(profile->get_bone_name(i));
		prof_skeleton->set_bone_rest(i, profile->get_reference_pose(i));
	}

	for (int i = 0; i < profile->get_bone_size(); ++i) {
		int parent = profile->find_bone(profile->get_bone_parent(i));
		if (parent >= 0) {
			prof_skeleton->set_bone_parent(i, parent);
		}
	}

	TypedArray<Transform3D> old_skeleton_rest;
	TypedArray<Transform3D> old_skeleton_global_rest;

	for (int i = 0; i < src_skeleton->get_bone_count(); ++i) {
		old_skeleton_rest.push_back(src_skeleton->get_bone_rest(i));
		old_skeleton_global_rest.push_back(src_skeleton->get_bone_global_rest(i));
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
				for (int i = 0; i < skin->get_bind_count(); ++i) {
					StringName bn = skin->get_bind_name(i);
					int bone_idx = src_skeleton->find_bone(bn);

					if (bone_idx >= 0) {
						Transform3D new_rest = src_skeleton->get_bone_global_rest(bone_idx);
						skin->set_bind_pose(i, new_rest.inverse());
					}
				}
			}
		}
	}

	for (int i = 0; i < src_skeleton->get_bone_count(); ++i) {
		Transform3D fixed_rest = src_skeleton->get_bone_rest(i);
		src_skeleton->set_bone_pose_position(i, fixed_rest.origin);
		src_skeleton->set_bone_pose_rotation(i, fixed_rest.basis.get_rotation_quaternion());
		src_skeleton->set_bone_pose_scale(i, fixed_rest.basis.get_scale());
	}
}

VRMExtension::VRMExtension() {
	vrm_constants_class.instantiate();
	vrm_meta_class.instantiate();
	vrm_collidergroup.instantiate();
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
	bool is_valid = false;
	int outline_width_mode = 0;
	if (mtoon_dict_float_properties.has("_OutlineWidthMode")) {
		outline_width_mode = mtoon_dict_float_properties["_OutlineWidthMode"];
	}
	int blend_mode = 0;
	if (mtoon_dict_float_properties.has("_BlendMode")) {
		blend_mode = mtoon_dict_float_properties.get("_BlendMode", &is_valid);
		;
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
	for (int32_t i = 0; i < float_properties_keys.size(); i++) {
		String param_name = float_properties_keys[i];
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
	for (int32_t i = 0; i < vector_properties_types.size(); i++) {
		String param_name = vector_properties_types[i];
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

	for (int i = 0; i < materials.size(); ++i) {
		Ref<Material> oldmat = materials[i];
		Dictionary vrm_mat = vrm_extension["materialProperties"].get(i);
		bool is_valid = false;
		int delta_render_queue = vrm_mat.get("renderQueue", &is_valid);
		if (!is_valid) {
			delta_render_queue = 3000;
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
	for (int i = 0; i < materials.size(); ++i) {
		Ref<Material> oldmat = materials[i];

		if (oldmat->is_class("ShaderMaterial")) {
			print_line("Material " + itos(i) + ": " + oldmat->get_name() + " already is shader.");
			continue;
		}

		Ref<Material> newmat = process_khr_material(oldmat, gstate->get_json()["materials"].get(i));
		Dictionary vrm_mat_props = vrm_extension["materialProperties"].get(i);
		newmat = _process_vrm_material(newmat, images, vrm_mat_props);
		spatial_to_shader_mat[oldmat] = newmat;
		spatial_to_shader_mat[newmat] = newmat;

		int target_render_priority = 0;
		bool is_valid = false;
		int delta_render_queue = vrm_mat_props.get("renderQueue", &is_valid);
		if (!is_valid) {
			delta_render_queue = 3000;
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
			int blend_mode = int(vrm_mat_props["floatProperties"].get("_BlendMode", &is_valid));
			if (!is_valid) {
				blend_mode = 0;
			}
			if (blend_mode == int(RenderMode::Transparent) || blend_mode == int(RenderMode::TransparentWithZWrite)) {
				newmat->set_render_priority(target_render_priority);
			}
		}

		materials[i] = newmat;
		String oldpath = oldmat->get_path();

		if (oldpath.is_empty()) {
			continue;
		}
		newmat->set_path(oldpath, true);
		ResourceSaver::save(newmat, oldpath);
	}

	gstate->set_materials(materials);

	TypedArray<GLTFMesh> meshes = gstate->get_meshes();

	for (int i = 0; i < meshes.size(); ++i) {
		Ref<GLTFMesh> gltfmesh = meshes[i];
		Ref<ImporterMesh> mesh = gltfmesh->get_mesh();
		mesh->set_blend_shape_mode(Mesh::BLEND_SHAPE_MODE_NORMALIZED);

		for (int surf_idx = 0; surf_idx < mesh->get_surface_count(); ++surf_idx) {
			Ref<Material> surfmat = mesh->get_surface_material(surf_idx);

			if (spatial_to_shader_mat.has(surfmat)) {
				mesh->set_surface_material(surf_idx, spatial_to_shader_mat[surfmat]);
			} else {
				print_error("Mesh " + itos(i) + " material " + itos(surf_idx) + " name " + surfmat->get_name() + " has no replacement material.");
			}
		}
	}
}

Node *VRMExtension::_get_skel_godot_node(Ref<GLTFState> gstate, Array nodes, Array skeletons, int skel_id) {
	for (int i = 0; i < nodes.size(); ++i) {
		Ref<GLTFNode> gltf_node = nodes[i];
		if (gltf_node->get_skeleton() == skel_id) {
			return gstate->get_scene_node(i);
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
	for (int i = 0; i < gstate->get_animation_players_count(0); ++i) {
		AnimationPlayer *node = gstate->get_animation_player(i);
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

	for (int i = 0; i < meshes.size(); ++i) {
		Ref<GLTFMesh> gltfmesh = meshes[i];
		for (int j = 0; j < gltfmesh->get_mesh()->get_surface_count(); ++j) {
			Array i_j;
			i_j.push_back(i);
			i_j.push_back(j);
			material_name_to_mesh_and_surface_idx[gltfmesh->get_mesh()->get_surface_material(j)->get_name()] = i_j;
		}
	}

	for (int i = 0; i < nodes.size(); ++i) {
		Ref<GLTFNode> gltfnode = nodes[i];
		if (gltfnode.is_null()) {
			continue;
		}
		int mesh_idx = gltfnode->get_mesh();

		if (mesh_idx != -1) {
			ImporterMeshInstance3D *scenenode = cast_to<ImporterMeshInstance3D>(gstate->get_scene_node(i));
			mesh_idx_to_meshinstance[mesh_idx] = scenenode;
		}
	}
	Array blend_shape_groups_array = blend_shape_groups;
	for (int i = 0; i < blend_shape_groups_array.size(); ++i) {
		Dictionary shape = blend_shape_groups_array[i];

		Ref<Animation> anim;
		anim.instantiate();

		Array material_values = shape["materialValues"];
		for (int i = 0; i < material_values.size(); ++i) {
			Dictionary matbind = material_values[i];
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
		for (int i = 0; i < binds.size(); i++) {
			Dictionary bind = binds[i];

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
	for (int i = 0; i < mesh_annotations.size(); ++i) {
		Dictionary mesh_annotation = mesh_annotations[i];
		bool is_valid = false;
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

void VRMExtension::parse_secondary_node(Node3D *secondary_node, Dictionary vrm_extension, Ref<GLTFState> gstate, Array pose_diffs, bool is_vrm_0) {
	Array nodes = gstate->get_nodes();
	Array skeletons = gstate->get_skeletons();

	Vector3 offset_flip = is_vrm_0 ? Vector3(-1, 1, 1) : Vector3(1, 1, 1);

	Dictionary vrm_extension_secondary_animation = vrm_extension["secondaryAnimation"];
	Array vrm_extension_collider_groups = vrm_extension_secondary_animation["colliderGroups"];
	Array collider_groups;
	for (int i = 0; i < vrm_extension_collider_groups.size(); ++i) {
		Dictionary cgroup = vrm_extension_collider_groups[i];
		Ref<GLTFNode> gltfnode = nodes[int(cgroup["node"])];

		Ref<VRMColliderGroup> collider_group;
		collider_group.instantiate();
		collider_group->get_sphere_colliders().clear(); // HACK HACK HACK

		Basis pose_diff;
		if (gltfnode->get_skeleton() == -1) {
			Node *found_node = gstate->get_scene_node(int(cgroup["node"]));
			collider_group->set_skeleton_or_node(secondary_node->get_path_to(found_node));
			collider_group->set_bone("");
			collider_group->set_name(found_node->get_name());
		} else {
			Skeleton3D *skeleton = cast_to<Skeleton3D>(_get_skel_godot_node(gstate, nodes, skeletons, gltfnode->get_skeleton()));
			collider_group->set_skeleton_or_node(secondary_node->get_path_to(skeleton));
			Ref<GLTFNode> bone_gltf_node = cast_to<GLTFNode>(nodes[int(cgroup["node"])]);
			String bone_name = bone_gltf_node->get_name();
			collider_group->set_bone(bone_name);
			collider_group->set_name(collider_group->get_bone());
			String collider_bone_name = collider_group->get_bone();
			BoneId collider_bone_id = skeleton->find_bone(collider_bone_name);
			if (collider_bone_id != -1) {
				pose_diff = pose_diffs[collider_bone_id];
			}
		}
		Array colliders = cgroup["colliders"];
		for (int i = 0; i < colliders.size(); ++i) {
			Dictionary collider_info = colliders[i];
			Dictionary default_offset_obj;
			default_offset_obj["x"] = 0.0;
			default_offset_obj["y"] = 0.0;
			default_offset_obj["z"] = 0.0;
			Dictionary offset_obj = collider_info.get("offset", default_offset_obj);
			Vector3 local_pos = pose_diff.xform(offset_flip) * Vector3(offset_obj["x"], offset_obj["y"], offset_obj["z"]);
			float radius = collider_info.get("radius", 0.0);
			collider_group->get_sphere_colliders().push_back(Plane(local_pos, radius));
		}
		collider_groups.push_back(collider_group);
	}

	Array vrm_extension_bone_groups = vrm_extension["secondaryAnimation"].get("boneGroups");
	Array spring_bones;
	for (int i = 0; i < vrm_extension_bone_groups.size(); ++i) {
		Dictionary sbone = vrm_extension_bone_groups[i];
		bool is_valid = false;
		Array array = sbone.get("bones", &is_valid);
		if (!is_valid || (is_valid && array.size() == 0)) {
			continue;
		}
		int first_bone_node = sbone["bones"].get(0);
		Ref<GLTFNode> gltfnode = nodes[int(first_bone_node)];
		Skeleton3D *skeleton = cast_to<Skeleton3D>(_get_skel_godot_node(gstate, nodes, skeletons, gltfnode->get_skeleton()));

		Ref<VRMSpringBone> spring_bone;
		spring_bone.instantiate();
		spring_bone->set_skeleton(secondary_node->get_path_to(skeleton));
		spring_bone->set_comment(sbone.get("comment", ""));
		spring_bone->set_stiffness_force(float(sbone.get("stiffiness", 1.0)));
		spring_bone->set_gravity_power(float(sbone.get("gravityPower", 0.0)));
		Dictionary default_gravity_dir;
		default_gravity_dir["x"] = 0.0;
		default_gravity_dir["y"] = -1.0;
		default_gravity_dir["z"] = 0.0;
		Dictionary gravity_dir = sbone.get("gravityDir", default_gravity_dir);
		spring_bone->set_gravity_dir(Vector3(gravity_dir["x"], gravity_dir["y"], gravity_dir["z"]));
		spring_bone->set_drag_force(float(sbone.get("dragForce", 0.4)));
		spring_bone->set_hit_radius(float(sbone.get("hitRadius", 0.02)));

		if (!spring_bone->get_comment().is_empty()) {
			spring_bone->set_name(spring_bone->get_comment().split("\n")[0]);
		} else {
			String tmpname;
			Array bone_array = sbone.get("bones", Array());
			if (bone_array.size() > 1) {
				tmpname += vformat(" + %s roots", bone_array.size() - 1);
			}
			Ref<GLTFNode> gltfnode = nodes[int(bone_array[0])];
			tmpname = gltfnode->get_name() + tmpname;
			spring_bone->set_name(tmpname);
		}

		spring_bone->get_collider_groups().clear(); // HACK HACK HACK

		Array colliderGroups = sbone.get("colliderGroups", Array());
		for (int i = 0; i < colliderGroups.size(); ++i) {
			int cgroup_idx = colliderGroups[i];
			spring_bone->get_collider_groups().push_back(collider_groups[int(cgroup_idx)]);
		}

		spring_bone->get_root_bones().clear(); // HACK HACK HACK
		Array bones = sbone["bones"];
		for (int i = 0; i < bones.size(); ++i) {
			int bone_node = bones[i];
			Ref<GLTFNode> gltf_node = nodes[int(bone_node)];
			String bone_name = gltf_node->get_name();
			if (skeleton->find_bone(bone_name) == -1) {
				// Note that we make an assumption that a given SpringBone object is
				// only part of a single Skeleton*. This error might print if a given
				// SpringBone references bones from multiple Skeleton's.
				print_error("Failed to find node " + itos(bone_node) + " in skel " + String(skeleton->get_path()));
			} else {
				spring_bone->get_root_bones().push_back(bone_name);
			}
		}

		// Center commonly points outside of the glTF Skeleton, such as the root node.
		spring_bone->set_center_node(secondary_node->get_path_to(secondary_node));
		spring_bone->set_center_bone(String());
		int center_node_idx = sbone.get("center", -1);
		if (center_node_idx != -1) {
			Ref<GLTFNode> center_gltfnode = nodes[int(center_node_idx)];
			String bone_name = center_gltfnode->get_name();
			if (center_gltfnode->get_skeleton() == gltfnode->get_skeleton() && skeleton->find_bone(bone_name) != -1) {
				spring_bone->set_center_bone(bone_name);
				spring_bone->set_center_node(NodePath());
			} else {
				spring_bone->set_center_bone(String());
				spring_bone->set_center_node(secondary_node->get_path_to(gstate->get_scene_node(int(center_node_idx))));
				if (spring_bone->get_center_node().is_empty()) {
					print_error("Failed to find center scene node " + itos(center_node_idx));
					spring_bone->set_center_node(secondary_node->get_path_to(secondary_node)); // Fallback
				}
			}
		}

		spring_bones.push_back(spring_bone);
	}

	secondary_node->set("spring_bones", spring_bones);
	secondary_node->set("collider_groups", collider_groups);
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
	for (int i = 0; i < children.size(); ++i) {
		const Variant &child_node = children[i];
		if (!new_joints_set.has(child_node)) {
			add_joints_recursive(new_joints_set, gltf_nodes, static_cast<int>(child_node));
		}
	}
}

void VRMExtension::add_joint_set_as_skin(Dictionary obj, Dictionary new_joints_set) {
	Array new_joints;
	Array joints_set_array = new_joints_set.keys();
	for (int i = 0; i < joints_set_array.size(); ++i) {
		const int32_t node = joints_set_array[i];
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
	bool is_valid = false;
	Dictionary vrm_extension = obj.get("extensions", Dictionary()).get("VRM", &is_valid);
	if (!is_valid) {
		vrm_extension = Dictionary();
	}
	if (!vrm_extension.has("humanoid")) {
		return false;
	}
	Dictionary new_joints_set = Dictionary().duplicate();

	Dictionary secondaryAnimation = vrm_extension.get("secondaryAnimation", Dictionary());
	Array boneGroups = secondaryAnimation.get("boneGroups", Array());
	for (int i = 0; i < boneGroups.size(); ++i) {
		Dictionary bone_group = boneGroups[i];
		Array bones = bone_group["bones"];
		for (int j = 0; j < bones.size(); ++j) {
			int bone = bones[j];
			add_joints_recursive(new_joints_set, obj["nodes"], bone, true);
		}
	}

	Array colliderGroups = secondaryAnimation.get("colliderGroups", Array());
	for (int i = 0; i < colliderGroups.size(); ++i) {
		Dictionary collider_group = colliderGroups[i];
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
	for (int i = 0; i < humanBones.size(); ++i) {
		Dictionary human_bone = humanBones[i];
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
	for (int i = 0; i < human_bones.size(); ++i) {
		Dictionary human_bone = human_bones[i];
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
	Array gltfnodes = gstate->get_nodes();

	Ref<BoneMap> human_bones_map;
	human_bones_map.instantiate();
	human_bones_map->set_profile(Ref<SkeletonProfile>(memnew(SkeletonProfileHumanoid)));

	Ref<VRMConstants> vrmconst_inst = memnew(VRMConstants(is_vrm_0)); // vrm 0.0
	Array human_bone_keys = human_bone_to_idx.keys();

	for (int i = 0; i < human_bone_keys.size(); ++i) {
		const String &human_bone_name = human_bone_keys[i];
		Ref<GLTFNode> gltf_node = gltfnodes[human_bone_to_idx[human_bone_name]];
		human_bones_map->set_skeleton_bone_name(vrmconst_inst->get_vrm_to_human_bone()[human_bone_name], gltf_node->get_name());
	}

	skeleton_rename(gstate, root_node, skeleton, human_bones_map);

	int hips_bone_idx = skeleton->find_bone("Hips");
	if (hips_bone_idx != -1) {
		skeleton->set_motion_scale(Math::abs(skeleton->get_bone_global_rest(hips_bone_idx).origin.y));
		if (skeleton->get_motion_scale() < 0.0001) {
			skeleton->set_motion_scale(1.0);
		}
	}

	TypedArray<Basis> pose_diffs = skeleton_rotate(root_node, skeleton, human_bones_map);
	apply_rotation(root_node, skeleton);

	skeleton->set_name("GeneralSkeleton");
	skeleton->set_unique_name_in_owner(true);

	_update_materials(vrm_extension, gstate);

	AnimationPlayer *animplayer = memnew(AnimationPlayer);
	animplayer->set_name("anim");
	root_node->add_child(animplayer);
	animplayer->set_owner(root_node);
	create_animation_player(animplayer, vrm_extension, gstate, human_bone_to_idx, pose_diffs);

	VRMTopLevel *vrm_top_level = memnew(VRMTopLevel);
	vrm_top_level->set_name("VRMTopLevel");
	root_node->add_child(vrm_top_level);
	vrm_top_level->set_owner(root_node);
	NodePath animation_path = String(vrm_top_level->get_path_to(root_node)) + "/" + root_node->get_path_to(animplayer);
	vrm_top_level->set_vrm_animplayer(animation_path);
	NodePath skeleton_path = String(vrm_top_level->get_path_to(root_node)) + "/" + root_node->get_path_to(skeleton);
	vrm_top_level->set_vrm_skeleton(skeleton_path);

	Ref<VRMMeta> vrm_meta = _create_meta(root_node, vrm_extension, gstate, human_bones_map, human_bone_to_idx, pose_diffs);
	NodePath humanoid_skeleton_path = root_node->get_path_to(skeleton);
	vrm_meta->set_humanoid_skeleton_path(humanoid_skeleton_path);
	vrm_top_level->set_vrm_meta(vrm_meta);

	Array collider_groups = vrm_extension["secondaryAnimation"].get("colliderGroups");
	int32_t collider_groups_size = collider_groups.size();
	Array bone_groups = vrm_extension["secondaryAnimation"].get("boneGroups");
	int32_t bone_group_size = bone_groups.size();
	vrm_top_level->set_unique_name_in_owner(true);
	if (vrm_extension.has("secondaryAnimation") &&
			(collider_groups_size > 0 ||
					bone_group_size > 0)) {
		Node3D *secondary_node = cast_to<Node3D>(root_node->get_node(NodePath("secondary")));
		NodePath secondary_path = String(vrm_top_level->get_path_to(root_node)) + "/" + root_node->get_path_to(secondary_node);
		vrm_top_level->set_vrm_secondary(secondary_path);
		parse_secondary_node(secondary_node, vrm_extension, gstate, pose_diffs, is_vrm_0);
	}
	return OK;
}
