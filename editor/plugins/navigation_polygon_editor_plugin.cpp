/*************************************************************************/
/*  navigation_polygon_editor_plugin.cpp                                 */
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

#include "navigation_polygon_editor_plugin.h"

#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"

Ref<NavigationPolygon> NavigationPolygonEditor::_ensure_navpoly() const {
	Ref<NavigationPolygon> navpoly = node->get_navigation_polygon();
	if (!navpoly.is_valid()) {
		navpoly = Ref<NavigationPolygon>(memnew(NavigationPolygon));
		node->set_navigation_polygon(navpoly);
	}
	return navpoly;
}

Node2D *NavigationPolygonEditor::_get_node() const {
	return node;
}

void NavigationPolygonEditor::_set_node(Node *p_polygon) {
	node = Object::cast_to<NavigationRegion2D>(p_polygon);
}

int NavigationPolygonEditor::_get_polygon_count() const {
	Ref<NavigationPolygon> navpoly = node->get_navigation_polygon();
	if (navpoly.is_valid()) {
		return navpoly->get_outline_count();
	} else {
		return 0;
	}
}

Variant NavigationPolygonEditor::_get_polygon(int p_idx) const {
	Ref<NavigationPolygon> navpoly = node->get_navigation_polygon();
	if (navpoly.is_valid()) {
		return navpoly->get_outline(p_idx);
	} else {
		return Variant(Vector<Vector2>());
	}
}

void NavigationPolygonEditor::_set_polygon(int p_idx, const Variant &p_polygon) const {
	Ref<NavigationPolygon> navpoly = _ensure_navpoly();
	navpoly->set_outline(p_idx, p_polygon);
	navpoly->make_polygons_from_outlines();
}

void NavigationPolygonEditor::_action_add_polygon(const Variant &p_polygon) {
	Ref<NavigationPolygon> navpoly = _ensure_navpoly();
	Ref<EditorUndoRedoManager> &undo_redo = EditorNode::get_undo_redo();
	undo_redo->add_do_method(navpoly.ptr(), "add_outline", p_polygon);
	undo_redo->add_undo_method(navpoly.ptr(), "remove_outline", navpoly->get_outline_count());
	undo_redo->add_do_method(navpoly.ptr(), "make_polygons_from_outlines");
	undo_redo->add_undo_method(navpoly.ptr(), "make_polygons_from_outlines");
}

void NavigationPolygonEditor::_action_remove_polygon(int p_idx) {
	Ref<NavigationPolygon> navpoly = _ensure_navpoly();
	Ref<EditorUndoRedoManager> &undo_redo = EditorNode::get_undo_redo();
	undo_redo->add_do_method(navpoly.ptr(), "remove_outline", p_idx);
	undo_redo->add_undo_method(navpoly.ptr(), "add_outline_at_index", navpoly->get_outline(p_idx), p_idx);
	undo_redo->add_do_method(navpoly.ptr(), "make_polygons_from_outlines");
	undo_redo->add_undo_method(navpoly.ptr(), "make_polygons_from_outlines");
}

void NavigationPolygonEditor::_action_set_polygon(int p_idx, const Variant &p_previous, const Variant &p_polygon) {
	Ref<NavigationPolygon> navpoly = _ensure_navpoly();
	Ref<EditorUndoRedoManager> &undo_redo = EditorNode::get_undo_redo();
	undo_redo->add_do_method(navpoly.ptr(), "set_outline", p_idx, p_polygon);
	undo_redo->add_undo_method(navpoly.ptr(), "set_outline", p_idx, p_previous);
	undo_redo->add_do_method(navpoly.ptr(), "make_polygons_from_outlines");
	undo_redo->add_undo_method(navpoly.ptr(), "make_polygons_from_outlines");
}

bool NavigationPolygonEditor::_has_resource() const {
	return node && node->get_navigation_polygon().is_valid();
}

bool NavigationPolygonEditor::_resource_is_foreign() const {
	if (node) {
		Ref<NavigationPolygon> navigation_polygon = node->get_navigation_polygon();
		if (navigation_polygon.is_valid()) {
			String path = navigation_polygon->get_path();
			if (!path.is_resource_file()) {
				int srpos = path.find("::");
				if (srpos != -1) {
					String base = path.substr(0, srpos);
					if (ResourceLoader::get_resource_type(base) == "PackedScene") {
						if (!get_tree()->get_edited_scene_root() || get_tree()->get_edited_scene_root()->get_scene_file_path() != base) {
							return true;
						}
					} else {
						if (FileAccess::exists(base + ".import")) {
							return true;
						}
					}
				}
			} else {
				if (FileAccess::exists(navigation_polygon->get_path() + ".import")) {
					return true;
				}
			}
		}
	}
	return false;
}

void NavigationPolygonEditor::_create_resource() {
	if (!node) {
		return;
	}

	Ref<EditorUndoRedoManager> &undo_redo = EditorNode::get_undo_redo();
	undo_redo->create_action(TTR("Create Navigation Polygon"));
	undo_redo->add_do_method(node, "set_navigation_polygon", Ref<NavigationPolygon>(memnew(NavigationPolygon)));
	undo_redo->add_undo_method(node, "set_navigation_polygon", Variant(Ref<RefCounted>()));
	undo_redo->commit_action();

	_menu_option(MODE_CREATE);
}

NavigationPolygonEditor::NavigationPolygonEditor() {}

NavigationPolygonEditorPlugin::NavigationPolygonEditorPlugin() :
		AbstractPolygon2DEditorPlugin(memnew(NavigationPolygonEditor), "NavigationRegion2D") {
}
