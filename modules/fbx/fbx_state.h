/**************************************************************************/
/*  fbx_state.h                                                           */
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

#ifndef FBX_STATE_H
#define FBX_STATE_H

#include "structures/fbx_animation.h"
#include "structures/fbx_mesh.h"
#include "structures/fbx_node.h"
#include "structures/fbx_skeleton.h"
#include "structures/fbx_skin.h"
#include "structures/fbx_texture.h"

#include "thirdparty/ufbx/ufbx.h"

class FBXState : public Resource {
	GDCLASS(FBXState, Resource);
	friend class FBXDocument;

	// Smart pointer that holds the loaded scene.
	ufbx_unique_ptr<ufbx_scene> scene;

	String base_path;
	String filename;
	Dictionary json;
	int major_version = 0;
	int minor_version = 0;
	String copyright;
	Vector<uint8_t> glb_data;

	bool use_named_skin_binds = false;
	bool use_khr_texture_transform = false;
	bool discard_meshes_and_materials = false;
	bool create_animations = true;

	int handle_binary_image = HANDLE_BINARY_EXTRACT_TEXTURES;

	Vector<Ref<FBXNode>> nodes;
	Vector<Vector<uint8_t>> buffers;

	Vector<Ref<FBXMesh>> meshes; // meshes are loaded directly, no reason not to.

	Vector<AnimationPlayer *> animation_players;
	HashMap<Ref<Material>, FBXMaterialIndex> material_cache;
	Vector<Ref<Material>> materials;

	String scene_name;
	Vector<int> root_nodes;
	Vector<Ref<FBXTexture>> textures;
	Vector<Ref<Texture2D>> images;
	Vector<String> extensions_used;
	Vector<String> extensions_required;
	Vector<Ref<Image>> source_images;

	HashMap<uint64_t, Image::AlphaMode> alpha_mode_cache;
	HashMap<Pair<uint64_t, uint64_t>, FBXTextureIndex, PairHash<uint64_t, uint64_t>> albedo_transparency_textures;

	Vector<Ref<FBXSkin>> skins;
	Vector<FBXSkinIndex> skin_indices;
	HashSet<String> unique_names;
	HashSet<String> unique_animation_names;

	Vector<Ref<FBXSkeleton>> skeletons;
	Vector<Ref<FBXAnimation>> animations;
	HashMap<FBXNodeIndex, Node *> scene_nodes;
	HashMap<FBXNodeIndex, ImporterMeshInstance3D *> scene_mesh_instances;

	HashMap<ObjectID, FBXSkeletonIndex> skeleton3d_to_fbx_skeleton;
	HashMap<ObjectID, HashMap<ObjectID, FBXSkinIndex>> skin_and_skeleton3d_to_fbx_skin;
	Dictionary additional_data;

protected:
	static void _bind_methods();

public:
	void add_used_extension(const String &p_extension, bool p_required = false);

	enum FBXHandleBinary {
		HANDLE_BINARY_DISCARD_TEXTURES = 0,
		HANDLE_BINARY_EXTRACT_TEXTURES,
		HANDLE_BINARY_EMBED_AS_BASISU,
		HANDLE_BINARY_EMBED_AS_UNCOMPRESSED, // if this value changes from 3, ResourceImporterScene::pre_import must be changed as well.
	};
	int32_t get_handle_binary_image() {
		return handle_binary_image;
	}
	void set_handle_binary_image(int32_t p_handle_binary_image) {
		handle_binary_image = p_handle_binary_image;
	}

	Dictionary get_json();
	void set_json(Dictionary p_json);

	int get_major_version();
	void set_major_version(int p_major_version);

	int get_minor_version();
	void set_minor_version(int p_minor_version);

	String get_copyright() const;
	void set_copyright(const String &p_copyright);

	Vector<uint8_t> get_glb_data();
	void set_glb_data(Vector<uint8_t> p_glb_data);

	bool get_use_named_skin_binds();
	void set_use_named_skin_binds(bool p_use_named_skin_binds);

	bool get_discard_textures();
	void set_discard_textures(bool p_discard_textures);

	bool get_embed_as_basisu();
	void set_embed_as_basisu(bool p_embed_as_basisu);

	bool get_extract_textures();
	void set_extract_textures(bool p_extract_textures);

	bool get_discard_meshes_and_materials();
	void set_discard_meshes_and_materials(bool p_discard_meshes_and_materials);

	TypedArray<FBXNode> get_nodes();
	void set_nodes(TypedArray<FBXNode> p_nodes);

	TypedArray<PackedByteArray> get_buffers();
	void set_buffers(TypedArray<PackedByteArray> p_buffers);

	TypedArray<FBXMesh> get_meshes();
	void set_meshes(TypedArray<FBXMesh> p_meshes);

	TypedArray<Material> get_materials();
	void set_materials(TypedArray<Material> p_materials);

	String get_scene_name();
	void set_scene_name(String p_scene_name);

	String get_base_path();
	void set_base_path(String p_base_path);

	String get_filename() const;
	void set_filename(const String &p_filename);

	PackedInt32Array get_root_nodes();
	void set_root_nodes(PackedInt32Array p_root_nodes);

	TypedArray<FBXTexture> get_textures();
	void set_textures(TypedArray<FBXTexture> p_textures);

	TypedArray<Texture2D> get_images();
	void set_images(TypedArray<Texture2D> p_images);

	TypedArray<FBXSkin> get_skins();
	void set_skins(TypedArray<FBXSkin> p_skins);

	TypedArray<String> get_unique_names();
	void set_unique_names(TypedArray<String> p_unique_names);

	TypedArray<String> get_unique_animation_names();
	void set_unique_animation_names(TypedArray<String> p_unique_names);

	TypedArray<FBXSkeleton> get_skeletons();
	void set_skeletons(TypedArray<FBXSkeleton> p_skeletons);

	bool get_create_animations();
	void set_create_animations(bool p_create_animations);

	TypedArray<FBXAnimation> get_animations();
	void set_animations(TypedArray<FBXAnimation> p_animations);

	Node *get_scene_node(FBXNodeIndex idx);
	FBXNodeIndex get_node_index(Node *p_node);

	int get_animation_players_count(int idx);

	AnimationPlayer *get_animation_player(int idx);

	Variant get_additional_data(const StringName &p_extension_name);
	void set_additional_data(const StringName &p_extension_name, Variant p_additional_data);
};

#endif // FBX_STATE_H
