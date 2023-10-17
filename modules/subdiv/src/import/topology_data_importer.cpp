/**************************************************************************/
/*  topology_data_importer.cpp                                            */
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

#include "topology_data_importer.hpp"

#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"

#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/importer_mesh.h"
#include "scene/resources/mesh.h"

#include "../resources/baked_subdiv_mesh.hpp"
#include "../subdivision/subdivision_baker.hpp"

TopologyDataImporter::TopologyDataImporter() {
}

TopologyDataImporter::~TopologyDataImporter() {
}

void TopologyDataImporter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("convert_importer_meshinstance_to_subdiv", "object", "mode", "level"), &TopologyDataImporter::convert_importer_meshinstance_to_subdiv);
	BIND_ENUM_CONSTANT(BAKED_SUBDIV_MESH);
	BIND_ENUM_CONSTANT(IMPORTER_MESH)
}

TopologyDataImporter::SurfaceVertexArrays::SurfaceVertexArrays(const Array &p_mesh_arrays) {
	ERR_FAIL_COND(p_mesh_arrays.size() != Mesh::ARRAY_MAX);

	vertex_array = p_mesh_arrays[Mesh::ARRAY_VERTEX];
	normal_array = p_mesh_arrays[Mesh::ARRAY_NORMAL];
	index_array = p_mesh_arrays[Mesh::ARRAY_INDEX];
	uv_array = p_mesh_arrays[Mesh::ARRAY_TEX_UV];
	bones_array = p_mesh_arrays[Mesh::ARRAY_BONES];
	weights_array = p_mesh_arrays[Mesh::ARRAY_WEIGHTS];
}

void TopologyDataImporter::convert_importer_meshinstance_to_subdiv(ImporterMeshInstance3D *importer_mesh_instance, ImportMode import_mode, int32_t subdiv_level) {
	Ref<ImporterMesh> importer_mesh = importer_mesh_instance->get_mesh();
	ERR_FAIL_COND_MSG(importer_mesh.is_null(), "Mesh is null");

	// Always generate to flush out any errors.

	Ref<TopologyDataMesh> topology_data_mesh;
	topology_data_mesh.instantiate();
	topology_data_mesh->set_name(importer_mesh->get_name());
	// generate quad_mesh data
	for (int surface_index = 0; surface_index < importer_mesh->get_surface_count(); surface_index++) {
		//convert actual mesh data to quad
		Array p_arrays = importer_mesh->get_surface_arrays(surface_index);
		int32_t format = generate_fake_format(p_arrays);

		// generate_fake_format returns 0 if size != ARRAY_MAX
		if (format == 0 || !(format & Mesh::ARRAY_FORMAT_VERTEX)) {
			continue;
		} else if (!(format & Mesh::ARRAY_FORMAT_INDEX)) {
			//generate index array, p_arrays also used by blend_shapes so generating here
			const PackedVector3Array &vertex_array = p_arrays[Mesh::ARRAY_VERTEX];
			PackedInt32Array simple_index_array;
			for (int i = 0; i < vertex_array.size(); i++) {
				simple_index_array.push_back(i);
			}
			p_arrays[Mesh::ARRAY_INDEX] = simple_index_array;
			format |= Mesh::ARRAY_FORMAT_INDEX;
		}

		Array surface_arrays;
		int topology_type = _generate_topology_surface_arrays(SurfaceVertexArrays(p_arrays), format, surface_arrays);

		//convert all blend shapes in the exact same way (BlendShapeArrays are also just an Array with the size of ARRAY_MAX and data offsets)
		Array blend_shape_arrays;
		Array topology_data_blend_shape_arrays;
		for (int blend_shape_idx = 0; blend_shape_idx < importer_mesh->get_blend_shape_count(); blend_shape_idx++) {
			blend_shape_arrays.push_back(importer_mesh->get_surface_blend_shape_arrays(surface_index, blend_shape_idx));
		}
		if (!blend_shape_arrays.is_empty()) {
			topology_data_blend_shape_arrays = _generate_packed_blend_shapes(blend_shape_arrays, p_arrays[Mesh::ARRAY_INDEX], p_arrays[Mesh::ARRAY_VERTEX]);
		}

		ERR_FAIL_COND(!surface_arrays.size());
		topology_data_mesh->add_surface(surface_arrays, topology_data_blend_shape_arrays,
				importer_mesh->get_surface_material(surface_index), importer_mesh->get_surface_name(surface_index), format, topology_type);
	}
	//add blendshapes to data
	for (int blend_shape_idx = 0; blend_shape_idx < importer_mesh->get_blend_shape_count(); blend_shape_idx++) {
		topology_data_mesh->add_blend_shape_name(importer_mesh->get_blend_shape_name(blend_shape_idx));
	}

	StringName mesh_instance_name = importer_mesh_instance->get_name();
	switch (import_mode) {
		case ImportMode::BAKED_SUBDIV_MESH: {
			Ref<BakedSubdivMesh> subdiv_mesh;
			subdiv_mesh.instantiate();

			subdiv_mesh->set_data_mesh(topology_data_mesh);
			subdiv_mesh->set_subdiv_level(subdiv_level);
			subdiv_mesh->set_blend_shape_mode(Mesh::BLEND_SHAPE_MODE_NORMALIZED); //otherwise data would need to be converted

			MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(_replace_importer_mesh_instance_with_mesh_instance(importer_mesh_instance));

			importer_mesh_instance->replace_by(mesh_instance, false);
			mesh_instance->set_name(mesh_instance_name);
			mesh_instance->set_mesh(subdiv_mesh);
			break;
		}
		case ImportMode::IMPORTER_MESH: {
			Ref<ImporterMesh> subdiv_importer_mesh;
			subdiv_importer_mesh.instantiate();
			Ref<SubdivisionBaker> baker;
			baker.instantiate();
			subdiv_importer_mesh = baker->get_importer_mesh(subdiv_importer_mesh, topology_data_mesh, subdiv_level, true);
			subdiv_importer_mesh->generate_lods(Math::deg_to_rad(60.0f), Math::deg_to_rad(25.0f), Array());
			importer_mesh_instance->set_mesh(subdiv_importer_mesh);
			MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(_replace_importer_mesh_instance_with_mesh_instance(importer_mesh_instance));
			importer_mesh_instance->replace_by(mesh_instance, false);
			mesh_instance->set_name(mesh_instance_name);
			mesh_instance->set_mesh(subdiv_importer_mesh->get_mesh());
			break;
		}
		default:
			ERR_PRINT("Import mode doesn't exist");
			break;
	}
}

int TopologyDataImporter::_generate_topology_surface_arrays(const SurfaceVertexArrays &surface, int32_t format, Array &surface_arrays) {
	ERR_FAIL_COND_V(!(format & Mesh::ARRAY_FORMAT_INDEX), TopologyDataMesh::TOPOLOGY_DATA_MESH_QUAD);
	TopologySurfaceData topology_surface = _remove_duplicate_vertices(surface, format);
	bool is_quad = _merge_to_quads(topology_surface.index_array, topology_surface.uv_array, format);
	bool has_uv = format & Mesh::ARRAY_FORMAT_TEX_UV;

	surface_arrays.resize(TopologyDataMesh::ARRAY_MAX);
	surface_arrays[TopologyDataMesh::ARRAY_VERTEX] = topology_surface.vertex_array;
	surface_arrays[TopologyDataMesh::ARRAY_NORMAL] = topology_surface.normal_array;
	// surface_arrays[Mesh::ARRAY_TANGENT]
	surface_arrays[TopologyDataMesh::ARRAY_BONES] = topology_surface.bones_array; // TODO: docs say bones array can also be floats, might cause issues
	surface_arrays[TopologyDataMesh::ARRAY_WEIGHTS] = topology_surface.weights_array;
	surface_arrays[TopologyDataMesh::ARRAY_INDEX] = topology_surface.index_array;
	if (has_uv) {
		PackedInt32Array uv_index_array = _generate_uv_index_array(topology_surface.uv_array);
		surface_arrays[TopologyDataMesh::ARRAY_TEX_UV] = topology_surface.uv_array;
		surface_arrays[TopologyDataMesh::ARRAY_UV_INDEX] = uv_index_array;
	} else { //this is to avoid issues with casting null to Array when using these as reference
		surface_arrays[TopologyDataMesh::ARRAY_TEX_UV] = PackedVector2Array();
		surface_arrays[TopologyDataMesh::ARRAY_UV_INDEX] = PackedInt32Array();
	}

	if (is_quad) {
		return TopologyDataMesh::TOPOLOGY_DATA_MESH_QUAD;
	} else {
		return TopologyDataMesh::TOPOLOGY_DATA_MESH_TRIANGLE;
	}
}

TopologyDataImporter::TopologySurfaceData TopologyDataImporter::_remove_duplicate_vertices(const SurfaceVertexArrays &surface, int32_t format) {
	//these booleans decide what data gets stored, could be used directly with format, but I think this is more readable
	bool has_uv = format & Mesh::ARRAY_FORMAT_TEX_UV;
	bool has_skinning = (format & Mesh::ARRAY_FORMAT_BONES) && (format & Mesh::ARRAY_FORMAT_WEIGHTS);
	bool double_bone_weights = format & Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS;
	bool has_normals = format & Mesh::ARRAY_FORMAT_NORMAL;
	//TODO: maybe add tangents, uv2 here too, considering that data is lost after subdivisions not that urgent

	TopologySurfaceData topology_surface;
	int max_index = 0;
	HashMap<Vector3, int> original_verts;
	for (int vert_index = 0; vert_index < surface.index_array.size(); vert_index++) {
		int index = surface.index_array[vert_index];
		if (original_verts.has(surface.vertex_array[index])) {
			topology_surface.index_array.append(original_verts.get(surface.vertex_array[index]));
		} else {
			topology_surface.vertex_array.append(surface.vertex_array[index]);
			if (has_normals) {
				topology_surface.normal_array.append(surface.normal_array[index]);
			}

			// weights and bones arrays to corresponding vertex (4 weights per vert)
			if (has_skinning) {
				int bone_weights_per_vert = double_bone_weights ? 8 : 4;
				for (int weight_index = index * bone_weights_per_vert; weight_index < (index + 1) * bone_weights_per_vert; weight_index++) {
					topology_surface.weights_array.append(surface.weights_array[weight_index]);
					topology_surface.bones_array.append(surface.bones_array[weight_index]);
				}
			}

			topology_surface.index_array.append(max_index);

			original_verts.insert(surface.vertex_array[index], max_index);

			max_index++;
		}

		// always use uvs if they exist
		if (has_uv) {
			topology_surface.uv_array.append(surface.uv_array[index]);
		}
	}

	return topology_surface;
}

// Goes through index_array and always merges the 6 indices of 2 triangles to 1 quad (uv_array also updated)
// returns boolean if conversion to quad was successful
bool TopologyDataImporter::_merge_to_quads(PackedInt32Array &index_array, PackedVector2Array &uv_array, int32_t format) {
	if (index_array.size() % 6 != 0) {
		return false;
	}

	bool has_uv = format & Mesh::ARRAY_FORMAT_TEX_UV;

	PackedInt32Array quad_index_array;
	PackedVector2Array quad_uv_array;
	for (int i = 0; i < index_array.size(); i += 6) {
		// initalize unshared with verts from triangle 1
		PackedInt32Array unshared_verts;
		PackedInt32Array pos_unshared_verts; // array to keep track of index of index in index_array
		for (int t1 = i; t1 < i + 3; t1++) {
			unshared_verts.push_back(index_array[t1]);
			pos_unshared_verts.push_back(t1);
		}
		PackedInt32Array shared_verts;
		PackedInt32Array pos_shared_verts;
		// compare with triangle 2
		for (int t2 = i + 3; t2 < i + 6; t2++) {
			const int &index = index_array[t2]; // index from triangle 2
			int pos = unshared_verts.find(index, 0);
			if (pos != -1) {
				shared_verts.push_back(index);
				pos_shared_verts.push_back(t2);
				unshared_verts.remove_at(pos);
				pos_unshared_verts.remove_at(pos);
			} else {
				unshared_verts.push_back(index);
				pos_unshared_verts.push_back(t2);
			}
		}
		if (shared_verts.size() != 2) {
			return false;
		}

		/*isn't always like this, but for recreating triangles later this is
		unshared[0]	 - 	shared[0]
			|				|
		shared[1]	 -	unshared[1]
		*/
		quad_index_array.append(unshared_verts[0]);
		quad_index_array.append(shared_verts[0]);
		quad_index_array.append(unshared_verts[1]);
		quad_index_array.append(shared_verts[1]);

		// uv's
		if (has_uv) {
			quad_uv_array.append(uv_array[pos_unshared_verts[0]]);
			quad_uv_array.append(uv_array[pos_shared_verts[0]]);
			quad_uv_array.append(uv_array[pos_unshared_verts[1]]);
			quad_uv_array.append(uv_array[pos_shared_verts[1]]);
		}
	}
	index_array = quad_index_array;
	uv_array = quad_uv_array;
	return true;
}

// tries to store uv's as compact as possible with an index array, an additional index array
// is needed for uv data, because the index array for the vertices connects faces while uv's
// can still be different for faces even when it's the same vertex, works similar to _remove_duplicate_vertices
PackedInt32Array TopologyDataImporter::_generate_uv_index_array(PackedVector2Array &uv_array) {
	ERR_FAIL_COND_V(uv_array.is_empty(), PackedInt32Array());
	int max_index = 0;
	HashMap<Vector2, int> found_uvs;
	PackedInt32Array uv_index_array;
	PackedVector2Array packed_uv_array; // will overwrite the given uv_array before returning
	for (int uv_index = 0; uv_index < uv_array.size(); uv_index++) {
		Vector2 single_uv = uv_array[uv_index];
		if (found_uvs.has(single_uv)) {
			uv_index_array.append(found_uvs.get(single_uv));
		} else {
			packed_uv_array.append(single_uv);
			uv_index_array.append(max_index);
			found_uvs.insert(single_uv, max_index);
			max_index++;
		}
	}
	uv_array = packed_uv_array;
	return uv_index_array;
}

//same as remove duplicate vertices, just runs for every blend shape
Array TopologyDataImporter::_generate_packed_blend_shapes(const Array &tri_blend_shapes, const PackedInt32Array &mesh_index_array,
		const PackedVector3Array &mesh_vertex_array) {
	int max_index = 0;
	HashMap<Vector3, int> original_verts;
	Vector<PackedVector3Array> packed_vertex_arrays;
	packed_vertex_arrays.resize(tri_blend_shapes.size());
	for (int vert_index = 0; vert_index < mesh_index_array.size(); vert_index++) {
		int index = mesh_index_array[vert_index];
		if (!original_verts.has(mesh_vertex_array[index])) {
			original_verts.insert(mesh_vertex_array[index], max_index);
			for (int blend_shape_idx = 0; blend_shape_idx < tri_blend_shapes.size(); blend_shape_idx++) { //TODO: possibly add normal and tangent here as well
				const Array &single_blend_shape_array = tri_blend_shapes[blend_shape_idx];
				const PackedVector3Array &tri_blend_vertex_array = single_blend_shape_array[Mesh::ARRAY_VERTEX];
				packed_vertex_arrays.write[blend_shape_idx].append(tri_blend_vertex_array[index] - mesh_vertex_array[index]);
			}
			max_index++;
		}
	}

	Array packed_blend_shape_array;
	packed_blend_shape_array.resize(tri_blend_shapes.size());
	for (int blend_shape_idx = 0; blend_shape_idx < tri_blend_shapes.size(); blend_shape_idx++) {
		Array single_blend_shape_array;
		single_blend_shape_array.resize(TopologyDataMesh::ARRAY_MAX);
		single_blend_shape_array[TopologyDataMesh::ARRAY_VERTEX] = packed_vertex_arrays.get(blend_shape_idx);
		packed_blend_shape_array[blend_shape_idx] = single_blend_shape_array;
	}
	return packed_blend_shape_array;
}

int32_t TopologyDataImporter::generate_fake_format(const Array &arrays) const {
	ERR_FAIL_COND_V(arrays.size() != Mesh::ARRAY_MAX, 0);
	ERR_FAIL_COND_V(arrays[Mesh::ARRAY_VERTEX].get_type() != Variant::PACKED_VECTOR3_ARRAY, 0);
	int32_t format = Mesh::ARRAY_FORMAT_VERTEX;

	if (arrays[Mesh::ARRAY_NORMAL].get_type() == Variant::PACKED_VECTOR3_ARRAY) {
		format |= Mesh::ARRAY_FORMAT_NORMAL;
	}
	if (arrays[Mesh::ARRAY_TANGENT].get_type() == Variant::PACKED_FLOAT32_ARRAY || arrays[Mesh::ARRAY_TANGENT].get_type() == Variant::PACKED_FLOAT64_ARRAY) {
		format |= Mesh::ARRAY_FORMAT_TANGENT;
	}
	if (arrays[Mesh::ARRAY_COLOR].get_type() == Variant::PACKED_COLOR_ARRAY) {
		format |= Mesh::ARRAY_FORMAT_COLOR;
	}
	if (arrays[Mesh::ARRAY_TEX_UV].get_type() == Variant::PACKED_VECTOR2_ARRAY) {
		format |= Mesh::ARRAY_FORMAT_TEX_UV;
	}
	if (arrays[Mesh::ARRAY_TEX_UV2].get_type() == Variant::PACKED_VECTOR2_ARRAY) {
		format |= Mesh::ARRAY_FORMAT_TEX_UV2;
	}
	if (arrays[Mesh::ARRAY_BONES].get_type() == Variant::PACKED_INT32_ARRAY || arrays[Mesh::ARRAY_BONES].get_type() == Variant::PACKED_FLOAT32_ARRAY || arrays[Mesh::ARRAY_BONES].get_type() == Variant::PACKED_FLOAT64_ARRAY) {
		format |= Mesh::ARRAY_FORMAT_BONES;
	}
	if (arrays[Mesh::ARRAY_WEIGHTS].get_type() == Variant::PACKED_FLOAT32_ARRAY || arrays[Mesh::ARRAY_WEIGHTS].get_type() == Variant::PACKED_FLOAT64_ARRAY) {
		format |= Mesh::ARRAY_FORMAT_WEIGHTS;
	}
	if (arrays[Mesh::ARRAY_INDEX].get_type() == Variant::PACKED_INT32_ARRAY) {
		format |= Mesh::ARRAY_FORMAT_INDEX;
	}

	//check for custom stuff, probably could extract from flag, but this should work
	if ((format & Mesh::ARRAY_FORMAT_BONES) && (format & Mesh::ARRAY_FORMAT_WEIGHTS)) {
		const PackedVector3Array &vertex_array = arrays[Mesh::ARRAY_VERTEX];
		const Vector<real_t> &weights_array = arrays[Mesh::ARRAY_WEIGHTS];
		if (vertex_array.size() * 8 == weights_array.size()) {
			format |= Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS;
		}
	}

	return format;
}

MeshInstance3D *TopologyDataImporter::_replace_importer_mesh_instance_with_mesh_instance(ImporterMeshInstance3D *importer_mesh_instance) {
	MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
	Ref<ArrayMesh> array_mesh;
	array_mesh.instantiate();
	Ref<ImporterMesh> importer_mesh = importer_mesh_instance->get_mesh();
	array_mesh = importer_mesh->get_mesh(array_mesh);
	mesh_instance->set_skeleton_path(importer_mesh_instance->get_skeleton_path());
	if (!importer_mesh_instance->get_skin().is_null()) {
		mesh_instance->set_skin(importer_mesh_instance->get_skin());
	}
	mesh_instance->set_transform(importer_mesh_instance->get_transform());
	StringName mesh_instance_name = importer_mesh_instance->get_name();
	mesh_instance->set_mesh(array_mesh);
	importer_mesh_instance->replace_by(mesh_instance, false);
	mesh_instance->set_name(mesh_instance_name);
	return mesh_instance;
}
