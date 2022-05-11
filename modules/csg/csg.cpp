/**************************************************************************/
/*  csg.cpp                                                               */
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

#include "csg.h"

#include "core/error/error_macros.h"
#include "core/math/color.h"
#include "core/math/geometry_2d.h"
#include "core/math/math_funcs.h"
#include "core/math/plane.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/templates/sort_array.h"
#include "core/variant/variant.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/mesh_data_tool.h"
#include "scene/resources/surface_tool.h"

// CSGBrush

void CSGBrush::build_from_faces(const Vector<Vector3> &p_vertices, const Vector<Vector2> &p_uvs, const Vector<bool> &p_smooth, const Vector<Ref<Material>> &p_materials, const Vector<bool> &p_flip_faces) {
	faces.clear();

	int vc = p_vertices.size();

	ERR_FAIL_COND((vc % 3) != 0);

	const Vector3 *rv = p_vertices.ptr();
	int uvc = p_uvs.size();
	const Vector2 *ruv = p_uvs.ptr();
	int sc = p_smooth.size();
	const bool *rs = p_smooth.ptr();
	int mc = p_materials.size();
	const Ref<Material> *rm = p_materials.ptr();
	int ic = p_flip_faces.size();
	const bool *ri = p_flip_faces.ptr();

	HashMap<Ref<Material>, int> material_map;

	faces.resize(p_vertices.size() / 3);

	for (int i = 0; i < faces.size(); i++) {
		Face &f = faces.write[i];
		f.vertices[0] = rv[i * 3 + 0];
		f.vertices[1] = rv[i * 3 + 1];
		f.vertices[2] = rv[i * 3 + 2];

		if (uvc == vc) {
			f.uvs[0] = ruv[i * 3 + 0];
			f.uvs[1] = ruv[i * 3 + 1];
			f.uvs[2] = ruv[i * 3 + 2];
		}

		if (sc == vc / 3) {
			f.smooth = rs[i];
		} else {
			f.smooth = false;
		}

		if (ic == vc / 3) {
			f.invert = ri[i];
		} else {
			f.invert = false;
		}

		if (mc == vc / 3) {
			Ref<Material> mat = rm[i];
			if (mat.is_valid()) {
				HashMap<Ref<Material>, int>::ConstIterator E = material_map.find(mat);

				if (E) {
					f.material = E->value;
				} else {
					f.material = material_map.size();
					material_map[mat] = f.material;
				}

			} else {
				f.material = -1;
			}
		}
	}

	materials.resize(material_map.size());
	for (const KeyValue<Ref<Material>, int> &E : material_map) {
		materials.write[E.value] = E.key;
	}

	_regen_face_aabbs();
}

void CSGBrush::convert_manifold_to_brush() {
	manifold::Mesh mesh = manifold.GetMesh();
	size_t triangle_count = mesh.triVerts.size();
	faces.resize(triangle_count); // triVerts has one vector3 of indices
	for (size_t triangle_i = 0; triangle_i < triangle_count; triangle_i++) {
		CSGBrush::Face &face = faces.write[triangle_i];
		for (int32_t face_vertex_i = 0; face_vertex_i < 3; face_vertex_i++) {
			glm::ivec3 triangle_property_index = mesh.triVerts[triangle_i];
			constexpr int32_t order[3] = { 2, 1, 0 };
			size_t vertex_index = triangle_property_index[order[face_vertex_i]];
			glm::vec3 position = mesh.vertPos[vertex_index];
			face.vertices[face_vertex_i] = Vector3(position.x, position.y, position.z);
			glm::vec3 normal = mesh.vertNormal[vertex_index];
			bool flat = Math::is_equal_approx(normal.x, normal.y) && Math::is_equal_approx(normal.x, normal.z);
			face.smooth = !flat;
		}
		const manifold::MeshRelation &mesh_relation = manifold.GetMeshRelation();
		const manifold::BaryRef &bary_ref = mesh_relation.triBary[triangle_i];
		size_t original_id = bary_ref.originalID;
		if (!mesh_id_properties.has(original_id) || !mesh_id_triangle_property_indices.has(original_id)) {
			continue;
		}
		const std::vector<float> &vertex_properties = mesh_id_properties[original_id];
		const std::vector<glm::ivec3> &triangle_property_indices = mesh_id_triangle_property_indices[original_id];
		glm::ivec3 triangle_property_index = triangle_property_indices[bary_ref.tri];
		for (int32_t face_vertex_i = 0; face_vertex_i < 3; face_vertex_i++) {
			uint32_t original_face_index = triangle_property_index[face_vertex_i];
			face.invert = vertex_properties[original_face_index * MANIFOLD_MAX + MANIFOLD_PROPERTY_INVERT];
			face.smooth = vertex_properties[original_face_index * MANIFOLD_MAX + MANIFOLD_PROPERTY_SMOOTH_GROUP];
			for (int32_t face_index_i = 0; face_index_i < 3; face_index_i++) {
				constexpr int32_t order[3] = { 2, 1, 0 };
				face.uvs[order[face_index_i]].x = vertex_properties[original_face_index * MANIFOLD_MAX + MANIFOLD_PROPERTY_UV_X_0];
				face.uvs[order[face_index_i]].y = vertex_properties[original_face_index * MANIFOLD_MAX + MANIFOLD_PROPERTY_UV_X_0];
			}
		}
		if (!mesh_id_materials.has(original_id)) {
			continue;
		}
		if (unlikely((bary_ref.tri) < 0 || (bary_ref.tri) >= (mesh_id_materials[original_id].size()))) {
			continue;
		}
		Vector<Ref<Material>> triangle_materials = mesh_id_materials[original_id];
		Ref<Material> mat = triangle_materials[bary_ref.tri];
		int32_t mat_index = materials.find(mat);
		if (mat_index == -1) {
			materials.push_back(mat);
		}
		face.material = mat_index;
	}
	_regen_face_aabbs();
}

void CSGBrush::create_manifold() {
	Ref<SurfaceTool> st;
	st.instantiate();
	st->begin(Mesh::PRIMITIVE_TRIANGLES);
	for (int face_i = 0; face_i < faces.size(); face_i++) {
		const CSGBrush::Face &face = faces[face_i];
		for (int32_t vertex_i = 0; vertex_i < 3; vertex_i++) {
			st->set_smooth_group(face.smooth);
			int32_t mat_id = face.material;
			if (mat_id == -1 || mat_id >= materials.size()) {
				st->set_material(Ref<Material>());
			} else {
				st->set_material(materials[mat_id]);
			}
			st->add_vertex(face.vertices[vertex_i]);
		}
	}
	st->index();
	Ref<MeshDataTool> mdt;
	mdt.instantiate();
	mdt->create_from_surface(st->commit(), 0);
	std::vector<glm::ivec3> triangle_property_indices(mdt->get_face_count(), glm::vec3(-1, -1, -1));
	std::vector<float> vertex_properties(mdt->get_face_count() * MANIFOLD_MAX, NAN);
	Vector<Ref<Material>> triangle_material;
	triangle_material.resize(mdt->get_face_count());
	triangle_material.fill(Ref<Material>());
	manifold::Mesh mesh;
	mesh.triVerts.resize(mdt->get_face_count()); // triVerts has one vector3 of indices
	mesh.vertPos.resize(mdt->get_vertex_count());
	mesh.vertNormal.resize(mdt->get_vertex_count());
	mesh.halfedgeTangent.resize(mdt->get_face_count() * 3);
	HashMap<int32_t, Ref<Material>> vertex_material;
	for (int triangle_i = 0; triangle_i < mdt->get_face_count(); triangle_i++) {
		glm::ivec3 triangle_property_index;
		int32_t material_id = faces[triangle_i].material;
		for (int32_t face_index_i = 0; face_index_i < 3; face_index_i++) {
			size_t mesh_vertex = mdt->get_face_vertex(triangle_i, face_index_i);
			triangle_property_index[face_index_i] = mesh_vertex;
			{
				constexpr int32_t order[3] = { 2, 1, 0 };
				mesh.triVerts[triangle_i][order[face_index_i]] = mesh_vertex;
				Vector3 pos = mdt->get_vertex(mesh_vertex);
				mesh.vertPos[mesh_vertex] = glm::vec3(pos.x, pos.y, pos.z);
				Vector3 normal = -mdt->get_vertex_normal(mesh_vertex);
				normal.normalize();
				mesh.vertNormal[mesh_vertex] = glm::vec3(normal.x, normal.y, normal.z);
				Plane tangent = mdt->get_vertex_tangent(mesh_vertex);
				glm::vec4 glm_tangent = glm::vec4(tangent.normal.x, tangent.normal.y, tangent.normal.z, tangent.d);
				mesh.halfedgeTangent[mesh_vertex * order[face_index_i] + order[face_index_i]] = glm_tangent;
			}
			vertex_material[mesh_vertex] = mdt->get_material();
			vertex_properties[mesh_vertex * MANIFOLD_MAX + MANIFOLD_PROPERTY_INVERT] = faces[triangle_i].invert;
			vertex_properties[mesh_vertex * MANIFOLD_MAX + MANIFOLD_PROPERTY_PLACEHOLDER_MATERIAL] = material_id;
			vertex_properties[mesh_vertex * MANIFOLD_MAX + MANIFOLD_PROPERTY_SMOOTH_GROUP] = faces[triangle_i].smooth;
			vertex_properties[mesh_vertex * MANIFOLD_MAX + MANIFOLD_PROPERTY_UV_X_0 + face_index_i] = faces[triangle_i].uvs[face_index_i].x;
			vertex_properties[mesh_vertex * MANIFOLD_MAX + MANIFOLD_PROPERTY_UV_Y_0 + face_index_i] = faces[triangle_i].uvs[face_index_i].y;
		}
		triangle_property_indices[triangle_i] = triangle_property_index;
		if (unlikely((material_id) < 0 || (material_id) >= (materials.size()))) {
			continue;
		}
		triangle_material.write[triangle_i] = materials[faces[triangle_i].material];
	}
	manifold = manifold::Manifold(mesh, triangle_property_indices, vertex_properties);
	if (manifold.Status() != manifold::Manifold::Error::NO_ERROR) {
		print_line(vformat("Cannot copy the other brush. %d", int(manifold.Status())));
	}
	mesh_id_properties[manifold.OriginalID()] = vertex_properties;
	mesh_id_triangle_property_indices[manifold.OriginalID()] = triangle_property_indices;
	mesh_id_materials[manifold.OriginalID()] = triangle_material;
}

void CSGBrush::merge_manifold_properties(const HashMap<int64_t, std::vector<float>> &p_mesh_id_properties,
		const HashMap<int64_t, std::vector<glm::ivec3>> &p_mesh_id_triangle_property_indices,
		const HashMap<int64_t, Vector<Ref<Material>>> &p_mesh_id_materials,
		HashMap<int64_t, std::vector<float>> &r_mesh_id_properties,
		HashMap<int64_t, std::vector<glm::ivec3>> &r_mesh_id_triangle_property_indices,
		HashMap<int64_t, Vector<Ref<Material>>> &r_mesh_id_materials) {
	for (const KeyValue<int64_t, std::vector<float>> &E : p_mesh_id_properties) {
		r_mesh_id_properties.operator[](E.key) = E.value;
	}
	for (const KeyValue<int64_t, std::vector<glm::ivec3>> &E : p_mesh_id_triangle_property_indices) {
		r_mesh_id_triangle_property_indices.operator[](E.key) = E.value;
	}
	for (const KeyValue<int64_t, Vector<Ref<Material>>> &E : p_mesh_id_materials) {
		r_mesh_id_materials.operator[](E.key) = E.value;
	}
}
