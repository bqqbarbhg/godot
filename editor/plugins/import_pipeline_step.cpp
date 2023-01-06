/**************************************************************************/
/*  import_pipeline_step.cpp                                              */
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

#include "import_pipeline_step.h"

#include "core/error/error_macros.h"
#include "core/io/resource_saver.h"
#include "editor/editor_node.h"
#include "scene/3d/area_3d.h"
#include "scene/3d/collision_shape_3d.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/navigation_region_3d.h"
#include "scene/3d/occluder_instance_3d.h"
#include "scene/3d/physics_body_3d.h"
#include "scene/3d/vehicle_body_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/animation.h"
#include "scene/resources/box_shape_3d.h"
#include "scene/resources/importer_mesh.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/resource_format_text.h"
#include "scene/resources/separation_ray_shape_3d.h"
#include "scene/resources/sphere_shape_3d.h"
#include "scene/resources/surface_tool.h"
#include "scene/resources/world_boundary_shape_3d.h"

void ImportPipelineStep::_bind_methods() {
	GDVIRTUAL_BIND(_update);
	GDVIRTUAL_BIND(_source_changed);
	GDVIRTUAL_BIND(_get_display_name);
	GDVIRTUAL_BIND(_get_inputs);
	GDVIRTUAL_BIND(_get_outputs);
}

void ImportPipelineStep::update() {
	GDVIRTUAL_CALL(_update);
}

String ImportPipelineStep::get_display_name() {
	String ret;
	if (GDVIRTUAL_CALL(_get_display_name, ret)) {
		return ret;
	}
	return "Empty Step";
}

void ImportPipelineStep::source_changed() {
	GDVIRTUAL_CALL(_source_changed);
}

PackedStringArray ImportPipelineStep::get_inputs() {
	PackedStringArray ret;
	if (GDVIRTUAL_CALL(_get_inputs, ret)) {
		return ret;
	}
	return PackedStringArray();
}

PackedStringArray ImportPipelineStep::get_outputs() {
	PackedStringArray ret;
	if (GDVIRTUAL_CALL(_get_outputs, ret)) {
		return ret;
	}
	return PackedStringArray();
}

ImportPipelineStep::ImportPipelineStep() {
}

///////////////////////////////////////

void ImportPipelineStepRemoveNodes::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_source"), &ImportPipelineStepRemoveNodes::get_source);
	ClassDB::bind_method(D_METHOD("set_source", "scene"), &ImportPipelineStepRemoveNodes::set_source);
	ClassDB::bind_method(D_METHOD("get_result"), &ImportPipelineStepRemoveNodes::get_result);
	ClassDB::bind_method(D_METHOD("set_result", "scene"), &ImportPipelineStepRemoveNodes::set_result);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source", PropertyHint::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_source", "get_source");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "result", PropertyHint::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_result", "get_result");
}

bool ImportPipelineStepRemoveNodes::_set(const StringName &p_name, const Variant &p_value) {
	if (!selected.has(p_name)) {
		if (p_value.get_type() == Variant::BOOL) {
			maybe_selected[p_name] = p_value;
		}
		return false;
	}
	selected[p_name] = p_value;
	return true;
}

bool ImportPipelineStepRemoveNodes::_get(const StringName &p_name, Variant &r_ret) const {
	if (!selected.has(p_name)) {
		return false;
	}
	r_ret = selected[p_name];
	return true;
}

void ImportPipelineStepRemoveNodes::_get_property_list(List<PropertyInfo> *p_list) const {
	for (PropertyInfo info : properties) {
		p_list->push_back(info);
	}
}

bool ImportPipelineStepRemoveNodes::_property_can_revert(const StringName &p_name) const {
	return selected.has(p_name);
}

bool ImportPipelineStepRemoveNodes::_property_get_revert(const StringName &p_name, Variant &r_property) const {
	if (!selected.has(p_name)) {
		return false;
	}
	r_property = false;
	return true;
}

void ImportPipelineStepRemoveNodes::_iterate_remove(Node *p_node, const String &p_prefix) {
	String name = p_prefix + p_node->get_name();
	if (selected.has(name) && selected[name]) {
		p_node->set_owner(nullptr);
		return;
	}
	name += "/";
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_iterate_remove(p_node->get_child(i), name);
	}
}

void ImportPipelineStepRemoveNodes::_iterate_properties(Node *p_node, const String &p_prefix) {
	String name = p_prefix + p_node->get_name();
	properties.push_back(PropertyInfo(Variant::BOOL, name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT));
	if (!selected.has(name)) {
		if (maybe_selected.has(name)) {
			selected[name] = maybe_selected[name];
		} else {
			selected[name] = false;
		}
	}
	name += "/";
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_iterate_properties(p_node->get_child(i), name);
	}
}

void ImportPipelineStepRemoveNodes::source_changed() {
	properties.clear();
	if (!source.is_valid()) {
		print_line("source invalid");
		return;
	}
	Node *scene = source->instantiate();
	for (int i = 0; i < scene->get_child_count(); i++) {
		_iterate_properties(scene->get_child(i), "");
	}
	maybe_selected.clear();
	scene->queue_free();
	notify_property_list_changed();
}

void ImportPipelineStepRemoveNodes::update() {
	if (!source.is_valid()) {
		return;
	}
	Node *scene = source->instantiate();
	for (int i = 0; i < scene->get_child_count(); i++) {
		_iterate_remove(scene->get_child(i), "");
	}
	result.instantiate();
	result->pack(scene);
	scene->queue_free();
}

PackedStringArray ImportPipelineStepRemoveNodes::get_inputs() {
	PackedStringArray sources;
	sources.push_back("source");
	return sources;
}

PackedStringArray ImportPipelineStepRemoveNodes::get_outputs() {
	PackedStringArray sources;
	sources.push_back("result");
	return sources;
}
