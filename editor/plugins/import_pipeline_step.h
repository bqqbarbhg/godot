/**************************************************************************/
/*  import_pipeline_step.h                                                */
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

#ifndef IMPORT_PIPELINE_STEP_H
#define IMPORT_PIPELINE_STEP_H

#include "core/error/error_macros.h"
#include "core/io/resource_importer.h"
#include "core/variant/dictionary.h"
#include "scene/resources/packed_scene.h"

class ImportPipelineStep : public RefCounted {
	GDCLASS(ImportPipelineStep, RefCounted);

	Ref<Resource> _source;

protected:
	GDVIRTUAL0(_update)
	GDVIRTUAL0R(String, _get_display_name)
	GDVIRTUAL0(_source_changed)
	GDVIRTUAL0R(PackedStringArray, _get_inputs)
	GDVIRTUAL0R(PackedStringArray, _get_outputs)

	static void _bind_methods();

public:
	virtual void update();
	virtual void source_changed();
	virtual String get_display_name();
	virtual PackedStringArray get_inputs();
	virtual PackedStringArray get_outputs();

	ImportPipelineStep();
};

class ImportPipelineStepRemoveNodes : public ImportPipelineStep {
	GDCLASS(ImportPipelineStepRemoveNodes, ImportPipelineStep);

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _property_can_revert(const StringName &p_name) const;
	bool _property_get_revert(const StringName &p_name, Variant &r_property) const;

	Ref<PackedScene> source;
	Ref<PackedScene> result;
	HashMap<String, bool> selected;
	HashMap<String, bool> maybe_selected;
	List<PropertyInfo> properties;

	void _iterate_remove(Node *p_node, const String &p_prefix);
	void _iterate_properties(Node *p_node, const String &p_prefix);

protected:
	static void _bind_methods();

public:
	String get_display_name() override { return "Remove Nodes"; }
	void source_changed() override;
	void update() override;
	PackedStringArray get_inputs() override;
	PackedStringArray get_outputs() override;

	void set_source(Ref<PackedScene> p_scene) { source = p_scene; }
	Ref<PackedScene> get_source() { return source; }
	void set_result(Ref<PackedScene> p_scene) { result = p_scene; }
	Ref<PackedScene> get_result() { return result; }
};

#endif // IMPORT_PIPELINE_STEP_H
