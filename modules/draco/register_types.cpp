/*************************************************************************/
/*  register_types.cpp                                                   */
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

#include "register_types.h"
#include "core/error/error_macros.h"
#include "core/variant/array.h"
#include "draco/compression/decode.h"
#include "draco/core/status.h"
#include "scene/resources/mesh.h"
#include "scene/resources/mesh_data_tool.h"
#include "scene/resources/surface_tool.h"

#include "draco/compression/encode.h"
#include "draco/attributes/geometry_indices.h"
#include "draco/attributes/point_attribute.h"
#include "draco/compression/attributes/mesh_attribute_indices_encoding_data.h"
#include "draco/compression/mesh/mesh_edgebreaker_decoder.h"
#include "draco/core/draco_types.h"
#include "draco/mesh/triangle_soup_mesh_builder.h"
#include <stdint.h>
#include <memory>

float draco_attribute_quantization_func(ImporterMesh *p_importer_mesh, ImporterMesh *r_importer_mesh, int p_position_bits = 14, int p_normal_bits = 10, int p_uv_bits = 12, int p_other_attributes_bits = 32) {
	ERR_FAIL_NULL_V(p_importer_mesh, 0.0f);
	ERR_FAIL_NULL_V(r_importer_mesh, 0.0f);
	// Assumed deindexed
	draco::TriangleSoupMeshBuilder mesh_builder;
	Ref<MeshDataTool> mdt;
	mdt.instantiate();
	int32_t surface_count = p_importer_mesh->get_surface_count();

	const Ref<ArrayMesh> mesh = p_importer_mesh->get_mesh();
	for (int32_t surface_i = 0; surface_i < surface_count; surface_i++) {
		mdt->create_from_surface(mesh, surface_i);
		mesh_builder.Start(mdt->get_face_count());
		const int32_t pos_att_id = mesh_builder.AddAttribute(
				draco::GeometryAttribute::POSITION, 3, draco::DT_FLOAT32);
		const int32_t tex_att_id_0 = mesh_builder.AddAttribute(
				draco::GeometryAttribute::TEX_COORD, 2, draco::DT_FLOAT32);
		const int32_t tex_att_id_1 = mesh_builder.AddAttribute(
				draco::GeometryAttribute::TEX_COORD, 2, draco::DT_FLOAT32);
		const int32_t bone_id = mesh_builder.AddAttribute(
				draco::GeometryAttribute::GENERIC, 4, draco::DT_INT32); // TODO: fire 2022-01-24 Support 8 bones weights.
		const int32_t bone_weight_id = mesh_builder.AddAttribute(
				draco::GeometryAttribute::GENERIC, 4, draco::DT_FLOAT32); // TODO: fire 2022-01-24 Support 8 bones weights.
		for (int32_t face_i = 0; face_i < mdt->get_face_count(); face_i++) {
			int32_t vert_0 = mdt->get_face_vertex(face_i, 0);
			int32_t vert_1 = mdt->get_face_vertex(face_i, 1);
			int32_t vert_2 = mdt->get_face_vertex(face_i, 2);
			Vector3 pos_0 = mdt->get_vertex(vert_0);
			Vector3 pos_1 = mdt->get_vertex(vert_1);
			Vector3 pos_2 = mdt->get_vertex(vert_2);
			mesh_builder.SetAttributeValuesForFace(
					pos_att_id, draco::FaceIndex(face_i), &pos_0, &pos_1, &pos_2);
			Vector2 uv_0 = mdt->get_vertex_uv(vert_0);
			Vector2 uv_1 = mdt->get_vertex_uv(vert_1);
			Vector2 uv_2 = mdt->get_vertex_uv(vert_2);
			mesh_builder.SetAttributeValuesForFace(tex_att_id_0, draco::FaceIndex(face_i), &uv_0, &uv_1, &uv_2);
			Vector2 uv2_0 = mdt->get_vertex_uv2(vert_0);
			Vector2 uv2_1 = mdt->get_vertex_uv2(vert_1);
			Vector2 uv2_2 = mdt->get_vertex_uv2(vert_2);
			mesh_builder.SetAttributeValuesForFace(
					tex_att_id_1, draco::FaceIndex(face_i), &uv2_0, &uv2_1, &uv2_2);
			Vector<int> bones_0 = mdt->get_vertex_bones(vert_0);
			Vector<int> bones_1 = mdt->get_vertex_bones(vert_1);
			Vector<int> bones_2 = mdt->get_vertex_bones(vert_2);
			if (bones_0.size()) {
				mesh_builder.SetAttributeValuesForFace(
						bone_id, draco::FaceIndex(face_i), bones_0.ptrw(), bones_1.ptrw(), bones_2.ptrw());
			}
			Vector<float> weights_0 = mdt->get_vertex_weights(vert_0);
			Vector<float> weights_1 = mdt->get_vertex_weights(vert_1);
			Vector<float> weights_2 = mdt->get_vertex_weights(vert_2);
			if (weights_0.size()) {
				mesh_builder.SetAttributeValuesForFace(
						bone_weight_id, draco::FaceIndex(face_i), weights_0.ptrw(), weights_1.ptrw(), weights_2.ptrw());
			}
		}
		std::unique_ptr<draco::Mesh> draco_mesh = mesh_builder.Finalize();
		int32_t points = draco_mesh->num_faces() * 3;
		draco::Encoder encoder;
		encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, 14);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, 12);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::COLOR, 10);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, 10);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::GENERIC, 30);
		encoder.SetSpeedOptions(0, 5);
		draco::EncoderBuffer buffer;
		encoder.EncodeMeshToBuffer(*draco_mesh, &buffer);
		draco::DecoderBuffer in_buffer;
		in_buffer.Init(buffer.data(), buffer.size());
		draco::Decoder decoder;
		draco::DecoderOptions options;
		draco::StatusOr<std::unique_ptr<draco::Mesh>> status_or_mesh = decoder.DecodeMeshFromBuffer(&in_buffer);
		draco::Status status = status_or_mesh.status();
		ERR_CONTINUE_MSG(status.code() != draco::Status::OK, vformat("Error decoding draco buffer '%s' message %d\n", status.error_msg(), status.code()));		
		const std::unique_ptr<draco::Mesh> &output_draco_mesh = status_or_mesh.value();
		ERR_FAIL_COND_V(!output_draco_mesh, 0.0f);
		int32_t quantized_points = output_draco_mesh->num_faces() * 3;
		Array array;
		array.resize(ArrayMesh::ARRAY_MAX);
		const draco::PointAttribute *const pos_att = output_draco_mesh->GetAttributeByUniqueId(pos_att_id);
		ERR_CONTINUE(!pos_att);
		ERR_CONTINUE(pos_att->data_type() != draco::DataType::DT_FLOAT32);
		Vector<Vector3> pos_array;
		pos_array.resize(quantized_points);
		int32_t number_of_components = pos_att->num_components();
		ERR_CONTINUE(number_of_components != 3);
		for (draco::FaceIndex i(0); i < output_draco_mesh->num_faces(); ++i) {
			float pos_val[3];
			const draco::PointAttribute *point_attr = output_draco_mesh->GetAttributeByUniqueId(pos_att_id);
			ERR_BREAK(!point_attr);
			point_attr->GetMappedValue(output_draco_mesh->face(i)[0], pos_val);
			pos_array.write[i.value() * 3 + 0] = Vector3(pos_val[0], pos_val[1], pos_val[2]);
			point_attr->GetMappedValue(output_draco_mesh->face(i)[1], pos_val);
			pos_array.write[i.value() * 3 + 1] = Vector3(pos_val[0], pos_val[1], pos_val[2]);
			point_attr->GetMappedValue(output_draco_mesh->face(i)[2], pos_val);
			pos_array.write[i.value() * 3 + 2] = Vector3(pos_val[0], pos_val[1], pos_val[2]);
		}
		array[ArrayMesh::ARRAY_VERTEX] = pos_array;
		r_importer_mesh->add_surface(ArrayMesh::PRIMITIVE_TRIANGLES, array);
		return (float)points / quantized_points;
		// TODO: fire 2022-01-24T18:08:35-0800 Handle all surfaces
	}
	return 0.0f;
}

void register_draco_types() {
	SurfaceTool::attribute_quantization_func = draco_attribute_quantization_func;
}

void unregister_draco_types() {
	SurfaceTool::attribute_quantization_func = nullptr;
}
