/**************************************************************************/
/*  import_pipeline_plugin.h                                              */
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

#ifndef IMPORT_PIPELINE_PLUGIN_H
#define IMPORT_PIPELINE_PLUGIN_H

#include "editor/editor_file_dialog.h"
#include "editor/editor_plugin.h"
#include "editor/editor_scale.h"
#include "editor/plugins/import_pipeline_step.h"
#include "editor/plugins/node_3d_editor_gizmos.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/graph_edit.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"

class NodeData;

class ImportPipeline : public VBoxContainer {
	GDCLASS(ImportPipeline, VBoxContainer);
	friend class NodeData;

	const String CONFIG_SECTION = "post_import";
	static ImportPipeline *singleton;

	EditorInspector *preview_inspector;
	String preview_node;
	int preview_idx;
	Ref<Resource> preview_resource;

	enum UsedState {
		USED_AS_NONE = 0,
		USED_AS_SOURCE = 1 << 0,
		USED_AS_SINK = 1 << 1,
	};

	HashMap<String, UsedState> used_resources;

	EditorInspector *settings_inspector;
	String settings_node;

	EditorFileDialog *source_dialog;
	EditorFileDialog *sink_dialog;

	GraphEdit *graph;
	HashMap<String, NodeData *> steps;
	PopupMenu *add_menu;

	StringName _create_importer_node(Vector2 p_position, const String &p_path);
	StringName _create_loader_node(Vector2 p_position, const String &p_path);
	StringName _create_saver_node(Vector2 p_position, const String &p_path);
	StringName _create_node(Ref<ImportPipelineStep> p_step, Vector2 p_position, const String &p_path = "", UsedState p_state = USED_AS_NONE);

	void _connection_request(const String &p_from, int p_from_index, const String &p_to, int p_to_index);
	void _node_selected(Node *p_node);
	void _node_deselected(Node *p_node);
	void _result_button_pressed(StringName p_node, int p_idx);
	Ref<Resource> _get_result(StringName p_node, int p_idx);
	void _remove_node(StringName p_node, const String &p_path, int p_state);
	void _create_step(int p_idx);
	void _create_add_popup(Vector2 position);
	void _update_node(StringName p_node);
	bool _has_cycle(const String &current, const String &target);
	void _update_preview();
	void _settings_property_changed(const String &p_name);
	void _load();
	void _add_source(const String &p_path);
	void _add_sink(const String &p_path);
	Vector2 _get_creation_position();
	void _editor_resource_changed(Ref<Resource> p_resource);
	GraphNode *_get_import_node(const String &p_path);
	void _resources_reimported(PackedStringArray p_paths);
	void _save();
	void _execute();
	void _invalidate(const String &p_path);
	void _edit(const String &p_path);

protected:
	void
	_notification(int p_what);

public:
	static ImportPipeline *get_singleton() { return singleton; }

	Dictionary get_state() const;
	void set_state(const Dictionary &p_state);

	ImportPipeline();
	~ImportPipeline();
};

class ImportPipelinePlugin : public EditorPlugin {
	GDCLASS(ImportPipelinePlugin, EditorPlugin);

public:
	virtual String get_name() const override { return "ImportPipeline"; }
	bool has_main_screen() const override { return true; }
	void make_visible(bool p_visible) override;

	Dictionary get_state() const override;
	void set_state(const Dictionary &p_state) override;

	ImportPipelinePlugin();
	~ImportPipelinePlugin();
};

#endif // IMPORT_PIPELINE_PLUGIN_H
