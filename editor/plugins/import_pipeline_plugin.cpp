/**************************************************************************/
/*  import_pipeline_plugin.cpp                                            */
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

#include "import_pipeline_plugin.h"

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/math/math_funcs.h"
#include "core/math/projection.h"
#include "core/os/keyboard.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/editor_file_dialog.h"
#include "editor/editor_file_system.h"
#include "editor/editor_node.h"
#include "editor/editor_resource_preview.h"
#include "editor/editor_settings.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/import_dock.h"
#include "editor/plugins/animation_player_editor_plugin.h"
#include "editor/plugins/node_3d_editor_gizmos.h"
#include "editor/scene_tree_dock.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/collision_shape_3d.h"
#include "scene/3d/decal.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics_body_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/gui/center_container.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/graph_edit.h"
#include "scene/gui/split_container.h"
#include "scene/gui/subviewport_container.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/sky_material.h"
#include "scene/resources/surface_tool.h"

class ImporterStep : public ImportPipelineStep {
	GDCLASS(ImporterStep, ImportPipelineStep)
	friend class ImportPipeline;

	List<ResourceImporter::ImportOption> options;
	HashMap<StringName, Variant> defaults;
	Ref<ResourceImporter> importer;
	Ref<Resource> result;
	String path;

	bool _set(const StringName &p_name, const Variant &p_value) {
		defaults[p_name] = p_value;
		return true;
	}

	bool _get(const StringName &p_name, Variant &r_ret) const {
		if (p_name == "result") {
			r_ret = result;
			return true;
		}
		if (defaults.has(p_name)) {
			r_ret = defaults[p_name];
			return true;
		}
		return false;
	}

	void _get_property_list(List<PropertyInfo> *p_list) const {
		for (ResourceImporter::ImportOption option : options) {
			p_list->push_back(option.option);
		}
		p_list->push_back(PropertyInfo(Variant::OBJECT, "result", PROPERTY_HINT_RESOURCE_TYPE, importer->get_resource_type()));
	}

	void update() override {
		String temp_path = ResourceFormatImporter::get_singleton()->get_import_base_path(path) + "-temp";
		importer->import(path, temp_path, defaults, nullptr);
		temp_path += "." + importer->get_save_extension();
		result = ResourceLoader::load(temp_path, "");
		result->set_path("");
		Ref<DirAccess> dir_access = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		dir_access->remove(temp_path);
	}

	String get_display_name() override {
		return vformat("Import \"%s\"", path);
	}

	PackedStringArray get_outputs() override {
		PackedStringArray results;
		results.append("result");
		return results;
	}

	void set_path(const String &p_path) {
		path = p_path;
	}

	void reload_import_parameters() {
		options.clear();
		defaults.clear();
		Ref<ConfigFile> config;
		config.instantiate();
		config->load(path + ".import");
		List<String> keys;
		config->get_section_keys("params", &keys);
		for (const String &E : keys) {
			Variant value = config->get_value("params", E);
			defaults[E] = value;
		}
		String importer_name = config->get_value("remap", "importer");
		importer = ResourceFormatImporter::get_singleton()->get_importer_by_name(importer_name);
		importer->get_import_options(path, &options);
	}
};

class LoaderStep : public ImportPipelineStep {
	GDCLASS(LoaderStep, ImportPipelineStep)
	friend class ImportPipeline;

	Ref<Resource> result;
	String path;

	bool _set(const StringName &p_name, const Variant &p_value) {
		return false;
	}

	bool _get(const StringName &p_name, Variant &r_ret) const {
		if (p_name == "result") {
			r_ret = result;
			return true;
		}
		return false;
	}

	void _get_property_list(List<PropertyInfo> *p_list) const {
		p_list->push_back(PropertyInfo(Variant::OBJECT, "result", PROPERTY_HINT_RESOURCE_TYPE, result->get_class()));
	}

	void update() override {
		result = ResourceLoader::load(path);
	}

	String get_display_name() override {
		return vformat("Load \"%s\"", path);
	}

	PackedStringArray get_outputs() override {
		PackedStringArray results;
		results.append("result");
		return results;
	}

	void load(const String &p_path) {
		path = p_path;
		update();
	}
};

class SaverStep : public ImportPipelineStep {
	GDCLASS(SaverStep, ImportPipelineStep)
	friend class ImportPipeline;

	Ref<Resource> source;
	String path;

	bool _set(const StringName &p_name, const Variant &p_value) {
		if (p_name == "source") {
			source = p_value;
			return true;
		}
		return false;
	}

	bool _get(const StringName &p_name, Variant &r_ret) const {
		return false;
	}

	void _get_property_list(List<PropertyInfo> *p_list) const {
		p_list->push_back(PropertyInfo(Variant::OBJECT, "source", PROPERTY_HINT_RESOURCE_TYPE));
	}

	void update() override {
		if (source == nullptr) {
			return;
		}
		if (FileAccess::exists(path + ".import")) {
			EditorFileSystem::get_singleton()->overwrite_file(path, source);
		} else {
			EditorNode::get_singleton()->save_resource_in_path(source, path);
		}
	}

	String get_display_name() override {
		return vformat("Save \"%s\"", path);
	}

	PackedStringArray get_inputs() override {
		PackedStringArray results;
		results.append("source");
		return results;
	}
};

//wrapper object to filter the in- and outputs of the step
class NodeData : public Object {
	GDCLASS(NodeData, Object)
	friend class ImportPipeline;

	List<PropertyInfo> options;
	Ref<ImportPipelineStep> step;
	bool results_valid = false;
	ImportPipeline *pipeline;
	String name;

	struct Source {
		String name;
		String type;

		//set if connected
		String source_node;
		int source_idx;
	};
	List<Source> sources;

	struct Result {
		String name;
		String type;
	};

	List<Result> results;

	bool _set(const StringName &p_name, const Variant &p_value) {
		bool valid;
		step->set(p_name, p_value, &valid);
		results_valid = false;
		pipeline->_update_preview();
		return valid;
	}

	bool _get(const StringName &p_name, Variant &r_ret) const {
		bool valid;
		r_ret = step->get(p_name, &valid);
		return valid;
	}

	void _get_property_list(List<PropertyInfo> *p_list) const {
		for (PropertyInfo option : options) {
			p_list->push_back(option);
		}
	}

	bool _property_can_revert(const StringName &p_name) const {
		return step->property_can_revert(p_name);
	}

	bool _property_get_revert(const StringName &p_name, Variant &r_property) const {
		r_property = step->property_get_revert(p_name);
		return true;
	}

	void update() {
		options.clear();
		PackedStringArray inputs = step->get_inputs();
		PackedStringArray outputs = step->get_outputs();
		List<PropertyInfo> properties;
		step->get_property_list(&properties, true);
		for (PropertyInfo property : properties) {
			if (property.name == "script") {
				continue;
			}
			if (inputs.has(property.name) || outputs.has(property.name)) {
				continue;
			}
			options.push_back(property);
		}
		notify_property_list_changed();
	}

	void update_connections() {
		sources.clear();
		results.clear();
		List<PropertyInfo> properties;
		step->get_property_list(&properties, true);

		PackedStringArray inputs = step->get_inputs();
		PackedStringArray outputs = step->get_outputs();

		for (PropertyInfo property : properties) {
			if (inputs.has(property.name)) {
				Source source;
				source.name = property.name;
				source.type = property.hint_string;
				if (source.type.is_empty()) {
					source.type = "Resource";
				}
				sources.push_back(source);
			} else if (outputs.has(property.name)) {
				Result result;
				result.name = property.name;
				result.type = property.hint_string;
				if (result.type.is_empty()) {
					result.type = "Resource";
				}
				results.push_back(result);
			}
		}
	}
};

ImportPipeline *ImportPipeline::singleton = nullptr;

void ImportPipeline::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			add_menu->add_item("Source");
			add_menu->add_separator();
			add_menu->add_item("Sink");
			add_menu->add_separator();
			add_menu->add_item("ImportPipelineStep");
			List<StringName> inheriters;
			ClassDB::get_inheriters_from_class("ImportPipelineStep", &inheriters);
			for (StringName name : inheriters) {
				add_menu->add_item(name);
			}

			List<String> extensions;
			ResourceLoader::get_recognized_extensions_for_type("Resource", &extensions);
			for (String extension : extensions) {
				source_dialog->add_filter("*." + extension);
				sink_dialog->add_filter("*." + extension);
			}
			EditorNode::get_singleton()->connect("resource_saved", callable_mp(this, &ImportPipeline::_editor_resource_changed));

			ImportDock::get_singleton()->connect("open_pipeline", callable_mp(this, &ImportPipeline::_edit));
			EditorFileSystem::get_singleton()->connect("resources_reimported", callable_mp(this, &ImportPipeline::_resources_reimported));
			_load();
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible()) {
				_update_preview();
			}
		} break;
	}
}

void ImportPipeline::_node_selected(Node *p_node) {
	NodeData *node_data = steps[p_node->get_name()];
	_get_result(p_node->get_name(), -1);
	if (node_data->step->get_class() == "ImportPipelineStep" && Ref<Script>(node_data->step->get_script()) == nullptr) {
		settings_inspector->set_hide_script(false);
		settings_inspector->edit(node_data->step.ptr());
	} else {
		settings_inspector->set_hide_script(true);
		settings_inspector->edit(node_data);
	}
	settings_node = p_node->get_name();
}

void ImportPipeline::_node_deselected(Node *p_node) {
	NodeData *node_data = steps[p_node->get_name()];
	if (settings_inspector->get_edited_object() == node_data) {
		settings_inspector->edit(nullptr);
		settings_node = "";
	}
}

bool ImportPipeline::_has_cycle(const String &current, const String &target) {
	if (current == target) {
		return true;
	}
	NodeData *node_data = steps[current];
	for (NodeData::Source source : node_data->sources) {
		if (source.source_node != "" && _has_cycle(source.source_node, target)) {
			return true;
		}
	}
	return false;
}

void ImportPipeline::_connection_request(const String &p_from, int p_from_index, const String &p_to, int p_to_index) {
	NodeData *to = steps[p_to];
	NodeData::Source current = to->sources[p_to_index];
	if (current.source_node == p_from) {
		graph->disconnect_node(p_from, p_from_index, p_to, p_to_index);
		to->sources[p_to_index].source_node = "";
		_update_preview();
	} else {
		if (_has_cycle(p_from, p_to)) {
			print_line("cycle");
			return;
		}
		String source_type = steps[p_from]->results[p_from_index].type;
		String target_type = current.type;
		if (!ClassDB::is_parent_class(source_type, target_type)) {
			print_line("types mismatch", source_type, target_type);
			return;
		}
		if (current.source_node != "") {
			graph->disconnect_node(current.source_node, current.source_idx, p_to, p_to_index);
		}
		graph->connect_node(p_from, p_from_index, p_to, p_to_index);
		to->sources[p_to_index].source_node = p_from;
		to->sources[p_to_index].source_idx = p_from_index;
		_update_preview();
	}
}

Ref<Resource> ImportPipeline::_get_result(StringName p_node, int p_idx) {
	NodeData *current = steps[p_node];
	bool valid = current->results_valid;

	for (NodeData::Source source : current->sources) {
		Ref<Resource> source_res;
		if (source.source_node == "") {
			source_res = Ref<Resource>();
		} else {
			source_res = _get_result(source.source_node, source.source_idx);
		}
		if (current->step->get(source.name) != source_res) {
			current->set(source.name, source_res);
			current->step->source_changed();
			valid = false;
		}
	}

	if (!valid) {
		current->step->update();
		current->results_valid = true;
	}
	if (p_idx >= 0) {
		return current->step->get(current->results[p_idx].name);
	} else {
		return nullptr;
	}
}

void ImportPipeline::_update_preview() {
	if (preview_node.is_empty()) {
		preview_inspector->edit(nullptr);
		preview_resource = Ref<Resource>();
	} else {
		Ref<Resource> result = _get_result(preview_node, preview_idx);
		preview_inspector->edit(result.ptr());
		preview_resource = result;
	}
}

void ImportPipeline::_result_button_pressed(StringName p_node, int p_idx) {
	if (preview_node == p_node && preview_idx == p_idx) {
		preview_node = "";
	} else {
		if (!preview_node.is_empty()) {
			NodeData *old = steps[preview_node];
			Button *old_button = Object::cast_to<Button>(graph->get_node(NodePath(preview_node))->get_child(preview_idx + old->sources.size()));
			old_button->set_pressed(false);
		}
		preview_node = p_node;
		preview_idx = p_idx;
	}
	_update_preview();
}

StringName ImportPipeline::_create_importer_node(Vector2 p_position, const String &p_path) {
	Ref<ImporterStep> step = memnew(ImporterStep);
	step->path = p_path;
	step->reload_import_parameters();
	if (used_resources.has(p_path)) {
		used_resources[p_path] = UsedState(used_resources[p_path] | USED_AS_SOURCE);
	} else {
		used_resources[p_path] = USED_AS_SOURCE;
	}

	return _create_node(step, p_position, p_path, USED_AS_SOURCE);
}

StringName ImportPipeline::_create_loader_node(Vector2 p_position, const String &p_path) {
	Ref<LoaderStep> step = memnew(LoaderStep);
	step->load(p_path);
	used_resources[p_path] = UsedState(USED_AS_SOURCE | USED_AS_SINK);
	return _create_node(step, p_position, p_path, UsedState(USED_AS_SOURCE | USED_AS_SINK));
}

StringName ImportPipeline::_create_saver_node(Vector2 p_position, const String &p_path) {
	Ref<SaverStep> step = memnew(SaverStep);
	UsedState state = FileAccess::exists(p_path + ".import") ? USED_AS_SINK : UsedState(USED_AS_SOURCE | USED_AS_SINK);
	step->path = p_path;
	used_resources[p_path] = UsedState((used_resources.has(p_path) ? used_resources[p_path] : USED_AS_NONE) | state);
	return _create_node(step, p_position, p_path, state);
}

void ImportPipeline ::_remove_node(StringName p_node, const String &p_path, int p_state) {
	if (preview_node == p_node) {
		preview_node = "";
		_update_preview();
	}
	if (settings_node == p_node) {
		_node_deselected(graph->get_node(NodePath(p_node)));
	}

	memfree(steps[p_node]);
	steps.erase(p_node);

	if (!p_path.is_empty()) {
		UsedState state = used_resources[p_path];
		state = UsedState(state & ~p_state); //remove active flags from "state" corresponding to "p_state"
		if (state == USED_AS_NONE) {
			used_resources.erase(p_path);
		} else {
			used_resources[p_path] = state;
		}
	}

	List<GraphEdit::Connection> connection_list;
	graph->get_connection_list(&connection_list);
	for (GraphEdit::Connection connection : connection_list) {
		if (connection.from == p_node) {
			steps[connection.to]->sources[connection.to_port].source_node = "";
			graph->disconnect_node(connection.from, connection.from_port, connection.to, connection.to_port);
		} else if (connection.to == p_node) {
			graph->disconnect_node(connection.from, connection.from_port, connection.to, connection.to_port);
		}
	}
	Node *node = graph->get_node(NodePath(p_node));
	graph->remove_child(node);
	memdelete(node);
}

void ImportPipeline::_update_node(StringName p_node) {
	NodeData *node_data = steps[p_node];
	GraphNode *node = Object::cast_to<GraphNode>(graph->get_node(NodePath(p_node)));
	for (int i = node->get_child_count() - 1; i >= 0; i--) {
		Node *child = node->get_child(i);
		node->remove_child(child);
		memdelete(child);
	}
	node_data->update_connections();
	node_data->update();
	node->set_title(node_data->step->get_display_name());

	for (NodeData::Source source : node_data->sources) {
		Label *label = memnew(Label);
		label->set_text(source.name);
		node->add_child(label);
		node->set_slot(label->get_index(), true, 0, Color(1.0, 1.0, 1.0), false, 0, Color(1.0, 1.0, 1.0));
	}
	for (int i = 0; i < node_data->results.size(); i++) {
		Button *button = memnew(Button);
		button->set_text(node_data->results[i].name);
		button->set_text_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
		button->set_toggle_mode(true);
		node->add_child(button);
		node->set_slot(button->get_index(), false, 0, Color(1.0, 1.0, 1.0), true, 0, Color(1.0, 1.0, 1.0));
		button->connect("pressed", callable_mp(this, &ImportPipeline::_result_button_pressed).bind(node->get_name(), i));
	}
	Control *spacer = memnew(Control);
	spacer->set_custom_minimum_size(Vector2(0.0, 5.0));
	node->add_child(spacer);
}

StringName ImportPipeline::_create_node(Ref<ImportPipelineStep> p_step, Vector2 p_position, const String &p_path, UsedState p_state) {
	GraphNode *node = memnew(GraphNode);
	graph->add_child(node);
	StringName name = node->get_name();
	node->set_position_offset(p_position);
	node->set_show_close_button(true);
	node->connect("close_request", callable_mp(this, &ImportPipeline ::_remove_node).bind(name, p_path, (int)p_state), CONNECT_DEFERRED);

	NodeData *node_data = memnew(NodeData);
	node_data->step = p_step;
	node_data->pipeline = this;

	node_data->name = name;
	p_step->connect("property_list_changed", callable_mp(node_data, &NodeData::update));
	steps[name] = node_data;
	_update_node(name);
	return name;
}

Vector2 ImportPipeline::_get_creation_position() {
	return (graph->get_scroll_ofs() + add_menu->get_position() - graph->get_screen_position()) / graph->get_zoom();
}

void ImportPipeline::_create_step(int p_idx) {
	if (p_idx >= 4) { //skip source, sink and the 2 seperators
		String name = add_menu->get_item_text(p_idx);
		Ref<ImportPipelineStep> step = Object::cast_to<ImportPipelineStep>(ClassDB::instantiate(name));
		_create_node(step, _get_creation_position());
	} else if (p_idx == 0) {
		source_dialog->popup_file_dialog();
	} else {
		sink_dialog->popup_file_dialog();
	}
}

void ImportPipeline::_create_add_popup(Vector2 position) {
	add_menu->set_position(position + graph->get_screen_position());
	add_menu->popup();
}

void ImportPipeline::_settings_property_changed(const String &p_name) {
	if (p_name == "script") {
		_update_node(settings_node);
		_node_selected(graph->get_node(NodePath(settings_node)));
	}
}

void ImportPipeline::_load() {
	graph->clear_connections();
	for (int i = graph->get_child_count() - 1; i >= 0; i--) {
		if (Object::cast_to<GraphNode>(graph->get_child(i))) {
			graph->get_child(i)->queue_free();
		}
	}
	for (KeyValue<String, NodeData *> entry : steps) {
		memfree(entry.value);
	}
	steps.clear();
	preview_inspector->edit(nullptr);
	preview_node = "";
	settings_inspector->edit(nullptr);
	settings_node = "";

	if (FileAccess::exists("project.import_pipeline")) {
		ConfigFile config_file;
		config_file.load("project.import_pipeline");
		Array steps_data = config_file.get_value(CONFIG_SECTION, "steps");
		Array connection_list = config_file.get_value(CONFIG_SECTION, "connections");
		Vector<StringName> step_names;
		step_names.resize(steps_data.size());

		for (int i = 0; i < steps_data.size(); i++) {
			Dictionary step_data = steps_data[i];
			String type = step_data["type"];
			StringName name;
			if (type == "importer") {
				name = _create_importer_node(step_data["position"], step_data["path"]);
			} else if (type == "loader") {
				name = _create_loader_node(step_data["position"], step_data["path"]);
			} else if (type == "saver") {
				name = _create_saver_node(step_data["position"], step_data["path"]);
			} else {
				Ref<ImportPipelineStep> step = Ref<ImportPipelineStep>(ClassDB::instantiate(step_data["step"]));
				if (step_data.has("script")) {
					step->set_script(step_data["script"]);
				}
				Dictionary parameters = step_data["parameters"];
				Array keys = parameters.keys();
				for (int j = 0; j < keys.size(); j++) {
					String key = keys[j];
					step->set(key, parameters[key]);
				}
				name = _create_node(step, step_data["position"]);
			}
			step_names.set(i, name);
		}
		for (int i = 0; i < connection_list.size(); i++) {
			Dictionary con = connection_list[i];
			_connection_request(step_names[con["from"]], con["from_port"], step_names[con["to"]], con["to_port"]);
		}
	}
}

void ImportPipeline::_add_source(const String &p_path) {
	if (FileAccess::exists(p_path + ".import")) {
		if (used_resources.has(p_path) && (used_resources[p_path] & USED_AS_SOURCE) != 0) {
			print_error(vformat(TTR("\"%s\" already used."), p_path));
			return;
		}
		_create_importer_node(_get_creation_position(), p_path);
	} else {
		if (used_resources.has(p_path)) {
			print_error(vformat(TTR("\"%s\" already used."), p_path));
			return;
		}
		_create_loader_node(_get_creation_position(), p_path);
	}
}

void ImportPipeline::_add_sink(const String &p_path) {
	if (FileAccess::exists(p_path + ".import")) {
		if (used_resources.has(p_path) && (used_resources[p_path] & USED_AS_SINK) != 0) {
			print_error(vformat(TTR("\"%s\" already used."), p_path));
			return;
		}
		_create_saver_node(_get_creation_position(), p_path);
	} else {
		if (used_resources.has(p_path)) {
			print_error(vformat(TTR("\"%s\" already used."), p_path));
			return;
		}
		_create_saver_node(_get_creation_position(), p_path);
	}
}

void ImportPipeline::_editor_resource_changed(Ref<Resource> p_resource) {
	if (!used_resources.has(p_resource->get_path()) || (used_resources[p_resource->get_path()] & USED_AS_SOURCE) == 0) {
		return;
	}
	for (KeyValue<String, NodeData *> entry : steps) {
		Ref<LoaderStep> loader_step = entry.value->step;
		if (loader_step == nullptr) {
			continue;
		}
		if (loader_step->path != p_resource->get_path()) {
			continue;
		}
		entry.value->results_valid = false;
		break;
	}
}

GraphNode *ImportPipeline::_get_import_node(const String &p_path) {
	if (!used_resources.has(p_path)) {
		return nullptr;
	}
	UsedState state = used_resources[p_path];
	if ((state & USED_AS_SOURCE) == 0) {
		return nullptr;
	}
	for (KeyValue<String, NodeData *> entry : steps) {
		Ref<ImporterStep> importer_step = entry.value->step;
		if (importer_step == nullptr) {
			continue;
		}
		if (importer_step->path != p_path) {
			continue;
		}
		return Object::cast_to<GraphNode>(graph->get_node(entry.key));
	}
	return nullptr;
}

void ImportPipeline::_resources_reimported(PackedStringArray p_paths) {
	for (const String &path : p_paths) {
		_invalidate(path);
	}
}

Dictionary ImportPipeline::get_state() const {
	Dictionary state;
	state["zoom"] = graph->get_zoom();
	state["offsets"] = graph->get_scroll_ofs();
	return state;
}

void ImportPipeline::set_state(const Dictionary &p_state) {
	graph->set_zoom(p_state["zoom"]);
	graph->set_scroll_ofs(p_state["offsets"]);
}

void ImportPipeline::_save() {
	_execute();

	ConfigFile save;
	HashMap<StringName, int> step_ids;

	/*save steps*/ {
		Array array;
		for (KeyValue<String, NodeData *> entry : steps) {
			GraphNode *node = Object::cast_to<GraphNode>(graph->get_node(entry.key));
			NodeData *node_data = entry.value;
			Ref<ImportPipelineStep> step = node_data->step;
			Dictionary dict;
			dict["position"] = node->get_position_offset();
			if (Object::cast_to<ImporterStep>(step.ptr()) != nullptr) {
				dict["type"] = "importer";
				Ref<ImporterStep> importer_step = step;
				dict["path"] = importer_step->path;
				EditorFileSystem::get_singleton()->update_import_parameters(importer_step->path, importer_step->defaults);
			} else if (Object::cast_to<LoaderStep>(step.ptr()) != nullptr) {
				dict["type"] = "loader";
				dict["path"] = Object::cast_to<LoaderStep>(step.ptr())->path;
			} else if (Object::cast_to<SaverStep>(step.ptr()) != nullptr) {
				dict["type"] = "saver";
				dict["path"] = Object::cast_to<SaverStep>(step.ptr())->path;
			} else {
				dict["type"] = "step";
				dict["step"] = step->get_class_name();
				Ref<Script> custom_script = step->get_script();
				if (custom_script != nullptr) {
					dict["script"] = custom_script;
				}
				PackedStringArray inputs = step->get_inputs();
				PackedStringArray outputs = step->get_outputs();
				Dictionary parameters;
				List<PropertyInfo> properties;
				step->get_property_list(&properties);
				for (PropertyInfo property : properties) {
					if ((property.usage & PROPERTY_USAGE_STORAGE) == 0) {
						continue;
					}
					if (property.name == "script" || inputs.has(property.name) || outputs.has(property.name)) {
						continue;
					}
					parameters[property.name] = step->get(property.name);
				}
				dict["parameters"] = parameters;
			}
			step_ids[node->get_name()] = array.size();
			array.push_back(dict);
		}
		save.set_value(CONFIG_SECTION, "steps", array);
	}

	/*save connections*/ {
		//could be saved as one int-array, but would be less readable
		Array array;
		List<GraphEdit::Connection> connection_list;
		graph->get_connection_list(&connection_list);
		for (GraphEdit::Connection connection : connection_list) {
			Dictionary dict;
			dict["from"] = step_ids[connection.from];
			dict["from_port"] = connection.from_port;
			dict["to"] = step_ids[connection.to];
			dict["to_port"] = connection.to_port;
			array.push_back(dict);
		}
		save.set_value(CONFIG_SECTION, "connections", array);
	}

	save.save("project.import_pipeline");
}

void ImportPipeline::_edit(const String &p_path) {
	GraphNode *graph_node = _get_import_node(p_path);
	if (graph_node == nullptr) {
		return;
	}
	graph->set_selected(graph_node);
	graph->set_scroll_ofs(graph_node->get_position_offset() * graph->get_zoom() - graph->get_size() * 0.5);
	if (!is_visible()) {
		EditorNode::get_singleton()->editor_select(EditorNode::EDITOR_IMPORT_PIPELINE);
	}
}

void ImportPipeline::_invalidate(const String &p_path) {
	GraphNode *graph_node = _get_import_node(p_path);
	if (graph_node == nullptr) {
		return;
	}
	NodeData *node_data = steps[graph_node->get_name()];
	Ref<ImporterStep> import_step = node_data->step;
	import_step->reload_import_parameters();
	node_data->results_valid = false;
	if (is_visible()) {
		_update_preview();
	}
}

void ImportPipeline::_execute() {
	for (KeyValue<String, NodeData *> entry : steps) {
		_get_result(entry.key, -1);
	}
}

ImportPipeline::ImportPipeline() {
	singleton = this;

	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);

	HBoxContainer *top_bar = memnew(HBoxContainer);
	add_child(top_bar);

	Button *save_button = memnew(Button);
	top_bar->add_child(save_button);
	save_button->set_text("Save");
	save_button->connect("pressed", callable_mp(this, &ImportPipeline::_save));

	source_dialog = memnew(EditorFileDialog);
	source_dialog->connect("file_selected", callable_mp(this, &ImportPipeline::_add_source));
	source_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	source_dialog->set_show_hidden_files(false);
	source_dialog->set_display_mode(EditorFileDialog::DISPLAY_THUMBNAILS);
	source_dialog->set_title(TTR("Select Source"));
	add_child(source_dialog);

	sink_dialog = memnew(EditorFileDialog);
	sink_dialog->connect("file_selected", callable_mp(this, &ImportPipeline::_add_sink));
	sink_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	sink_dialog->set_show_hidden_files(false);
	sink_dialog->set_display_mode(EditorFileDialog::DISPLAY_THUMBNAILS);
	sink_dialog->set_title(TTR("Select Sink"));
	add_child(sink_dialog);

	HSplitContainer *content = memnew(HSplitContainer);
	add_child(content);
	content->set_v_size_flags(SIZE_EXPAND_FILL);

	VBoxContainer *left_side = memnew(VBoxContainer);
	content->add_child(left_side);
	left_side->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	Label *left_label = memnew(Label);
	left_side->add_child(left_label);
	left_label->set_text(TTR("Step Settings"));

	settings_inspector = memnew(EditorInspector);
	left_side->add_child(settings_inspector);
	settings_inspector->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
	settings_inspector->set_autoclear(true);
	settings_inspector->set_show_categories(true);
	settings_inspector->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	settings_inspector->set_use_doc_hints(false);
	settings_inspector->set_hide_script(true);
	settings_inspector->set_hide_metadata(true);
	settings_inspector->set_property_name_style(EditorPropertyNameProcessor::get_default_inspector_style());
	settings_inspector->connect("property_edited", callable_mp(this, &ImportPipeline::_settings_property_changed));

	HSplitContainer *right_split = memnew(HSplitContainer);
	content->add_child(right_split);
	right_split->set_v_size_flags(SIZE_EXPAND_FILL);
	right_split->set_h_size_flags(SIZE_EXPAND_FILL);

	graph = memnew(GraphEdit);
	right_split->add_child(graph);
	graph->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	graph->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	graph->connect("connection_request", callable_mp(this, &ImportPipeline::_connection_request));
	graph->connect("node_selected", callable_mp(this, &ImportPipeline::_node_selected));
	graph->connect("node_deselected", callable_mp(this, &ImportPipeline::_node_deselected));
	graph->connect("popup_request", callable_mp(this, &ImportPipeline::_create_add_popup));

	VBoxContainer *right_side = memnew(VBoxContainer);
	right_split->add_child(right_side);
	right_side->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	Label *right_label = memnew(Label);
	right_side->add_child(right_label);
	right_label->set_text(TTR("Preview"));

	preview_inspector = memnew(EditorInspector);
	right_side->add_child(preview_inspector);
	preview_inspector->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
	preview_inspector->set_autoclear(true);
	preview_inspector->set_show_categories(true);
	preview_inspector->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	preview_inspector->set_use_doc_hints(false);
	preview_inspector->set_hide_script(false);
	preview_inspector->set_hide_metadata(true);
	preview_inspector->set_property_name_style(EditorPropertyNameProcessor::get_default_inspector_style());
	preview_inspector->set_read_only(true);

	add_menu = memnew(PopupMenu);
	add_child(add_menu);
	add_menu->connect("id_pressed", callable_mp(this, &ImportPipeline::_create_step));
}

ImportPipeline::~ImportPipeline() {
	singleton = nullptr;
}

///////////////////////////////////////

void ImportPipelinePlugin::make_visible(bool p_visible) {
	if (p_visible) {
		ImportPipeline::get_singleton()->show();
	} else {
		ImportPipeline::get_singleton()->hide();
	}
}

Dictionary ImportPipelinePlugin::get_state() const {
	return ImportPipeline::get_singleton()->get_state();
}

void ImportPipelinePlugin::set_state(const Dictionary &p_state) {
	ImportPipeline::get_singleton()->set_state(p_state);
}

ImportPipelinePlugin::ImportPipelinePlugin() {
	if (ImportPipeline::get_singleton() == nullptr) {
		memnew(ImportPipeline);
	}
	EditorNode::get_singleton()->get_main_screen_control()->add_child(ImportPipeline::get_singleton());
	ImportPipeline::get_singleton()->hide();
}

ImportPipelinePlugin::~ImportPipelinePlugin() {
}
