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
#include "modules/gltf/extensions/gltf_document_extension.h"
#include "modules/gltf/gltf_document.h"
#include "modules/gltf/structures/gltf_node.h"
#include "scene/3d/importer_mesh_instance_3d.h"
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
	Ref<Resource> vrm_constants_class = ResourceLoader::load("res://vrm_constants.gd");
	Ref<Resource> vrm_meta_class = ResourceLoader::load("res://vrm_meta.gd");
	Ref<Resource> vrm_secondary = ResourceLoader::load("res://vrm_secondary.gd");
	Ref<Resource> vrm_collidergroup = ResourceLoader::load("res://vrm_collidergroup.gd");
	Ref<Resource> vrm_top_level = ResourceLoader::load("res://vrm_toplevel.gd");

	Ref<VRMMeta> vrm_meta;

	Basis ROTATE_180_BASIS = Basis(Vector3(-1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, -1));
	Transform3D ROTATE_180_TRANSFORM = Transform3D(ROTATE_180_BASIS, Vector3(0, 0, 0));

	void adjust_mesh_zforward(Ref<ImporterMesh> mesh) {
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
			String name = mesh->get_surface_name(surf_idx);
			int bscount = mesh->get_blend_shape_count();
			Array bsarr;

			for (int bsidx = 0; bsidx < bscount; ++bsidx) {
				bsarr.append(mesh->get_surface_blend_shape_arrays(surf_idx, bsidx));
			}

			Dictionary lods; // mesh.surface_get_lods(surf_idx) // get_lods(mesh, surf_idx)
			Ref<Material> mat = mesh->get_surface_material(surf_idx);
			PackedVector3Array vertarr = arr[ArrayMesh::ARRAY_VERTEX];
			Vector3 invert_vector(-1, 1, -1);

			for (int i = 0; i < vertarr.size(); ++i) {
				vertarr.set(i, invert_vector * vertarr[i]);
			}

			if (arr[ArrayMesh::ARRAY_NORMAL].get_type() == Variant::PACKED_VECTOR3_ARRAY) {
				PackedVector3Array normarr = arr[ArrayMesh::ARRAY_NORMAL];
				for (int i = 0; i < vertarr.size(); ++i) {
					normarr.set(i, invert_vector * normarr[i]);
				}
			}

			if (arr[ArrayMesh::ARRAY_TANGENT].get_type() == Variant::PACKED_FLOAT32_ARRAY) {
				PackedFloat32Array tangarr = arr[ArrayMesh::ARRAY_TANGENT];
				for (int i = 0; i < vertarr.size(); ++i) {
					tangarr.set(i * 4, -tangarr[i * 4]);
					tangarr.set(i * 4 + 2, -tangarr[i * 4 + 2]);
				}
			}

			for (int bsidx = 0; bsidx < bsarr.size(); ++bsidx) {
				vertarr = bsarr[bsidx].get(ArrayMesh::ARRAY_VERTEX);
				for (int i = 0; i < vertarr.size(); ++i) {
					vertarr.set(i, invert_vector * vertarr[i]);
				}

				if (bsarr[bsidx].get(ArrayMesh::ARRAY_NORMAL).get_type() == Variant::PACKED_VECTOR3_ARRAY) {
					PackedVector3Array normarr = bsarr[bsidx].get(ArrayMesh::ARRAY_NORMAL);
					for (int i = 0; i < vertarr.size(); ++i) {
						normarr.set(i, invert_vector * normarr[i]);
					}
				}

				if (bsarr[bsidx].get(ArrayMesh::ARRAY_TANGENT).get_type() == Variant::PACKED_FLOAT32_ARRAY) {
					PackedFloat32Array tangarr = bsarr[bsidx].get(ArrayMesh::ARRAY_TANGENT);
					for (int i = 0; i < vertarr.size(); ++i) {
						tangarr.set(i * 4, -tangarr[i * 4]);
						tangarr.set(i * 4 + 2, -tangarr[i * 4 + 2]);
					}
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
			surf_data_dict["name"] = name;
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
			String name = surf_data_by_mesh[surf_idx].get("name");
			Ref<Material> mat = surf_data_by_mesh[surf_idx].get("mat");

			mesh->add_surface(Mesh::PrimitiveType(prim), arr, bsarr, lods, mat, name, fmt_compress_flags);
		}
	}

	void skeleton_rename(Ref<GLTFState> gstate, Node *p_base_scene, Skeleton3D *p_skeleton, Ref<BoneMap> p_bone_map) {
		HashMap<StringName, int> original_bone_names_to_indices;
		HashMap<int, StringName> original_indices_to_bone_names;
		HashMap<int, StringName> original_indices_to_new_bone_names;
		int skellen = p_skeleton->get_bone_count();

		// Rename bones to their humanoid equivalents.
		for (int i = 0; i < skellen; ++i) {
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

		TypedArray<ImporterMeshInstance3D> nodes = p_base_scene->find_children("*", "ImporterMeshInstance3D");
		while (!nodes.is_empty()) {
			ImporterMeshInstance3D *mi = Object::cast_to<ImporterMeshInstance3D>(nodes.pop_back());
			Ref<Skin> skin = mi->get_skin();
			if (skin.is_valid()) {
				Node *node = mi->get_node(mi->get_skeleton_path());
				if (node && Object::cast_to<Skeleton3D>(node) && node == p_skeleton) {
					skellen = skin->get_bind_count();
					for (int i = 0; i < skellen; ++i) {
						String bind_bone_name = skin->get_bind_name(i);
						StringName bone_name_from_skel = p_bone_map->find_profile_bone_name(bind_bone_name);
						if (bone_name_from_skel != StringName()) {
							skin->set_bind_name(i, bone_name_from_skel);
						}
					}
				}
			}
		}

		// Rename bones in all Nodes by calling method.
		nodes = p_base_scene->find_children("*");
		while (!nodes.is_empty()) {
			ImporterMeshInstance3D *nd = cast_to<ImporterMeshInstance3D>(nodes.pop_back());
			if (nd->has_method("_notify_skeleton_bones_renamed")) {
				nd->call("_notify_skeleton_bones_renamed", p_base_scene, p_skeleton, p_bone_map);
			}
		}

		p_skeleton->set_name("GeneralSkeleton");
		p_skeleton->set_unique_name_in_owner(true);
	}

	void rotate_scene_180_inner(Node3D *p_node, Dictionary mesh_set, Dictionary skin_set) {
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

		for (int i = 0; i < p_node->get_child_count(); ++i) {
			Node3D *child = Object::cast_to<Node3D>(p_node->get_child(i));
			if (child) {
				rotate_scene_180_inner(child, mesh_set, skin_set);
			}
		}
	}

	void xtmp(Node3D *p_node, HashMap<Ref<Mesh>, bool> &mesh_set, HashMap<Ref<Skin>, bool> &skin_set) {
		ImporterMeshInstance3D *importer_mesh_instance = Object::cast_to<ImporterMeshInstance3D>(p_node);
		if (importer_mesh_instance) {
			mesh_set[importer_mesh_instance->get_mesh()] = true;
			skin_set[importer_mesh_instance->get_skin()] = true;
		}

		for (int i = 0; i < p_node->get_child_count(); ++i) {
			Node3D *child = Object::cast_to<Node3D>(p_node->get_child(i));
			if (child) {
				xtmp(child, mesh_set, skin_set);
			}
		}
	}

	void rotate_scene_180(Node3D *p_scene) {
		Dictionary mesh_set;
		Dictionary skin_set;

		rotate_scene_180_inner(p_scene, mesh_set, skin_set);
		Array mesh_set_keys = mesh_set.keys();
		;
		for (int32_t mesh_i = 0; mesh_i < mesh_set_keys.size(); mesh_i++) {
			Ref<Mesh> mesh = mesh_set_keys[mesh_i];
			adjust_mesh_zforward(mesh);
		}
		for (int32_t skin_set_i = 0; skin_set_i < skin_set.size(); skin_set_i++) {
			Ref<Skin> skin = skin_set.keys()[skin_set_i];
			for (int32_t bind_i = 0; bind_i < skin->get_bind_count(); bind_i++) {
				skin->set_bind_pose(bind_i, skin->get_bind_pose(bind_i) * ROTATE_180_TRANSFORM);
			}
		}
	}

	TypedArray<Basis> skeleton_rotate(Node *p_base_scene, Skeleton3D *src_skeleton, Ref<BoneMap> p_bone_map) {
		bool is_renamed = true;
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
			StringName src_bone_name = is_renamed ? StringName(src_skeleton->get_bone_name(src_idx)) : p_bone_map->find_profile_bone_name(src_skeleton->get_bone_name(src_idx));

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
				diffs[src_idx] = (src_skeleton->get_bone_rest(src_idx).basis.xform(tgt_rot.inverse().xform(diffs[src_skeleton->get_bone_parent(src_idx)])));
			} else {
				diffs[src_idx] = tgt_rot.inverse() * src_skeleton->get_bone_rest(src_idx).basis;
			}

			Basis diff;

			if (src_skeleton->get_bone_parent(src_idx) >= 0) {
				diff = diffs[src_skeleton->get_bone_parent(src_idx)];
			}

			src_skeleton->set_bone_rest(src_idx, Transform3D(tgt_rot, diff.xform(src_skeleton->get_bone_rest(src_idx).origin)));
		}

		prof_skeleton->queue_free();
		return diffs;
	}

	void apply_rotation(Node *p_base_scene, Skeleton3D *src_skeleton) {
		Array nodes = p_base_scene->find_children("*", "ImporterMeshInstance3D");

		while (!nodes.is_empty()) {
			Node *this_node = Object::cast_to<Node>(nodes.back());
			nodes.pop_back();

			if (ImporterMeshInstance3D *mi = Object::cast_to<ImporterMeshInstance3D>(this_node)) {
				Ref<Skin> skin = mi->get_skin();
				Node *node = mi->get_node_or_null(mi->get_skeleton_path());

				if (skin.is_valid() && node && Object::cast_to<Skeleton3D>(node) == src_skeleton) {
					int skellen = skin->get_bind_count();

					for (int i = 0; i < skellen; ++i) {
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
		Auto, // Create headlessModel
		Both, // Default layer
		ThirdPersonOnly,
		FirstPersonOnly,
	};

	HashMap<String, VRMExtension::FirstPersonFlag> FirstPersonParser;
	VRMExtension() {
		FirstPersonParser.insert("Auto", FirstPersonFlag::Auto);
		FirstPersonParser.insert("Both", FirstPersonFlag::Both);
		FirstPersonParser.insert("FirstPersonOnly", FirstPersonFlag::FirstPersonOnly);
		FirstPersonParser.insert("ThirdPersonOnly", FirstPersonFlag::ThirdPersonOnly);
	}

	Ref<Material> process_khr_material(Ref<StandardMaterial3D> orig_mat, Dictionary gltf_mat_props) {
		// VRM spec requires support for the KHR_materials_unlit extension.
		if (gltf_mat_props.has("extensions")) {
			// TODO : Implement this extension upstream.
			bool is_valid = false;
			gltf_mat_props["extensions"].get("KHR_materials_unlit", &is_valid);
			if (is_valid) {
				// TODO : validate that this is sufficient.
				orig_mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
			}
		}
		return orig_mat;
	}

	Dictionary vrm_get_texture_info(Array gltf_images, Dictionary vrm_mat_props, String unity_tex_name) {
		Dictionary texture_info;
		texture_info["tex"] = Ref<Texture2D>();
		texture_info["offset"] = Vector3(0.0, 0.0, 0.0);
		texture_info["scale"] = Vector3(1.0, 1.0, 1.0);

		bool is_valid = false;
		vrm_mat_props["textureProperties"].get(unity_tex_name, &is_valid);
		if (is_valid) {
			int mainTexId = vrm_mat_props["textureProperties"].get(unity_tex_name);
			if (is_valid) {
				Ref<Texture2D> mainTexImage = gltf_images[mainTexId];
				texture_info["tex"] = mainTexImage;
			}
		}
		vrm_mat_props["vectorProperties"].get(unity_tex_name, &is_valid);
		if (is_valid) {
			Array offsetScale = vrm_mat_props["vectorProperties"].get(unity_tex_name);
			texture_info["offset"] = Vector3(offsetScale[0], offsetScale[1], 0.0);
			texture_info["scale"] = Vector3(offsetScale[2], offsetScale[3], 1.0);
		}

		return texture_info;
	}

	float vrm_get_float(Dictionary vrm_mat_props, String key, float def) {
		bool is_valid = false;
		float result = vrm_mat_props["floatProperties"].get(key, &is_valid);
		if (!is_valid) {
			return def;
		}
		return result;
	}

	Ref<Material> _process_vrm_material(Ref<Material> orig_mat, Array gltf_images, Dictionary vrm_mat_props) {
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
		bool is_valid = false;
		int outline_width_mode = vrm_mat_props["floatProperties"].get("_OutlineWidthMode", &is_valid);
		if (!is_valid) {
			outline_width_mode = 0;
		}
		int blend_mode = blend_mode = vrm_mat_props["floatProperties"].get("_BlendMode", &is_valid);
		if (!is_valid) {
			blend_mode = 0;
		}
		int cull_mode = vrm_mat_props["floatProperties"].get("_CullMode", &is_valid);
		if (!is_valid) {
			cull_mode = 2;
		}
		int outl_cull_mode = int(vrm_mat_props["floatProperties"].get("_OutlineCullMode", &is_valid));
		if (!is_valid) {
			outl_cull_mode = 1;
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
			bool is_valid = false;
			Variant text_info_tex = tex_info.get("tex", &is_valid);
			if (is_valid) {
				new_mat->set_shader_parameter(param_name, text_info_tex);
				if (outline_mat != nullptr) {
					outline_mat->set_shader_parameter(param_name, text_info_tex);
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

	void _update_materials(Dictionary vrm_extension, Ref<GLTFState> gstate) {
		TypedArray<Image> images = gstate->get_images();
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

	Node *_get_skel_godot_node(Ref<GLTFState> gstate, Array nodes, Array skeletons, int skel_id) {
		for (int i = 0; i < nodes.size(); ++i) {
			Ref<GLTFNode> gltf_node = nodes[i];
			if (gltf_node->get_skeleton() == skel_id) {
				return gstate->get_scene_node(i);
			}
		}

		return nullptr;
	}

	class SkelBone {
	public:
		Ref<Skeleton3D> skel;
		String bone_name;
	};

	Ref<Resource> _create_meta(Node *root_node, AnimationPlayer *animplayer, Dictionary vrm_extension, Ref<GLTFState> gstate, Skeleton3D *skeleton, Ref<BoneMap> humanBones, Dictionary human_bone_to_idx, TypedArray<Basis> pose_diffs) {
		TypedArray<GLTFNode> nodes = gstate->get_nodes();

		NodePath skeletonPath = root_node->get_path_to(skeleton);
		root_node->set("vrm_skeleton", skeletonPath);

		NodePath animPath = root_node->get_path_to(animplayer);
		root_node->set("vrm_animplayer", animPath);

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
		bool is_valid = false;
		String exporter_version = vrm_extension.get("exporterVersion", &is_valid);
		if (!is_valid) {
			exporter_version = String();
		}
		vrm_meta->set_exporter_version(exporter_version);
		String spec_version = vrm_extension.get("specVersion", &is_valid);
		if (!is_valid) {
			spec_version = String();
		}
		vrm_meta->set_spec_version(spec_version);

		Dictionary vrm_extension_meta = vrm_extension["meta"];
		if (!vrm_extension_meta.is_empty()) {
			String title = vrm_extension_meta.get("title", &is_valid);
			if (!is_valid) {
				title = "";
			}
			vrm_meta->set_title(title);

			String version = vrm_extension_meta.get("version", &is_valid);
			if (!is_valid) {
				version = "";
			}
			vrm_meta->set_version(version);

			String author = vrm_extension_meta.get("author", &is_valid);
			if (!is_valid) {
				author = "";
			}
			vrm_meta->set_author(author);

			String contact_information = vrm_extension_meta.get("contactInformation", &is_valid);
			if (!is_valid) {
				contact_information = "";
			}
			vrm_meta->set_contact_information(contact_information);

			String reference_information = vrm_extension_meta.get("reference", &is_valid);
			if (!is_valid) {
				reference_information = "";
			}
			vrm_meta->set_reference_information(reference_information);

			int tex = vrm_extension_meta.get("texture", is_valid);
			if (!is_valid || is_valid && !(tex >= 0)) {
				tex = -1;
			}
			Ref<GLTFTexture> gltftex = gstate->get_textures()[tex];
			vrm_meta->set_texture(gstate->get_images()[gltftex->get_src_image()]);

			String allowed_user_name = vrm_extension_meta.get("allowedUserName", &is_valid);
			if (!is_valid) {
				allowed_user_name = "";
			}
			vrm_meta->set_allowed_user_name(allowed_user_name);

			String violent_usage = vrm_extension_meta.get("violentUssageName", &is_valid); // Ussage(sic.) in VRM spec
			if (!is_valid) {
				violent_usage = "";
			}
			vrm_meta->set_violent_usage(violent_usage);

			String sexual_usage = vrm_extension_meta.get("sexualUssageName", &is_valid); // Ussage(sic.) in VRM spec
			if (!is_valid) {
				sexual_usage = "";
			}
			vrm_meta->set_sexual_usage(sexual_usage);

			String commercial_usage = vrm_extension_meta.get("commercialUssageName", &is_valid); // Ussage(sic.) in VRM spec
			if (!is_valid) {
				commercial_usage = "";
			}
			vrm_meta->set_commercial_usage(commercial_usage);

			String other_permission_url = vrm_extension_meta.get("otherPermissionUrl", &is_valid);
			if (!is_valid) {
				other_permission_url = "";
			}
			vrm_meta->set_other_permission_url(other_permission_url);

			String license_name = vrm_extension_meta.get("licenseName", &is_valid);
			if (!is_valid) {
				license_name = "";
			}
			vrm_meta->set_license_name(license_name);

			String other_license_url = vrm_extension_meta.get("otherLicenseUrl", &is_valid);
			if (!is_valid) {
				other_license_url = "";
			}
			vrm_meta->set_other_license_url(other_license_url);
		}

		return vrm_meta;
	}

	AnimationPlayer *create_animation_player(AnimationPlayer *animplayer, Dictionary vrm_extension, Ref<GLTFState> gstate, Dictionary human_bone_to_idx, TypedArray<Basis> pose_diffs) {
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

		Variant firstperson = vrm_extension["firstPerson"];

		Ref<Animation> firstpersanim = memnew(Animation);
		animation_library->add_animation("FirstPerson", firstpersanim);

		Ref<Animation> thirdpersanim = memnew(Animation);
		animation_library->add_animation("ThirdPerson", thirdpersanim);

		Array skeletons = gstate->get_skeletons();
		bool is_valid = false;
		int head_bone_idx = firstperson.get("firstPersonBone", &is_valid);
		if (!is_valid) {
			head_bone_idx = -1;
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
		Dictionary mesh_annotations = firstperson.get("meshAnnotations");
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
		String look_at_type_name = firstperson.get("lookAtTypeName", &is_valid);
		if (!is_valid) {
			look_at_type_name = "";
		}
		if (look_at_type_name == "Bone") {
			Dictionary horizout = firstperson.get("lookAtHorizontalOuter");
			Dictionary horizin = firstperson.get("lookAtHorizontalInner");
			Dictionary vertup = firstperson.get("lookAtVerticalUp");
			Dictionary vertdown = firstperson.get("lookAtVerticalDown");
			int lefteye = human_bone_to_idx.get("leftEye", -1);
			int righteye = human_bone_to_idx.get("rightEye", -1);
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
			// LOOKUP
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

			// LOOKDOWN
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

	void parse_secondary_node(Node3D *secondary_node, Dictionary vrm_extension, Ref<GLTFState> gstate, Array pose_diffs, bool is_vrm_0) {
		Array nodes = gstate->get_nodes();
		Array skeletons = gstate->get_skeletons();

		Vector3 offset_flip = is_vrm_0 ? Vector3(-1, 1, 1) : Vector3(1, 1, 1);

		Dictionary vrm_extension_secondary_animation = vrm_extension["secondaryAnimation"];
		Array collider_groups = vrm_extension_secondary_animation["colliderGroups"];
		for (int i = 0; i < collider_groups.size(); ++i) {
			Dictionary cgroup = collider_groups[i];
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
				String bone_name = cast_to<Node>(nodes[int(cgroup["node"])])->get_name();
				collider_group->set_bone(bone_name);
				collider_group->set_name(collider_group->get_bone());
				pose_diff = pose_diffs[skeleton->find_bone(collider_group->get_bone())];
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

		Array spring_bones;
		Array bone_groups = vrm_extension["secondaryAnimation"].get("boneGroups");
		for (int i = 0; i < bone_groups.size(); ++i) {
			Dictionary sbone = bone_groups[i];
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

	void add_joints_recursive(Dictionary &new_joints_set, Array gltf_nodes, int bone, bool include_child_meshes = false) {
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

	void add_joint_set_as_skin(Dictionary obj, Dictionary new_joints_set) {
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

	bool add_vrm_nodes_to_skin(Dictionary obj) {
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

	Error _import_preflight(Ref<GLTFState> gstate, PackedStringArray psa = PackedStringArray(), Variant psa2 = Variant()) {
		Dictionary gltf_json_parsed = gstate->get_json();
		if (!add_vrm_nodes_to_skin(gltf_json_parsed)) {
			ERR_PRINT("Failed to find required VRM keys in json");
			return ERR_INVALID_DATA;
		}
		return OK;
	}

	TypedArray<Basis> apply_retarget(Ref<GLTFState> gstate, Node *root_node, Skeleton3D *skeleton, Ref<BoneMap> bone_map) {
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

	Error _import_post(Ref<GLTFState> gstate, Node *node) {
		Ref<GLTFDocument> gltf;
		gltf.instantiate();
		Node3D *root_node = cast_to<Node3D>(node);
		if (!root_node) {
			ERR_PRINT("The root node is not a Node3D.");
			return ERR_INVALID_DATA;
		}

		bool is_vrm_0 = true;

		Dictionary gltf_json = gstate->get_json();
		Dictionary gltf_vrm_extension = gltf_json["extensions"];
		Dictionary vrm_extension = gltf_vrm_extension["VRM"];

		Dictionary human_bone_to_idx;
		// Ignoring in ["humanoid"]: armStretch, legStretch, upperArmTwist,
		// lowerArmTwist, upperLegTwist, lowerLegTwist, feetSpacing,
		// and hasTranslationDoF
		Dictionary humanoid = vrm_extension["humanoid"];
		Array human_bones = humanoid["humanBones"];
		for (int i = 0; i < human_bones.size(); ++i) {
			Dictionary human_bone = human_bones[i];
			human_bone_to_idx[human_bone["bone"]] = static_cast<int>(human_bone["node"]);
			// Unity Mecanim properties:
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
		human_bones_map->set_profile(memnew(SkeletonProfileHumanoid));

		Ref<VRMConstants> vrmconst_inst = memnew(VRMConstants(is_vrm_0)); // vrm 0.0
		Array human_bone_keys = human_bone_to_idx.keys();

		for (int i = 0; i < human_bone_keys.size(); ++i) {
			const String &human_bone_name = human_bone_keys[i];
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
		if (do_retarget) {
			pose_diffs = apply_retarget(gstate, root_node, skeleton, human_bones_map);
		} else {
			// resize is busted for TypedArray and crashes Godot
			for (int i = 0; i < skeleton->get_bone_count(); ++i) {
				pose_diffs.push_back(Basis());
			}
		}

		_update_materials(vrm_extension, gstate);

		AnimationPlayer *animplayer = memnew(AnimationPlayer);
		animplayer->set_name("anim");
		root_node->add_child(animplayer);
		animplayer->set_owner(root_node);
		create_animation_player(animplayer, vrm_extension, gstate, human_bone_to_idx, pose_diffs);

		root_node->set_script(vrm_top_level);

		Ref<Resource> vrm_meta = _create_meta(root_node, animplayer, vrm_extension, gstate, skeleton, human_bones_map, human_bone_to_idx, pose_diffs);
		root_node->set("vrm_meta", vrm_meta);
		root_node->set("vrm_secondary", NodePath());

		Array collider_groups = vrm_extension["secondaryAnimation"].get("colliderGroups");
		int32_t collider_groups_size = collider_groups.size();
		Array bone_groups = vrm_extension["secondaryAnimation"].get("boneGroups");
		int32_t bone_group_size = bone_groups.size();
		if (vrm_extension.has("secondaryAnimation") &&
				(collider_groups_size > 0 ||
						bone_group_size > 0)) {
			Node3D *secondary_node = cast_to<Node3D>(root_node->get_node(NodePath("secondary")));
			if (!secondary_node) {
				secondary_node = memnew(Node3D);
				root_node->add_child(secondary_node);
				secondary_node->set_owner(root_node);
				secondary_node->set_name("secondary");
			}

			NodePath secondary_path = root_node->get_path_to(secondary_node);
			root_node->set("vrm_secondary", secondary_path);

			parse_secondary_node(secondary_node, vrm_extension, gstate, pose_diffs, is_vrm_0);
		}
		return OK;
	}
};
