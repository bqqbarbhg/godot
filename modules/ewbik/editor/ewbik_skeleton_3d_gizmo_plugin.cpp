/*************************************************************************/
/*  ewbik_skeleton_3d_gizmo_plugin.cpp                                   */
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

#include "ewbik_skeleton_3d_gizmo_plugin.h"

#include "core/io/resource_saver.h"
#include "core/math/transform_3d.h"
#include "editor/editor_file_dialog.h"
#include "editor/editor_node.h"
#include "editor/editor_properties.h"
#include "editor/editor_scale.h"
#include "editor/plugins/animation_player_editor_plugin.h"
#include "editor/plugins/node_3d_editor_gizmos.h"
#include "editor/plugins/node_3d_editor_plugin.h"
#include "scene/3d/collision_shape_3d.h"
#include "scene/3d/joint_3d.h"
#include "scene/3d/label_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics_body_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/capsule_shape_3d.h"
#include "scene/resources/primitive_meshes.h"
#include "scene/resources/sphere_shape_3d.h"
#include "scene/resources/surface_tool.h"
#include "scene/scene_string_names.h"

#include "../src/ik_kusudama.h"

bool EWBIK3DGizmoPlugin::has_gizmo(Node3D *p_spatial) {
	return cast_to<Skeleton3D>(p_spatial);
}

String EWBIK3DGizmoPlugin::get_gizmo_name() const {
	return "EWBIK Constraints";
}

void EWBIK3DGizmoPlugin::redraw(EditorNode3DGizmo *p_gizmo) {
	if (!p_gizmo) {
		return;
	}
	Node3D *node_3d = p_gizmo->get_node_3d();
	if (!node_3d) {
		return;
	}
	Node *owner_node = node_3d->get_owner();
	if (!owner_node) {
		return;
	}
	TypedArray<Node> nodes = owner_node->find_children("*", "SkeletonModification3DNBoneIK");
	p_gizmo->clear();
	for (int32_t node_i = 0; node_i < nodes.size(); node_i++) {
		SkeletonModification3DNBoneIK *ewbik = cast_to<SkeletonModification3DNBoneIK>(nodes[node_i]);
		if (!ewbik) {
			continue;
		}
		Skeleton3D *ewbik_skeleton = ewbik->get_skeleton();
		if (!ewbik_skeleton) {
			continue;
		}
		if (cast_to<Skeleton3D>(node_3d) != ewbik_skeleton) {
			continue;
		}
		LocalVector<int> bones;
		LocalVector<float> weights;
		bones.resize(4);
		weights.resize(4);
		for (int i = 0; i < 4; i++) {
			bones[i] = 0;
			weights[i] = 0;
		}
		weights[0] = 1;
		Vector<int> bones_to_process = ewbik_skeleton->get_parentless_bones();
		Ref<Shader> kusudama_shader;
		kusudama_shader.instantiate();
		kusudama_shader->set_code(R"(
// Skeleton 3D gizmo kusudama constraint shader.
shader_type spatial;
render_mode depth_prepass_alpha, cull_disabled;

uniform vec4 kusudama_color : source_color = vec4(0.58039218187332, 0.27058824896812, 0.00784313771874, 1.0);
uniform int cone_count = 0;

// 0,0,0 is the center of the kusudama. The kusudamas have their own bases that automatically get reoriented such that +y points in the direction that is the weighted average of the limitcones on the kusudama.
// But, if you have a kusuduma with just 1 limit_cone, then in general that limit_cone should be 0,1,0 in the kusudama's basis unless the user has specifically specified otherwise.

uniform vec4 cone_sequence[30];

// This shader can display up to 30 cones (represented by 30 4d vectors)
// Each group of 4 represents the xyz coordinates of the cone direction
// vector in model space and the fourth element represents radius

// TODO: fire 2022-05-26
// Use a texture to store bone parameters.
// Use the uv to get the row of the bone.

varying vec3 normal_model_dir;
varying vec4 vert_model_color;

bool is_in_inter_cone_path(in vec3 normal_dir, in vec4 tangent_1, in vec4 cone_1, in vec4 tangent_2, in vec4 cone_2) {
	vec3 c1xc2 = cross(cone_1.xyz, cone_2.xyz);
	float c1c2dir = dot(normal_dir, c1xc2);

	if (c1c2dir < 0.0) {
		vec3 c1xt1 = cross(cone_1.xyz, tangent_1.xyz);
		vec3 t1xc2 = cross(tangent_1.xyz, cone_2.xyz);
		float c1t1dir = dot(normal_dir, c1xt1);
		float t1c2dir = dot(normal_dir, t1xc2);

	 	return (c1t1dir > 0.0 && t1c2dir > 0.0);

	} else {
		vec3 t2xc1 = cross(tangent_2.xyz, cone_1.xyz);
		vec3 c2xt2 = cross(cone_2.xyz, tangent_2.xyz);
		float t2c1dir = dot(normal_dir, t2xc1);
		float c2t2dir = dot(normal_dir, c2xt2);

		return (c2t2dir > 0.0 && t2c1dir > 0.0);
	}
	return false;
}

//determines the current draw condition based on the desired draw condition in the setToArgument
// -3 = disallowed entirely;
// -2 = disallowed and on tangent_cone boundary
// -1 = disallowed and on control_cone boundary
// 0 =  allowed and empty;
// 1 =  allowed and on control_cone boundary
// 2  = allowed and on tangent_cone boundary
int get_allowability_condition(in int current_condition, in int set_to) {
	if((current_condition == -1 || current_condition == -2)
		&& set_to >= 0) {
		return current_condition *= -1;
	} else if(current_condition == 0 && (set_to == -1 || set_to == -2)) {
		return set_to *=-2;
	}
	return max(current_condition, set_to);
}

// returns 1 if normal_dir is beyond (cone.a) radians from the cone.rgb
// returns 0 if normal_dir is within (cone.a + boundary_width) radians from the cone.rgb
// return -1 if normal_dir is less than (cone.a) radians from the cone.rgb
int is_in_cone(in vec3 normal_dir, in vec4 cone, in float boundary_width) {
	float arc_dist_to_cone = acos(dot(normal_dir, cone.rgb));
	if (arc_dist_to_cone > (cone.a+(boundary_width/2.))) {
		return 1;
	}
	if (arc_dist_to_cone < (cone.a-(boundary_width/2.))) {
		return -1;
	}
	return 0;
}

// Returns a color corresponding to the allowability of this region,
// or otherwise the boundaries corresponding
// to various cones and tangent_cone.
vec4 color_allowed(in vec3 normal_dir,  in int cone_counts, in float boundary_width) {
	int current_condition = -3;
	if (cone_counts == 1) {
		vec4 cone = cone_sequence[0];
		int in_cone = is_in_cone(normal_dir, cone, boundary_width);
		bool is_in_cone = in_cone == 0;
		if (is_in_cone) {
			in_cone = -1;
		} else {
			if (in_cone < 0) {
				in_cone = 0;
			} else {
				in_cone = -3;
			}
		}
		current_condition = get_allowability_condition(current_condition, in_cone);
	} else {
		for(int i=0; i < (cone_counts-1)*3; i=i+3) {
			normal_dir = normalize(normal_dir);

			vec4 cone_1 = cone_sequence[i+0];
			vec4 tangent_1 = cone_sequence[i+1];
			vec4 tangent_2 = cone_sequence[i+2];
			vec4 cone_2 = cone_sequence[i+3];

			int inCone1 = is_in_cone(normal_dir, cone_1, boundary_width);
			if (inCone1 == 0) {
				inCone1 = -1;
			} else {
				if (inCone1 < 0) {
					inCone1 = 0;
				} else {
					inCone1 = -3;
				}
			}
			current_condition = get_allowability_condition(current_condition, inCone1);

			int inCone2 = is_in_cone(normal_dir, cone_2, boundary_width);
			if (inCone2 == 0) {
				inCone2 = -1;
			} else {
				if (inCone2 < 0) {
					inCone2 = 0;
				} else {
					inCone2 = -3;
				}
			}
			current_condition = get_allowability_condition(current_condition, inCone2);

			int in_tan_1 = is_in_cone(normal_dir, tangent_1, boundary_width);
			int in_tan_2 = is_in_cone(normal_dir, tangent_2, boundary_width);

			if (float(in_tan_1) < 1. || float(in_tan_2) < 1.) {
				in_tan_1 = in_tan_1 == 0 ? -2 : -3;
				current_condition = get_allowability_condition(current_condition, in_tan_1);
				in_tan_2 = in_tan_2 == 0 ? -2 : -3;
				current_condition = get_allowability_condition(current_condition, in_tan_2);
			} else {
				bool in_intercone = is_in_inter_cone_path(normal_dir, tangent_1, cone_1, tangent_2, cone_2);
				int intercone_condition = in_intercone ? 0 : -3;
				current_condition = get_allowability_condition(current_condition, intercone_condition);
			}
		}
	}
	vec4 result = vert_model_color;
	if (current_condition != 0 && current_condition != 2) {
		float on_cone_boundary = current_condition == 1 ? -0.3 : 0.0;
		result += vec4(0.0, on_cone_boundary, 0, 0.0);
	} else {
		return vec4(0.0, 0.0, 0.0, 0.0);
	}
	return result;
}

void vertex() {
	normal_model_dir = CUSTOM0.rgb;
	vert_model_color.rgb = kusudama_color.rgb;
}

void fragment() {
	vec4 result_color_allowed = vec4(0.0, 0.0, 0.0, 0.0);
	if (cone_sequence.length() == 30) {
		result_color_allowed = color_allowed(normal_model_dir, cone_count, 0.02);
	}
	ALBEDO = kusudama_color.rgb;
	if (result_color_allowed.a == 0.0) {
		discard;
	}
	ALBEDO = result_color_allowed.rgb;
	ALPHA = result_color_allowed.a;
}
)");
		int bones_to_process_i = 0;
		Vector<Vector3> handles;
		while (bones_to_process_i < bones_to_process.size()) {
			int current_bone_idx = bones_to_process[bones_to_process_i];
			BoneId parent_idx = ewbik_skeleton->get_bone_parent(current_bone_idx);
			bones[0] = parent_idx;
			bones_to_process_i++;
			Color current_bone_color = bone_color;
			Ref<IKBoneSegment> bone_segment = ewbik->get_segmented_skeleton();
			if (bone_segment.is_null()) {
				continue;
			}
			Ref<IKBone3D> ik_bone = bone_segment->get_ik_bone(current_bone_idx);
			if (ik_bone.is_null() || ik_bone->get_bone_id() != current_bone_idx) {
				continue;
			}
			Vector<int> child_bones_vector = ewbik_skeleton->get_bone_children(current_bone_idx);
			for (int child_bone_idx : child_bones_vector) {
				String bone_name = ewbik_skeleton->get_bone_name(current_bone_idx);
				// Add the bone's children to the list of bones to be processed.
				bones_to_process.push_back(child_bone_idx);
			}
			Ref<IKKusudama> ik_kusudama = ik_bone->get_constraint();
			if (ik_kusudama.is_null()) {
				continue;
			}
			Transform3D constraint_relative_to_the_skeleton = ewbik_skeleton->get_transform().affine_inverse() * ik_bone->get_constraint_transform()->get_global_transform();
			PackedFloat32Array kusudama_limit_cones;
			Ref<IKKusudama> kusudama = ik_bone->get_constraint();
			kusudama_limit_cones.resize(KUSUDAMA_MAX_CONES * 4);
			kusudama_limit_cones.fill(0.0f);
			int out_idx = 0;
			const TypedArray<IKLimitCone> &limit_cones = ik_kusudama->get_limit_cones();
			for (int32_t cone_i = 0; cone_i < limit_cones.size(); cone_i++) {
				Ref<IKLimitCone> limit_cone = limit_cones[cone_i];
				Vector3 control_point = limit_cone->get_control_point();
				kusudama_limit_cones.write[out_idx + 0] = control_point.x;
				kusudama_limit_cones.write[out_idx + 1] = control_point.y;
				kusudama_limit_cones.write[out_idx + 2] = control_point.z;
				float radius = limit_cone->get_radius();
				kusudama_limit_cones.write[out_idx + 3] = radius;
				out_idx += 4;

				Vector3 tangent_center_1 = limit_cone->get_tangent_circle_center_next_1();
				kusudama_limit_cones.write[out_idx + 0] = tangent_center_1.x;
				kusudama_limit_cones.write[out_idx + 1] = tangent_center_1.y;
				kusudama_limit_cones.write[out_idx + 2] = tangent_center_1.z;
				float tangent_radius = limit_cone->get_tangent_circle_radius_next();
				kusudama_limit_cones.write[out_idx + 3] = tangent_radius;
				out_idx += 4;

				Vector3 tangent_center_2 = limit_cone->get_tangent_circle_center_next_2();
				kusudama_limit_cones.write[out_idx + 0] = tangent_center_2.x;
				kusudama_limit_cones.write[out_idx + 1] = tangent_center_2.y;
				kusudama_limit_cones.write[out_idx + 2] = tangent_center_2.z;
				kusudama_limit_cones.write[out_idx + 3] = tangent_radius;
				out_idx += 4;
			}
			Vector3 v0 = ewbik_skeleton->get_bone_global_rest(current_bone_idx).origin;
			Vector3 v1 = ewbik_skeleton->get_bone_global_rest(parent_idx).origin;
			real_t dist = v0.distance_to(v1);
			float radius = dist / 5.0;
			{ // BEGIN Create a kusudama ball visualization.
				// Code copied from the SphereMesh.
				float height = dist / 2.5;
				int rings = 64;

				int i, j, prevrow, thisrow, point;
				float x, y, z;

				float scale = height * 0.5;

				Vector<Vector3> points;
				Vector<Vector3> normals;
				Vector<int> indices;
				point = 0;

				thisrow = 0;
				prevrow = 0;
				for (j = 0; j <= (rings + 1); j++) {
					int radial_segments = 32;
					float v = j;
					float w;

					v /= (rings + 1);
					w = sin(Math_PI * v);
					y = scale * cos(Math_PI * v);

					for (i = 0; i <= radial_segments; i++) {
						float u = i;
						u /= radial_segments;

						x = sin(u * Math_TAU);
						z = cos(u * Math_TAU);

						Vector3 p = Vector3(x * radius * w, y, z * radius * w);
						points.push_back(p);
						Vector3 normal = Vector3(x * w * scale, radius * (y / scale), z * w * scale);
						normals.push_back(normal.normalized());
						point++;

						if (i > 0 && j > 0) {
							indices.push_back(prevrow + i - 1);
							indices.push_back(prevrow + i);
							indices.push_back(thisrow + i - 1);

							indices.push_back(prevrow + i);
							indices.push_back(thisrow + i);
							indices.push_back(thisrow + i - 1);
						};
					};

					prevrow = thisrow;
					thisrow = point;
				}
				Ref<SurfaceTool> kusudama_surface_tool;
				kusudama_surface_tool.instantiate();
				kusudama_surface_tool->begin(Mesh::PRIMITIVE_TRIANGLES);
				const int32_t MESH_CUSTOM_0 = 0;
				kusudama_surface_tool->set_custom_format(MESH_CUSTOM_0, SurfaceTool::CustomFormat::CUSTOM_RGBA_HALF);
				for (int32_t point_i = 0; point_i < points.size(); point_i++) {
					kusudama_surface_tool->set_bones(bones);
					kusudama_surface_tool->set_weights(weights);
					Color c;
					c.r = normals[point_i].x;
					c.g = normals[point_i].y;
					c.b = normals[point_i].z;
					c.a = 0;
					kusudama_surface_tool->set_custom(MESH_CUSTOM_0, c);
					kusudama_surface_tool->set_normal(normals[point_i]);
					kusudama_surface_tool->add_vertex(points[point_i]);
				}
				for (int32_t index_i : indices) {
					kusudama_surface_tool->add_index(index_i);
				}
				Ref<ShaderMaterial> kusudama_material;
				kusudama_material.instantiate();
				kusudama_material->set_shader(kusudama_shader);
				kusudama_material->set_shader_parameter("cone_sequence", kusudama_limit_cones);
				int32_t cone_count = kusudama->get_limit_cones().size();
				kusudama_material->set_shader_parameter("cone_count", cone_count);
				kusudama_material->set_shader_parameter("kusudama_color", current_bone_color);
				p_gizmo->add_mesh(kusudama_surface_tool->commit(Ref<Mesh>(), RS::ARRAY_CUSTOM_RGBA_HALF << RS::ARRAY_FORMAT_CUSTOM0_SHIFT), kusudama_material, constraint_relative_to_the_skeleton, ewbik_skeleton->register_skin(ewbik_skeleton->create_skin_from_rest_transforms()));
				// END Create a kusudama ball visualization.
			}
			{
				// START Create a cone visualization.
				Ref<SurfaceTool> cone_sides_surface_tool;
				cone_sides_surface_tool.instantiate();
				cone_sides_surface_tool->begin(Mesh::PRIMITIVE_LINES);
				// Make the gizmo color as bright as possible for better visibility
				Color color = bone_color;
				color.set_ok_hsl(color.get_h(), color.get_s(), 1);

				const Ref<Material> material_primary = get_material("lines_primary", p_gizmo);
				const Ref<Material> material_secondary = get_material("lines_secondary", p_gizmo);
				const Ref<StandardMaterial3D> material_tertiary = get_material("lines_tertiary", p_gizmo);
				Basis mesh_orientation = Basis::from_euler(Vector3(Math::deg_to_rad(90.0f), 0, 0));
				for (int32_t cone_i = 0; cone_i < kusudama_limit_cones.size(); cone_i = cone_i + (3 * 4)) {
					Vector3 center = Vector3(kusudama_limit_cones[cone_i + 0], kusudama_limit_cones[cone_i + 1], kusudama_limit_cones[cone_i + 2]);
					Transform3D center_relative_to_mesh = Transform3D(Quaternion(Vector3(0, 1, 0), center)) * mesh_orientation;
					{
						Transform3D handle_relative_to_mesh;
						handle_relative_to_mesh.origin = center; // * radius;
						Transform3D handle_relative_to_skeleton = constraint_relative_to_the_skeleton * handle_relative_to_mesh;
						Transform3D handle_relative_to_universe = ewbik_skeleton->get_global_transform() * handle_relative_to_skeleton;
						handles.push_back(handle_relative_to_universe.origin);
					}
					Vector<Vector3> points_primary;
					Vector<Vector3> points_secondary;

					float r = radius;
					float cone_radius = kusudama_limit_cones[cone_i + 3];
					float w = r * Math::sin(cone_radius);
					float d = r * Math::cos(cone_radius);

					for (int circle_i = 0; circle_i < 120; circle_i++) {
						// Draw a circle
						const float ra = Math::deg_to_rad((float)(circle_i * 3));
						const Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * w;
						if (circle_i == 0) {
							Transform3D handle_border_relative_to_mesh;
							handle_border_relative_to_mesh.origin = center_relative_to_mesh.xform(Vector3(a.x, a.y, -d));
							Transform3D handle_border_relative_to_skeleton = constraint_relative_to_the_skeleton * handle_border_relative_to_mesh;
							Transform3D handle_border_relative_to_universe = ewbik_skeleton->get_global_transform() * handle_border_relative_to_skeleton;
							handles.push_back(handle_border_relative_to_universe.origin);
						}
						if (circle_i % 15 == 0) {
							// Draw 8 lines from the cone origin to the sides of the circle
							cone_sides_surface_tool->set_bones(bones);
							cone_sides_surface_tool->set_weights(weights);
							cone_sides_surface_tool->add_vertex((center_relative_to_mesh).xform(Vector3(a.x, a.y, -d)));
							cone_sides_surface_tool->set_bones(bones);
							cone_sides_surface_tool->set_weights(weights);
							cone_sides_surface_tool->add_vertex((center_relative_to_mesh).xform(Vector3()));
						}
					}
				}
				p_gizmo->add_mesh(cone_sides_surface_tool->commit(), material_tertiary, constraint_relative_to_the_skeleton, ewbik_skeleton->register_skin(ewbik_skeleton->create_skin_from_rest_transforms()));
			} // END cone
			// TODO: Use several colors for the dots and match the color of the lines.
			p_gizmo->add_handles(handles, get_material("handles"), Vector<int>(), true, true);
		}
	}
}
