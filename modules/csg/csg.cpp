/*************************************************************************/
/*  csg.cpp                                                              */
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

// Static helper functions.

inline static bool is_snapable(const Vector3 &p_point1, const Vector3 &p_point2, real_t p_distance) {
	return p_point2.distance_squared_to(p_point1) < p_distance * p_distance;
}

inline static Vector2 interpolate_segment_uv(const Vector2 p_segment_points[2], const Vector2 p_uvs[2], const Vector2 &p_interpolation_point) {
	if (p_segment_points[0].is_equal_approx(p_segment_points[1])) {
		return p_uvs[0];
	}

	float segment_length = p_segment_points[0].distance_to(p_segment_points[1]);
	float distance = p_segment_points[0].distance_to(p_interpolation_point);
	float fraction = distance / segment_length;

	return p_uvs[0].lerp(p_uvs[1], fraction);
}

inline static Vector2 interpolate_triangle_uv(const Vector2 p_vertices[3], const Vector2 p_uvs[3], const Vector2 &p_interpolation_point) {
	if (p_interpolation_point.is_equal_approx(p_vertices[0])) {
		return p_uvs[0];
	}
	if (p_interpolation_point.is_equal_approx(p_vertices[1])) {
		return p_uvs[1];
	}
	if (p_interpolation_point.is_equal_approx(p_vertices[2])) {
		return p_uvs[2];
	}

	Vector2 edge1 = p_vertices[1] - p_vertices[0];
	Vector2 edge2 = p_vertices[2] - p_vertices[0];
	Vector2 interpolation = p_interpolation_point - p_vertices[0];

	float edge1_on_edge1 = edge1.dot(edge1);
	float edge1_on_edge2 = edge1.dot(edge2);
	float edge2_on_edge2 = edge2.dot(edge2);
	float inter_on_edge1 = interpolation.dot(edge1);
	float inter_on_edge2 = interpolation.dot(edge2);
	float scale = (edge1_on_edge1 * edge2_on_edge2 - edge1_on_edge2 * edge1_on_edge2);
	if (scale == 0) {
		return p_uvs[0];
	}

	float v = (edge2_on_edge2 * inter_on_edge1 - edge1_on_edge2 * inter_on_edge2) / scale;
	float w = (edge1_on_edge1 * inter_on_edge2 - edge1_on_edge2 * inter_on_edge1) / scale;
	float u = 1.0f - v - w;

	return p_uvs[0] * u + p_uvs[1] * v + p_uvs[2] * w;
}

static inline bool ray_intersects_triangle(const Vector3 &p_from, const Vector3 &p_dir, const Vector3 p_vertices[3], float p_tolerance, Vector3 &r_intersection_point) {
	Vector3 edge1 = p_vertices[1] - p_vertices[0];
	Vector3 edge2 = p_vertices[2] - p_vertices[0];
	Vector3 h = p_dir.cross(edge2);
	real_t a = edge1.dot(h);
	// Check if ray is parallel to triangle.
	if (Math::is_zero_approx(a)) {
		return false;
	}
	real_t f = 1.0 / a;

	Vector3 s = p_from - p_vertices[0];
	real_t u = f * s.dot(h);
	if (u < 0.0 - p_tolerance || u > 1.0 + p_tolerance) {
		return false;
	}

	Vector3 q = s.cross(edge1);
	real_t v = f * p_dir.dot(q);
	if (v < 0.0 - p_tolerance || u + v > 1.0 + p_tolerance) {
		return false;
	}

	// Ray intersects triangle.
	// Calculate distance.
	real_t t = f * edge2.dot(q);
	// Confirm triangle is in front of ray.
	if (t >= p_tolerance) {
		r_intersection_point = p_from + p_dir * t;
		return true;
	} else {
		return false;
	}
}

inline bool is_point_in_triangle(const Vector3 &p_point, const Vector3 p_vertices[3], int p_shifted = 0) {
	real_t det = p_vertices[0].dot(p_vertices[1].cross(p_vertices[2]));

	// If determinant is, zero try shift the triangle and the point.
	if (Math::is_zero_approx(det)) {
		if (p_shifted > 2) {
			// Triangle appears degenerate, so ignore it.
			return false;
		}
		Vector3 shift_by;
		shift_by[p_shifted] = 1;
		Vector3 shifted_point = p_point + shift_by;
		Vector3 shifted_vertices[3] = { p_vertices[0] + shift_by, p_vertices[1] + shift_by, p_vertices[2] + shift_by };
		return is_point_in_triangle(shifted_point, shifted_vertices, p_shifted + 1);
	}

	// Find the barycentric coordinates of the point with respect to the vertices.
	real_t lambda[3];
	lambda[0] = p_vertices[1].cross(p_vertices[2]).dot(p_point) / det;
	lambda[1] = p_vertices[2].cross(p_vertices[0]).dot(p_point) / det;
	lambda[2] = p_vertices[0].cross(p_vertices[1]).dot(p_point) / det;

	// Point is in the plane if all lambdas sum to 1.
	if (!Math::is_equal_approx(lambda[0] + lambda[1] + lambda[2], 1)) {
		return false;
	}

	// Point is inside the triangle if all lambdas are positive.
	if (lambda[0] < 0 || lambda[1] < 0 || lambda[2] < 0) {
		return false;
	}

	return true;
}

inline static bool is_triangle_degenerate(const Vector2 p_vertices[3], real_t p_vertex_snap2) {
	real_t det = p_vertices[0].x * p_vertices[1].y - p_vertices[0].x * p_vertices[2].y +
			p_vertices[0].y * p_vertices[2].x - p_vertices[0].y * p_vertices[1].x +
			p_vertices[1].x * p_vertices[2].y - p_vertices[1].y * p_vertices[2].x;

	return det < p_vertex_snap2;
}

inline static bool are_segments_parallel(const Vector2 p_segment1_points[2], const Vector2 p_segment2_points[2], float p_vertex_snap2) {
	Vector2 segment1 = p_segment1_points[1] - p_segment1_points[0];
	Vector2 segment2 = p_segment2_points[1] - p_segment2_points[0];
	real_t segment1_length2 = segment1.dot(segment1);
	real_t segment2_length2 = segment2.dot(segment2);
	real_t segment_onto_segment = segment2.dot(segment1);

	if (segment1_length2 < p_vertex_snap2 || segment2_length2 < p_vertex_snap2) {
		return true;
	}

	real_t max_separation2;
	if (segment1_length2 > segment2_length2) {
		max_separation2 = segment2_length2 - segment_onto_segment * segment_onto_segment / segment1_length2;
	} else {
		max_separation2 = segment1_length2 - segment_onto_segment * segment_onto_segment / segment2_length2;
	}

	return max_separation2 < p_vertex_snap2;
}

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

void CSGBrush::copy_from(const CSGBrush &p_brush, const Transform3D &p_xform) {
	faces = p_brush.faces;
	materials = p_brush.materials;
	manifold = p_brush.manifold;
	for (int i = 0; i < faces.size(); i++) {
		for (int j = 0; j < 3; j++) {
			faces.write[i].vertices[j] = p_xform.xform(p_brush.faces[i].vertices[j]);
		}
	}

	_regen_face_aabbs();
	create_manifold();
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
	std::vector<float> vertex_property_tolerance(MANIFOLD_MAX, 1e-3);
	std::vector<float> vertex_properties(mdt->get_face_count() * MANIFOLD_MAX, NAN);
	Vector<Ref<Material>> triangle_material;
	triangle_material.resize(mdt->get_face_count());
	triangle_material.fill(Ref<Material>());
	manifold::Mesh mesh;
	mesh.triVerts.resize(mdt->get_face_count()); // triVerts has one vector3 of indices
	mesh.vertPos.resize(mdt->get_vertex_count());
	mesh.vertNormal.resize(mdt->get_vertex_count());
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
				Vector3 normal = mdt->get_vertex_normal(mesh_vertex);
				normal = -normal;
				mesh.vertNormal[mesh_vertex] = glm::vec3(normal.x, normal.y, normal.z);
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
	manifold = manifold::Manifold(mesh, triangle_property_indices, vertex_properties, vertex_property_tolerance);
	if (manifold.Status() != manifold::Manifold::Error::NO_ERROR) {
		print_line(vformat("Cannot copy from the other brush. %d", int(manifold.Status())));
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
