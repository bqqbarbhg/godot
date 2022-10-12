/*************************************************************************/
/*  window.cpp                                                           */
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

#include "window.h"

#include "core/config/project_settings.h"
#include "core/debugger/engine_debugger.h"
#include "core/input/shortcut.h"
#include "core/string/translation.h"
#include "core/variant/variant_parser.h"
#include "scene/gui/control.h"
#include "scene/scene_string_names.h"
#include "scene/theme/theme_db.h"
#include "scene/theme/theme_owner.h"

void Window::set_title(const String &p_title) {
	title = p_title;

	if (embedder) {
		embedder->_sub_window_update(this);
	} else if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		String tr_title = atr(p_title);
#ifdef DEBUG_ENABLED
		if (window_id == DisplayServer::MAIN_WINDOW_ID) {
			// Append a suffix to the window title to denote that the project is running
			// from a debug build (including the editor). Since this results in lower performance,
			// this should be clearly presented to the user.
			tr_title = vformat("%s (DEBUG)", tr_title);
		}
#endif
		DisplayServer::get_singleton()->window_set_title(tr_title, window_id);
	}
}

String Window::get_title() const {
	return title;
}

void Window::set_current_screen(int p_screen) {
	current_screen = p_screen;
	if (window_id == DisplayServer::INVALID_WINDOW_ID) {
		return;
	}
	DisplayServer::get_singleton()->window_set_current_screen(p_screen, window_id);
}

int Window::get_current_screen() const {
	if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		current_screen = DisplayServer::get_singleton()->window_get_current_screen(window_id);
	}
	return current_screen;
}

void Window::set_position(const Point2i &p_position) {
	position = p_position;

	if (embedder) {
		embedder->_sub_window_update(this);

	} else if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_position(p_position, window_id);
	}
}

Point2i Window::get_position() const {
	return position;
}

void Window::set_size(const Size2i &p_size) {
	size = p_size;
	_update_window_size();
}

Size2i Window::get_size() const {
	return size;
}

void Window::reset_size() {
	set_size(Size2i());
}

Size2i Window::get_real_size() const {
	if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		return DisplayServer::get_singleton()->window_get_real_size(window_id);
	}
	return size;
}

void Window::set_max_size(const Size2i &p_max_size) {
	max_size = p_max_size;
	_update_window_size();
}

Size2i Window::get_max_size() const {
	return max_size;
}

void Window::set_min_size(const Size2i &p_min_size) {
	min_size = p_min_size;
	_update_window_size();
}

Size2i Window::get_min_size() const {
	return min_size;
}

void Window::set_mode(Mode p_mode) {
	mode = p_mode;

	if (embedder) {
		embedder->_sub_window_update(this);

	} else if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_mode(DisplayServer::WindowMode(p_mode), window_id);
	}
}

Window::Mode Window::get_mode() const {
	if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		mode = (Mode)DisplayServer::get_singleton()->window_get_mode(window_id);
	}
	return mode;
}

void Window::set_flag(Flags p_flag, bool p_enabled) {
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	flags[p_flag] = p_enabled;

	if (embedder) {
		embedder->_sub_window_update(this);

	} else if (window_id != DisplayServer::INVALID_WINDOW_ID) {
#ifdef TOOLS_ENABLED
		if ((p_flag != FLAG_POPUP) || !(Engine::get_singleton()->is_editor_hint() && get_tree()->get_edited_scene_root() && (get_tree()->get_edited_scene_root()->is_ancestor_of(this) || get_tree()->get_edited_scene_root() == this))) {
			DisplayServer::get_singleton()->window_set_flag(DisplayServer::WindowFlags(p_flag), p_enabled, window_id);
		}
#else
		DisplayServer::get_singleton()->window_set_flag(DisplayServer::WindowFlags(p_flag), p_enabled, window_id);
#endif
	}
}

bool Window::get_flag(Flags p_flag) const {
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	if (window_id != DisplayServer::INVALID_WINDOW_ID) {
#ifdef TOOLS_ENABLED
		if ((p_flag != FLAG_POPUP) || !(Engine::get_singleton()->is_editor_hint() && get_tree()->get_edited_scene_root() && (get_tree()->get_edited_scene_root()->is_ancestor_of(this) || get_tree()->get_edited_scene_root() == this))) {
			flags[p_flag] = DisplayServer::get_singleton()->window_get_flag(DisplayServer::WindowFlags(p_flag), window_id);
		}
#else
		flags[p_flag] = DisplayServer::get_singleton()->window_get_flag(DisplayServer::WindowFlags(p_flag), window_id);
#endif
	}
	return flags[p_flag];
}

bool Window::is_maximize_allowed() const {
	if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		return DisplayServer::get_singleton()->window_is_maximize_allowed(window_id);
	}
	return true;
}

void Window::request_attention() {
	if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_request_attention(window_id);
	}
}

void Window::move_to_foreground() {
	if (embedder) {
		embedder->_sub_window_grab_focus(this);

	} else if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_move_to_foreground(window_id);
	}
}

bool Window::can_draw() const {
	if (!is_inside_tree()) {
		return false;
	}
	if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		return DisplayServer::get_singleton()->window_can_draw(window_id);
	}

	return visible;
}

void Window::set_ime_active(bool p_active) {
	if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_ime_active(p_active, window_id);
	}
}

void Window::set_ime_position(const Point2i &p_pos) {
	if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_ime_position(p_pos, window_id);
	}
}

bool Window::is_embedded() const {
	ERR_FAIL_COND_V(!is_inside_tree(), false);

	return _get_embedder() != nullptr;
}

void Window::_make_window() {
	ERR_FAIL_COND(window_id != DisplayServer::INVALID_WINDOW_ID);

	uint32_t f = 0;
	for (int i = 0; i < FLAG_MAX; i++) {
		if (flags[i]) {
			f |= (1 << i);
		}
	}

	DisplayServer::VSyncMode vsync_mode = DisplayServer::get_singleton()->window_get_vsync_mode(DisplayServer::MAIN_WINDOW_ID);
	window_id = DisplayServer::get_singleton()->create_sub_window(DisplayServer::WindowMode(mode), vsync_mode, f, Rect2i(position, size));
	ERR_FAIL_COND(window_id == DisplayServer::INVALID_WINDOW_ID);
	DisplayServer::get_singleton()->window_set_current_screen(current_screen, window_id);
	DisplayServer::get_singleton()->window_set_max_size(Size2i(), window_id);
	DisplayServer::get_singleton()->window_set_min_size(Size2i(), window_id);
	String tr_title = atr(title);
#ifdef DEBUG_ENABLED
	if (window_id == DisplayServer::MAIN_WINDOW_ID) {
		// Append a suffix to the window title to denote that the project is running
		// from a debug build (including the editor). Since this results in lower performance,
		// this should be clearly presented to the user.
		tr_title = vformat("%s (DEBUG)", tr_title);
	}
#endif
	DisplayServer::get_singleton()->window_set_title(tr_title, window_id);
	DisplayServer::get_singleton()->window_attach_instance_id(get_instance_id(), window_id);
#ifdef TOOLS_ENABLED
	if (!(Engine::get_singleton()->is_editor_hint() && get_tree()->get_edited_scene_root() && (get_tree()->get_edited_scene_root()->is_ancestor_of(this) || get_tree()->get_edited_scene_root() == this))) {
		DisplayServer::get_singleton()->window_set_exclusive(window_id, exclusive);
	} else {
		DisplayServer::get_singleton()->window_set_exclusive(window_id, false);
	}
#else
	DisplayServer::get_singleton()->window_set_exclusive(window_id, exclusive);
#endif

	_update_window_size();

	if (transient_parent && transient_parent->window_id != DisplayServer::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_transient(window_id, transient_parent->window_id);
	}

	for (const Window *E : transient_children) {
		if (E->window_id != DisplayServer::INVALID_WINDOW_ID) {
			DisplayServer::get_singleton()->window_set_transient(E->window_id, transient_parent->window_id);
		}
	}

	_update_window_callbacks();

	RS::get_singleton()->viewport_set_update_mode(get_viewport_rid(), RS::VIEWPORT_UPDATE_WHEN_VISIBLE);
	DisplayServer::get_singleton()->show_window(window_id);
}

void Window::_update_from_window() {
	ERR_FAIL_COND(window_id == DisplayServer::INVALID_WINDOW_ID);
	mode = (Mode)DisplayServer::get_singleton()->window_get_mode(window_id);
	for (int i = 0; i < FLAG_MAX; i++) {
		flags[i] = DisplayServer::get_singleton()->window_get_flag(DisplayServer::WindowFlags(i), window_id);
	}
}

void Window::_clear_window() {
	ERR_FAIL_COND(window_id == DisplayServer::INVALID_WINDOW_ID);

	if (transient_parent && transient_parent->window_id != DisplayServer::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_transient(window_id, DisplayServer::INVALID_WINDOW_ID);
	}

	for (const Window *E : transient_children) {
		if (E->window_id != DisplayServer::INVALID_WINDOW_ID) {
			DisplayServer::get_singleton()->window_set_transient(E->window_id, DisplayServer::INVALID_WINDOW_ID);
		}
	}

	_update_from_window();

	DisplayServer::get_singleton()->delete_sub_window(window_id);
	window_id = DisplayServer::INVALID_WINDOW_ID;

	// If closing window was focused and has a parent, return focus.
	if (focused && transient_parent) {
		transient_parent->grab_focus();
	}

	_update_viewport_size();
	RS::get_singleton()->viewport_set_update_mode(get_viewport_rid(), RS::VIEWPORT_UPDATE_DISABLED);
}

void Window::_rect_changed_callback(const Rect2i &p_callback) {
	//we must always accept this as the truth
	if (size == p_callback.size && position == p_callback.position) {
		return;
	}
	position = p_callback.position;

	if (size != p_callback.size) {
		size = p_callback.size;
		_update_viewport_size();
	}
}

void Window::_propagate_window_notification(Node *p_node, int p_notification) {
	p_node->notification(p_notification);
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		Window *window = Object::cast_to<Window>(child);
		if (window) {
			continue;
		}
		_propagate_window_notification(child, p_notification);
	}
}

void Window::_event_callback(DisplayServer::WindowEvent p_event) {
	switch (p_event) {
		case DisplayServer::WINDOW_EVENT_MOUSE_ENTER: {
			_propagate_window_notification(this, NOTIFICATION_WM_MOUSE_ENTER);
			emit_signal(SNAME("mouse_entered"));
			notification(NOTIFICATION_VP_MOUSE_ENTER);
			if (DisplayServer::get_singleton()->has_feature(DisplayServer::FEATURE_CURSOR_SHAPE)) {
				DisplayServer::get_singleton()->cursor_set_shape(DisplayServer::CURSOR_ARROW); //restore cursor shape
			}
		} break;
		case DisplayServer::WINDOW_EVENT_MOUSE_EXIT: {
			notification(NOTIFICATION_VP_MOUSE_EXIT);
			_propagate_window_notification(this, NOTIFICATION_WM_MOUSE_EXIT);
			emit_signal(SNAME("mouse_exited"));
		} break;
		case DisplayServer::WINDOW_EVENT_FOCUS_IN: {
			focused = true;
			_propagate_window_notification(this, NOTIFICATION_WM_WINDOW_FOCUS_IN);
			emit_signal(SNAME("focus_entered"));

		} break;
		case DisplayServer::WINDOW_EVENT_FOCUS_OUT: {
			focused = false;
			_propagate_window_notification(this, NOTIFICATION_WM_WINDOW_FOCUS_OUT);
			emit_signal(SNAME("focus_exited"));
		} break;
		case DisplayServer::WINDOW_EVENT_CLOSE_REQUEST: {
			if (exclusive_child != nullptr) {
				break; //has an exclusive child, can't get events until child is closed
			}
			_propagate_window_notification(this, NOTIFICATION_WM_CLOSE_REQUEST);
			emit_signal(SNAME("close_requested"));
		} break;
		case DisplayServer::WINDOW_EVENT_GO_BACK_REQUEST: {
			_propagate_window_notification(this, NOTIFICATION_WM_GO_BACK_REQUEST);
			emit_signal(SNAME("go_back_requested"));
		} break;
		case DisplayServer::WINDOW_EVENT_DPI_CHANGE: {
			_update_viewport_size();
			_propagate_window_notification(this, NOTIFICATION_WM_DPI_CHANGE);
			emit_signal(SNAME("dpi_changed"));
		} break;
		case DisplayServer::WINDOW_EVENT_TITLEBAR_CHANGE: {
			emit_signal(SNAME("titlebar_changed"));
		} break;
	}
}

void Window::update_mouse_cursor_shape() {
	// The default shape is set in Viewport::_gui_input_event. To instantly
	// see the shape in the viewport we need to trigger a mouse motion event.
	Ref<InputEventMouseMotion> mm;
	Vector2 pos = get_mouse_position();
	Transform2D xform = get_global_canvas_transform().affine_inverse();
	mm.instantiate();
	mm->set_position(pos);
	mm->set_global_position(xform.xform(pos));
	push_input(mm);
}

void Window::show() {
	set_visible(true);
}

void Window::hide() {
	set_visible(false);
}

void Window::set_visible(bool p_visible) {
	if (visible == p_visible) {
		return;
	}

	visible = p_visible;

	if (!is_inside_tree()) {
		return;
	}

	if (updating_child_controls) {
		_update_child_controls();
	}

	ERR_FAIL_COND_MSG(get_parent() == nullptr, "Can't change visibility of main window.");

	Viewport *embedder_vp = _get_embedder();

	if (!embedder_vp) {
		if (!p_visible && window_id != DisplayServer::INVALID_WINDOW_ID) {
			_clear_window();
		}
		if (p_visible && window_id == DisplayServer::INVALID_WINDOW_ID) {
			_make_window();
		}
	} else {
		if (visible) {
			embedder = embedder_vp;
			embedder->_sub_window_register(this);
			RS::get_singleton()->viewport_set_update_mode(get_viewport_rid(), RS::VIEWPORT_UPDATE_WHEN_PARENT_VISIBLE);
		} else {
			embedder->_sub_window_remove(this);
			embedder = nullptr;
			RS::get_singleton()->viewport_set_update_mode(get_viewport_rid(), RS::VIEWPORT_UPDATE_DISABLED);
		}
		_update_window_size();
	}

	if (!visible) {
		focused = false;
	}
	notification(NOTIFICATION_VISIBILITY_CHANGED);
	emit_signal(SceneStringNames::get_singleton()->visibility_changed);

	RS::get_singleton()->viewport_set_active(get_viewport_rid(), visible);

	//update transient exclusive
	if (transient_parent) {
		if (exclusive && visible) {
#ifdef TOOLS_ENABLED
			if (!(Engine::get_singleton()->is_editor_hint() && get_tree()->get_edited_scene_root() && (get_tree()->get_edited_scene_root()->is_ancestor_of(this) || get_tree()->get_edited_scene_root() == this))) {
				ERR_FAIL_COND_MSG(transient_parent->exclusive_child && transient_parent->exclusive_child != this, "Transient parent has another exclusive child.");
				transient_parent->exclusive_child = this;
			}
#else
			transient_parent->exclusive_child = this;
#endif
		} else {
			if (transient_parent->exclusive_child == this) {
				transient_parent->exclusive_child = nullptr;
			}
		}
	}
}

void Window::_clear_transient() {
	if (transient_parent) {
		if (transient_parent->window_id != DisplayServer::INVALID_WINDOW_ID && window_id != DisplayServer::INVALID_WINDOW_ID) {
			DisplayServer::get_singleton()->window_set_transient(window_id, DisplayServer::INVALID_WINDOW_ID);
		}
		transient_parent->transient_children.erase(this);
		if (transient_parent->exclusive_child == this) {
			transient_parent->exclusive_child = nullptr;
		}
		transient_parent = nullptr;
	}
}

void Window::_make_transient() {
	if (!get_parent()) {
		//main window, can't be transient
		return;
	}
	//find transient parent
	Viewport *vp = get_parent()->get_viewport();
	Window *window = nullptr;
	while (vp) {
		window = Object::cast_to<Window>(vp);
		if (window) {
			break;
		}
		if (!vp->get_parent()) {
			break;
		}

		vp = vp->get_parent()->get_viewport();
	}

	if (window) {
		transient_parent = window;
		window->transient_children.insert(this);
		if (is_inside_tree() && is_visible() && exclusive) {
			if (transient_parent->exclusive_child == nullptr) {
#ifdef TOOLS_ENABLED
				if (!(Engine::get_singleton()->is_editor_hint() && get_tree()->get_edited_scene_root() && (get_tree()->get_edited_scene_root()->is_ancestor_of(this) || get_tree()->get_edited_scene_root() == this))) {
					transient_parent->exclusive_child = this;
				}
#else
				transient_parent->exclusive_child = this;
#endif
			} else if (transient_parent->exclusive_child != this) {
				ERR_PRINT("Making child transient exclusive, but parent has another exclusive child");
			}
		}
	}

	//see if we can make transient
	if (transient_parent->window_id != DisplayServer::INVALID_WINDOW_ID && window_id != DisplayServer::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_transient(window_id, transient_parent->window_id);
	}
}

void Window::set_transient(bool p_transient) {
	if (transient == p_transient) {
		return;
	}

	transient = p_transient;

	if (!is_inside_tree()) {
		return;
	}

	if (transient) {
		_make_transient();
	} else {
		_clear_transient();
	}
}

bool Window::is_transient() const {
	return transient;
}

void Window::set_exclusive(bool p_exclusive) {
	if (exclusive == p_exclusive) {
		return;
	}

	exclusive = p_exclusive;

	if (!embedder && window_id != DisplayServer::INVALID_WINDOW_ID) {
#ifdef TOOLS_ENABLED
		if (!(Engine::get_singleton()->is_editor_hint() && get_tree()->get_edited_scene_root() && (get_tree()->get_edited_scene_root()->is_ancestor_of(this) || get_tree()->get_edited_scene_root() == this))) {
			DisplayServer::get_singleton()->window_set_exclusive(window_id, exclusive);
		} else {
			DisplayServer::get_singleton()->window_set_exclusive(window_id, false);
		}
#else
		DisplayServer::get_singleton()->window_set_exclusive(window_id, exclusive);
#endif
	}

	if (transient_parent) {
		if (p_exclusive && is_inside_tree() && is_visible()) {
			ERR_FAIL_COND_MSG(transient_parent->exclusive_child && transient_parent->exclusive_child != this, "Transient parent has another exclusive child.");
#ifdef TOOLS_ENABLED
			if (!(Engine::get_singleton()->is_editor_hint() && get_tree()->get_edited_scene_root() && (get_tree()->get_edited_scene_root()->is_ancestor_of(this) || get_tree()->get_edited_scene_root() == this))) {
				transient_parent->exclusive_child = this;
			}
#else
			transient_parent->exclusive_child = this;
#endif
		} else {
			if (transient_parent->exclusive_child == this) {
				transient_parent->exclusive_child = nullptr;
			}
		}
	}
}

bool Window::is_exclusive() const {
	return exclusive;
}

bool Window::is_visible() const {
	return visible;
}

void Window::_update_window_size() {
	Size2i size_limit;
	if (wrap_controls) {
		size_limit = get_contents_minimum_size();
	}

	size_limit.x = MAX(size_limit.x, min_size.x);
	size_limit.y = MAX(size_limit.y, min_size.y);

	size.x = MAX(size_limit.x, size.x);
	size.y = MAX(size_limit.y, size.y);

	bool reset_min_first = false;

	bool max_size_valid = false;
	if ((max_size.x > 0 || max_size.y > 0) && (max_size.x >= min_size.x && max_size.y >= min_size.y)) {
		max_size_valid = true;

		if (size.x > max_size.x) {
			size.x = max_size.x;
		}
		if (size_limit.x > max_size.x) {
			size_limit.x = max_size.x;
			reset_min_first = true;
		}

		if (size.y > max_size.y) {
			size.y = max_size.y;
		}
		if (size_limit.y > max_size.y) {
			size_limit.y = max_size.y;
			reset_min_first = true;
		}
	}

	if (embedder) {
		size.x = MAX(size.x, 1);
		size.y = MAX(size.y, 1);

		embedder->_sub_window_update(this);
	} else if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		if (reset_min_first && wrap_controls) {
			// Avoid an error if setting max_size to a value between min_size and the previous size_limit.
			DisplayServer::get_singleton()->window_set_min_size(Size2i(), window_id);
		}

		DisplayServer::get_singleton()->window_set_max_size(max_size_valid ? max_size : Size2i(), window_id);
		DisplayServer::get_singleton()->window_set_min_size(size_limit, window_id);
		DisplayServer::get_singleton()->window_set_size(size, window_id);
	}

	//update the viewport
	_update_viewport_size();
}

void Window::_update_viewport_size() {
	//update the viewport part

	Size2i final_size;
	Size2i final_size_override;
	Rect2i attach_to_screen_rect(Point2i(), size);
	Transform2D stretch_transform_new;
	float font_oversampling = 1.0;

	if (content_scale_mode == CONTENT_SCALE_MODE_DISABLED || content_scale_size.x == 0 || content_scale_size.y == 0) {
		font_oversampling = content_scale_factor;
		final_size = size;
		final_size_override = Size2(size) / content_scale_factor;

		stretch_transform_new = Transform2D();
		stretch_transform_new.scale(Size2(content_scale_factor, content_scale_factor));
	} else {
		//actual screen video mode
		Size2 video_mode = size;
		Size2 desired_res = content_scale_size;

		Size2 viewport_size;
		Size2 screen_size;

		float viewport_aspect = desired_res.aspect();
		float video_mode_aspect = video_mode.aspect();

		if (content_scale_aspect == CONTENT_SCALE_ASPECT_IGNORE || Math::is_equal_approx(viewport_aspect, video_mode_aspect)) {
			//same aspect or ignore aspect
			viewport_size = desired_res;
			screen_size = video_mode;
		} else if (viewport_aspect < video_mode_aspect) {
			// screen ratio is smaller vertically

			if (content_scale_aspect == CONTENT_SCALE_ASPECT_KEEP_HEIGHT || content_scale_aspect == CONTENT_SCALE_ASPECT_EXPAND) {
				//will stretch horizontally
				viewport_size.x = desired_res.y * video_mode_aspect;
				viewport_size.y = desired_res.y;
				screen_size = video_mode;

			} else {
				//will need black bars
				viewport_size = desired_res;
				screen_size.x = video_mode.y * viewport_aspect;
				screen_size.y = video_mode.y;
			}
		} else {
			//screen ratio is smaller horizontally
			if (content_scale_aspect == CONTENT_SCALE_ASPECT_KEEP_WIDTH || content_scale_aspect == CONTENT_SCALE_ASPECT_EXPAND) {
				//will stretch horizontally
				viewport_size.x = desired_res.x;
				viewport_size.y = desired_res.x / video_mode_aspect;
				screen_size = video_mode;

			} else {
				//will need black bars
				viewport_size = desired_res;
				screen_size.x = video_mode.x;
				screen_size.y = video_mode.x / viewport_aspect;
			}
		}

		screen_size = screen_size.floor();
		viewport_size = viewport_size.floor();

		Size2 margin;
		Size2 offset;
		//black bars and margin
		if (content_scale_aspect != CONTENT_SCALE_ASPECT_EXPAND && screen_size.x < video_mode.x) {
			margin.x = Math::round((video_mode.x - screen_size.x) / 2.0);
			//RenderingServer::get_singleton()->black_bars_set_margins(margin.x, 0, margin.x, 0);
			offset.x = Math::round(margin.x * viewport_size.y / screen_size.y);
		} else if (content_scale_aspect != CONTENT_SCALE_ASPECT_EXPAND && screen_size.y < video_mode.y) {
			margin.y = Math::round((video_mode.y - screen_size.y) / 2.0);
			//RenderingServer::get_singleton()->black_bars_set_margins(0, margin.y, 0, margin.y);
			offset.y = Math::round(margin.y * viewport_size.x / screen_size.x);
		} else {
			//RenderingServer::get_singleton()->black_bars_set_margins(0, 0, 0, 0);
		}

		switch (content_scale_mode) {
			case CONTENT_SCALE_MODE_DISABLED: {
				// Already handled above
				//_update_font_oversampling(1.0);
			} break;
			case CONTENT_SCALE_MODE_CANVAS_ITEMS: {
				final_size = screen_size;
				final_size_override = viewport_size / content_scale_factor;
				attach_to_screen_rect = Rect2(margin, screen_size);
				font_oversampling = (screen_size.x / viewport_size.x) * content_scale_factor;

				Size2 scale = Vector2(screen_size) / Vector2(final_size_override);
				stretch_transform_new.scale(scale);

			} break;
			case CONTENT_SCALE_MODE_VIEWPORT: {
				final_size = (viewport_size / content_scale_factor).floor();
				attach_to_screen_rect = Rect2(margin, screen_size);

			} break;
		}
	}

	bool allocate = is_inside_tree() && visible && (window_id != DisplayServer::INVALID_WINDOW_ID || embedder != nullptr);
	_set_size(final_size, final_size_override, attach_to_screen_rect, stretch_transform_new, allocate);

	if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		RenderingServer::get_singleton()->viewport_attach_to_screen(get_viewport_rid(), attach_to_screen_rect, window_id);
	} else {
		RenderingServer::get_singleton()->viewport_attach_to_screen(get_viewport_rid(), Rect2i(), DisplayServer::INVALID_WINDOW_ID);
	}

	if (window_id == DisplayServer::MAIN_WINDOW_ID) {
		if (!use_font_oversampling) {
			font_oversampling = 1.0;
		}
		if (TS->font_get_global_oversampling() != font_oversampling) {
			TS->font_set_global_oversampling(font_oversampling);
		}
	}

	notification(NOTIFICATION_WM_SIZE_CHANGED);

	if (embedder) {
		embedder->_sub_window_update(this);
	}
}

void Window::_update_window_callbacks() {
	DisplayServer::get_singleton()->window_set_rect_changed_callback(callable_mp(this, &Window::_rect_changed_callback), window_id);
	DisplayServer::get_singleton()->window_set_window_event_callback(callable_mp(this, &Window::_event_callback), window_id);
	DisplayServer::get_singleton()->window_set_input_event_callback(callable_mp(this, &Window::_window_input), window_id);
	DisplayServer::get_singleton()->window_set_input_text_callback(callable_mp(this, &Window::_window_input_text), window_id);
	DisplayServer::get_singleton()->window_set_drop_files_callback(callable_mp(this, &Window::_window_drop_files), window_id);
}

Viewport *Window::_get_embedder() const {
	Viewport *vp = get_parent_viewport();

	while (vp) {
		if (vp->is_embedding_subwindows()) {
			return vp;
		}

		if (vp->get_parent()) {
			vp = vp->get_parent()->get_viewport();
		} else {
			vp = nullptr;
		}
	}
	return nullptr;
}

void Window::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			_invalidate_theme_cache();
			_update_theme_item_cache();
		} break;

		case NOTIFICATION_PARENTED: {
			theme_owner->assign_theme_on_parented(this);
		} break;

		case NOTIFICATION_UNPARENTED: {
			theme_owner->clear_theme_on_unparented(this);
		} break;

		case NOTIFICATION_ENTER_TREE: {
			bool embedded = false;
			{
				embedder = _get_embedder();
				if (embedder) {
					embedded = true;
					if (!visible) {
						embedder = nullptr; // Not yet since not visible.
					}
				}
			}

			if (embedded) {
				// Create as embedded.
				if (embedder) {
					embedder->_sub_window_register(this);
					RS::get_singleton()->viewport_set_update_mode(get_viewport_rid(), RS::VIEWPORT_UPDATE_WHEN_PARENT_VISIBLE);
					_update_window_size();
				}

			} else {
				if (!get_parent()) {
					// It's the root window!
					visible = true; // Always visible.
					window_id = DisplayServer::MAIN_WINDOW_ID;
					DisplayServer::get_singleton()->window_attach_instance_id(get_instance_id(), window_id);
					_update_from_window();
					// Since this window already exists (created on start), we must update pos and size from it.
					{
						position = DisplayServer::get_singleton()->window_get_position(window_id);
						size = DisplayServer::get_singleton()->window_get_size(window_id);
					}
					_update_viewport_size(); // Then feed back to the viewport.
					_update_window_callbacks();
					RS::get_singleton()->viewport_set_update_mode(get_viewport_rid(), RS::VIEWPORT_UPDATE_WHEN_VISIBLE);
				} else {
					// Create.
					if (visible) {
						_make_window();
					}
				}
			}

			if (transient) {
				_make_transient();
			}
			if (visible) {
				notification(NOTIFICATION_VISIBILITY_CHANGED);
				emit_signal(SceneStringNames::get_singleton()->visibility_changed);
				RS::get_singleton()->viewport_set_active(get_viewport_rid(), true);
			}

			notification(NOTIFICATION_THEME_CHANGED);
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			emit_signal(SceneStringNames::get_singleton()->theme_changed);
			_invalidate_theme_cache();
			_update_theme_item_cache();
		} break;

		case NOTIFICATION_READY: {
			if (wrap_controls) {
				_update_child_controls();
			}
		} break;

		case NOTIFICATION_TRANSLATION_CHANGED: {
			_invalidate_theme_cache();
			_update_theme_item_cache();

			if (embedder) {
				embedder->_sub_window_update(this);
			} else if (window_id != DisplayServer::INVALID_WINDOW_ID) {
				String tr_title = atr(title);
#ifdef DEBUG_ENABLED
				if (window_id == DisplayServer::MAIN_WINDOW_ID) {
					// Append a suffix to the window title to denote that the project is running
					// from a debug build (including the editor). Since this results in lower performance,
					// this should be clearly presented to the user.
					tr_title = vformat("%s (DEBUG)", tr_title);
				}
#endif
				DisplayServer::get_singleton()->window_set_title(tr_title, window_id);
			}

			child_controls_changed();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (transient) {
				_clear_transient();
			}

			if (!is_embedded() && window_id != DisplayServer::INVALID_WINDOW_ID) {
				if (window_id == DisplayServer::MAIN_WINDOW_ID) {
					RS::get_singleton()->viewport_set_update_mode(get_viewport_rid(), RS::VIEWPORT_UPDATE_DISABLED);
					_update_window_callbacks();
				} else {
					_clear_window();
				}
			} else {
				if (embedder) {
					embedder->_sub_window_remove(this);
					embedder = nullptr;
					RS::get_singleton()->viewport_set_update_mode(get_viewport_rid(), RS::VIEWPORT_UPDATE_DISABLED);
				}
				_update_viewport_size(); //called by clear and make, which does not happen here
			}

			RS::get_singleton()->viewport_set_active(get_viewport_rid(), false);
		} break;
	}
}

void Window::set_content_scale_size(const Size2i &p_size) {
	ERR_FAIL_COND(p_size.x < 0);
	ERR_FAIL_COND(p_size.y < 0);
	content_scale_size = p_size;
	_update_viewport_size();
}

Size2i Window::get_content_scale_size() const {
	return content_scale_size;
}

void Window::set_content_scale_mode(ContentScaleMode p_mode) {
	content_scale_mode = p_mode;
	_update_viewport_size();
}

Window::ContentScaleMode Window::get_content_scale_mode() const {
	return content_scale_mode;
}

void Window::set_content_scale_aspect(ContentScaleAspect p_aspect) {
	content_scale_aspect = p_aspect;
	_update_viewport_size();
}

Window::ContentScaleAspect Window::get_content_scale_aspect() const {
	return content_scale_aspect;
}

void Window::set_content_scale_factor(real_t p_factor) {
	ERR_FAIL_COND(p_factor <= 0);
	content_scale_factor = p_factor;
	_update_viewport_size();
}

real_t Window::get_content_scale_factor() const {
	return content_scale_factor;
}

void Window::set_use_font_oversampling(bool p_oversampling) {
	if (is_inside_tree() && window_id != DisplayServer::MAIN_WINDOW_ID) {
		ERR_FAIL_MSG("Only the root window can set and use font oversampling.");
	}
	use_font_oversampling = p_oversampling;
	_update_viewport_size();
}

bool Window::is_using_font_oversampling() const {
	return use_font_oversampling;
}

DisplayServer::WindowID Window::get_window_id() const {
	if (embedder) {
		return parent->get_window_id();
	}
	return window_id;
}

void Window::set_wrap_controls(bool p_enable) {
	wrap_controls = p_enable;
	if (wrap_controls) {
		child_controls_changed();
	} else {
		_update_window_size();
	}
}

bool Window::is_wrapping_controls() const {
	return wrap_controls;
}

Size2 Window::_get_contents_minimum_size() const {
	Size2 max;

	for (int i = 0; i < get_child_count(); i++) {
		Control *c = Object::cast_to<Control>(get_child(i));
		if (c) {
			Point2i pos = c->get_position();
			Size2i min = c->get_combined_minimum_size();

			max.x = MAX(pos.x + min.x, max.x);
			max.y = MAX(pos.y + min.y, max.y);
		}
	}

	return max;
}

void Window::_update_child_controls() {
	if (!updating_child_controls) {
		return;
	}

	_update_window_size();

	updating_child_controls = false;
}

void Window::child_controls_changed() {
	if (!is_inside_tree() || !visible || updating_child_controls) {
		return;
	}

	updating_child_controls = true;
	call_deferred(SNAME("_update_child_controls"));
}

bool Window::_can_consume_input_events() const {
	return exclusive_child == nullptr;
}

void Window::_window_input(const Ref<InputEvent> &p_ev) {
	if (EngineDebugger::is_active()) {
		// Quit from game window using the stop shortcut (F8 by default).
		// The custom shortcut is provided via environment variable when running from the editor.
		if (debugger_stop_shortcut.is_null()) {
			String shortcut_str = OS::get_singleton()->get_environment("__GODOT_EDITOR_STOP_SHORTCUT__");
			if (!shortcut_str.is_empty()) {
				Variant shortcut_var;

				VariantParser::StreamString ss;
				ss.s = shortcut_str;

				String errs;
				int line;
				VariantParser::parse(&ss, shortcut_var, errs, line);
				debugger_stop_shortcut = shortcut_var;
			}

			if (debugger_stop_shortcut.is_null()) {
				// Define a default shortcut if it wasn't provided or is invalid.
				debugger_stop_shortcut.instantiate();
				debugger_stop_shortcut->set_events({ (Variant)InputEventKey::create_reference(Key::F8) });
			}
		}

		Ref<InputEventKey> k = p_ev;
		if (k.is_valid() && k->is_pressed() && !k->is_echo() && debugger_stop_shortcut->matches_event(k)) {
			EngineDebugger::get_singleton()->send_message("request_quit", Array());
		}
	}

	if (exclusive_child != nullptr) {
		if (!is_embedding_subwindows()) { // Not embedding, no need for event.
			return;
		}
	}

	emit_signal(SceneStringNames::get_singleton()->window_input, p_ev);

	push_input(p_ev);
	if (!is_input_handled()) {
		push_unhandled_input(p_ev);
	}
}

void Window::_window_input_text(const String &p_text) {
	push_text_input(p_text);
}

void Window::_window_drop_files(const Vector<String> &p_files) {
	emit_signal(SNAME("files_dropped"), p_files);
}

Viewport *Window::get_parent_viewport() const {
	if (get_parent()) {
		return get_parent()->get_viewport();
	} else {
		return nullptr;
	}
}

Window *Window::get_parent_visible_window() const {
	Viewport *vp = get_parent_viewport();
	Window *window = nullptr;
	while (vp) {
		window = Object::cast_to<Window>(vp);
		if (window && window->visible) {
			break;
		}
		if (!vp->get_parent()) {
			break;
		}

		vp = vp->get_parent()->get_viewport();
	}
	return window;
}

void Window::popup_on_parent(const Rect2i &p_parent_rect) {
	ERR_FAIL_COND(!is_inside_tree());
	ERR_FAIL_COND_MSG(window_id == DisplayServer::MAIN_WINDOW_ID, "Can't popup the main window.");

	if (!is_embedded()) {
		Window *window = get_parent_visible_window();

		if (!window) {
			popup(p_parent_rect);
		} else {
			popup(Rect2i(window->get_position() + p_parent_rect.position, p_parent_rect.size));
		}
	} else {
		popup(p_parent_rect);
	}
}

void Window::popup_centered_clamped(const Size2i &p_size, float p_fallback_ratio) {
	ERR_FAIL_COND(!is_inside_tree());
	ERR_FAIL_COND_MSG(window_id == DisplayServer::MAIN_WINDOW_ID, "Can't popup the main window.");

	Rect2 parent_rect;

	if (is_embedded()) {
		parent_rect = _get_embedder()->get_visible_rect();
	} else {
		DisplayServer::WindowID parent_id = get_parent_visible_window()->get_window_id();
		int parent_screen = DisplayServer::get_singleton()->window_get_current_screen(parent_id);
		parent_rect.position = DisplayServer::get_singleton()->screen_get_position(parent_screen);
		parent_rect.size = DisplayServer::get_singleton()->screen_get_size(parent_screen);
	}

	Vector2i size_ratio = parent_rect.size * p_fallback_ratio;

	Rect2i popup_rect;
	popup_rect.size = Vector2i(MIN(size_ratio.x, p_size.x), MIN(size_ratio.y, p_size.y));
	if (parent_rect != Rect2()) {
		popup_rect.position = parent_rect.position + (parent_rect.size - popup_rect.size) / 2;
	}

	popup(popup_rect);
}

void Window::popup_centered(const Size2i &p_minsize) {
	ERR_FAIL_COND(!is_inside_tree());
	ERR_FAIL_COND_MSG(window_id == DisplayServer::MAIN_WINDOW_ID, "Can't popup the main window.");

	Rect2 parent_rect;

	if (is_embedded()) {
		parent_rect = _get_embedder()->get_visible_rect();
	} else {
		DisplayServer::WindowID parent_id = get_parent_visible_window()->get_window_id();
		int parent_screen = DisplayServer::get_singleton()->window_get_current_screen(parent_id);
		parent_rect.position = DisplayServer::get_singleton()->screen_get_position(parent_screen);
		parent_rect.size = DisplayServer::get_singleton()->screen_get_size(parent_screen);
	}

	Rect2i popup_rect;
	Size2 contents_minsize = _get_contents_minimum_size();
	popup_rect.size.x = MAX(p_minsize.x, contents_minsize.x);
	popup_rect.size.y = MAX(p_minsize.y, contents_minsize.y);

	if (parent_rect != Rect2()) {
		popup_rect.position = parent_rect.position + (parent_rect.size - popup_rect.size) / 2;
	}

	popup(popup_rect);
}

void Window::popup_centered_ratio(float p_ratio) {
	ERR_FAIL_COND(!is_inside_tree());
	ERR_FAIL_COND_MSG(window_id == DisplayServer::MAIN_WINDOW_ID, "Can't popup the main window.");
	ERR_FAIL_COND_MSG(p_ratio <= 0.0 || p_ratio > 1.0, "Ratio must be between 0.0 and 1.0!");

	Rect2 parent_rect;

	if (is_embedded()) {
		parent_rect = _get_embedder()->get_visible_rect();
	} else {
		DisplayServer::WindowID parent_id = get_parent_visible_window()->get_window_id();
		int parent_screen = DisplayServer::get_singleton()->window_get_current_screen(parent_id);
		parent_rect.position = DisplayServer::get_singleton()->screen_get_position(parent_screen);
		parent_rect.size = DisplayServer::get_singleton()->screen_get_size(parent_screen);
	}

	Rect2i popup_rect;
	if (parent_rect != Rect2()) {
		popup_rect.size = parent_rect.size * p_ratio;
		popup_rect.position = parent_rect.position + (parent_rect.size - popup_rect.size) / 2;
	}

	popup(popup_rect);
}

void Window::popup(const Rect2i &p_screen_rect) {
	emit_signal(SNAME("about_to_popup"));

	if (!_get_embedder() && get_flag(FLAG_POPUP)) {
		// Send a focus-out notification when opening a Window Manager Popup.
		SceneTree *scene_tree = get_tree();
		if (scene_tree) {
			scene_tree->notify_group_flags(SceneTree::GROUP_CALL_DEFERRED, "_viewports", NOTIFICATION_WM_WINDOW_FOCUS_OUT);
		}
	}

	// Update window size to calculate the actual window size based on contents minimum size and minimum size.
	_update_window_size();

	if (p_screen_rect != Rect2i()) {
		set_position(p_screen_rect.position);
		set_size(p_screen_rect.size);
	}

	Rect2i adjust = _popup_adjust_rect();
	if (adjust != Rect2i()) {
		set_position(adjust.position);
		set_size(adjust.size);
	}

	int scr = DisplayServer::get_singleton()->get_screen_count();
	for (int i = 0; i < scr; i++) {
		Rect2i r = DisplayServer::get_singleton()->screen_get_usable_rect(i);
		if (r.has_point(position)) {
			current_screen = i;
			break;
		}
	}

	set_transient(true);
	set_visible(true);

	Rect2i parent_rect;
	if (is_embedded()) {
		parent_rect = _get_embedder()->get_visible_rect();
	} else {
		int screen_id = DisplayServer::get_singleton()->window_get_current_screen(get_window_id());
		parent_rect = DisplayServer::get_singleton()->screen_get_usable_rect(screen_id);
	}
	if (parent_rect != Rect2i() && !parent_rect.intersects(Rect2i(position, size))) {
		ERR_PRINT(vformat("Window %d spawned at invalid position: %s.", get_window_id(), position));
		set_position((parent_rect.size - size) / 2);
	}

	_post_popup();
	notification(NOTIFICATION_POST_POPUP);
}

Size2 Window::get_contents_minimum_size() const {
	return _get_contents_minimum_size();
}

void Window::grab_focus() {
	if (embedder) {
		embedder->_sub_window_grab_focus(this);
	} else if (window_id != DisplayServer::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_move_to_foreground(window_id);
	}
}

bool Window::has_focus() const {
	return focused;
}

Rect2i Window::get_usable_parent_rect() const {
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2());
	Rect2i parent_rect;
	if (is_embedded()) {
		parent_rect = _get_embedder()->get_visible_rect();
	} else {
		const Window *w = is_visible() ? this : get_parent_visible_window();
		//find a parent that can contain us
		ERR_FAIL_COND_V(!w, Rect2());

		parent_rect = DisplayServer::get_singleton()->screen_get_usable_rect(DisplayServer::get_singleton()->window_get_current_screen(w->get_window_id()));
	}
	return parent_rect;
}

void Window::add_child_notify(Node *p_child) {
	if (is_inside_tree() && wrap_controls) {
		child_controls_changed();
	}
}

void Window::remove_child_notify(Node *p_child) {
	if (is_inside_tree() && wrap_controls) {
		child_controls_changed();
	}
}

void Window::set_theme_owner_node(Node *p_node) {
	theme_owner->set_owner_node(p_node);
}

Node *Window::get_theme_owner_node() const {
	return theme_owner->get_owner_node();
}

bool Window::has_theme_owner_node() const {
	return theme_owner->has_owner_node();
}

void Window::set_theme(const Ref<Theme> &p_theme) {
	if (theme == p_theme) {
		return;
	}

	if (theme.is_valid()) {
		theme->disconnect("changed", callable_mp(this, &Window::_theme_changed));
	}

	theme = p_theme;
	if (theme.is_valid()) {
		theme_owner->propagate_theme_changed(this, this, is_inside_tree(), true);
		theme->connect("changed", callable_mp(this, &Window::_theme_changed), CONNECT_DEFERRED);
		return;
	}

	Control *parent_c = Object::cast_to<Control>(get_parent());
	if (parent_c && parent_c->has_theme_owner_node()) {
		theme_owner->propagate_theme_changed(this, parent_c->get_theme_owner_node(), is_inside_tree(), true);
		return;
	}

	Window *parent_w = cast_to<Window>(get_parent());
	if (parent_w && parent_w->has_theme_owner_node()) {
		theme_owner->propagate_theme_changed(this, parent_w->get_theme_owner_node(), is_inside_tree(), true);
		return;
	}

	theme_owner->propagate_theme_changed(this, nullptr, is_inside_tree(), true);
}

Ref<Theme> Window::get_theme() const {
	return theme;
}

void Window::_theme_changed() {
	if (is_inside_tree()) {
		theme_owner->propagate_theme_changed(this, this, true, false);
	}
}

void Window::_invalidate_theme_cache() {
	theme_icon_cache.clear();
	theme_style_cache.clear();
	theme_font_cache.clear();
	theme_font_size_cache.clear();
	theme_color_cache.clear();
	theme_constant_cache.clear();
}

void Window::_update_theme_item_cache() {
}

void Window::set_theme_type_variation(const StringName &p_theme_type) {
	theme_type_variation = p_theme_type;
	if (is_inside_tree()) {
		notification(NOTIFICATION_THEME_CHANGED);
	}
}

StringName Window::get_theme_type_variation() const {
	return theme_type_variation;
}

Ref<Texture2D> Window::get_theme_icon(const StringName &p_name, const StringName &p_theme_type) const {
	if (theme_icon_cache.has(p_theme_type) && theme_icon_cache[p_theme_type].has(p_name)) {
		return theme_icon_cache[p_theme_type][p_name];
	}

	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	Ref<Texture2D> icon = theme_owner->get_theme_item_in_types(Theme::DATA_TYPE_ICON, p_name, theme_types);
	theme_icon_cache[p_theme_type][p_name] = icon;
	return icon;
}

Ref<StyleBox> Window::get_theme_stylebox(const StringName &p_name, const StringName &p_theme_type) const {
	if (theme_style_cache.has(p_theme_type) && theme_style_cache[p_theme_type].has(p_name)) {
		return theme_style_cache[p_theme_type][p_name];
	}

	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	Ref<StyleBox> style = theme_owner->get_theme_item_in_types(Theme::DATA_TYPE_STYLEBOX, p_name, theme_types);
	theme_style_cache[p_theme_type][p_name] = style;
	return style;
}

Ref<Font> Window::get_theme_font(const StringName &p_name, const StringName &p_theme_type) const {
	if (theme_font_cache.has(p_theme_type) && theme_font_cache[p_theme_type].has(p_name)) {
		return theme_font_cache[p_theme_type][p_name];
	}

	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	Ref<Font> font = theme_owner->get_theme_item_in_types(Theme::DATA_TYPE_FONT, p_name, theme_types);
	theme_font_cache[p_theme_type][p_name] = font;
	return font;
}

int Window::get_theme_font_size(const StringName &p_name, const StringName &p_theme_type) const {
	if (theme_font_size_cache.has(p_theme_type) && theme_font_size_cache[p_theme_type].has(p_name)) {
		return theme_font_size_cache[p_theme_type][p_name];
	}

	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	int font_size = theme_owner->get_theme_item_in_types(Theme::DATA_TYPE_FONT_SIZE, p_name, theme_types);
	theme_font_size_cache[p_theme_type][p_name] = font_size;
	return font_size;
}

Color Window::get_theme_color(const StringName &p_name, const StringName &p_theme_type) const {
	if (theme_color_cache.has(p_theme_type) && theme_color_cache[p_theme_type].has(p_name)) {
		return theme_color_cache[p_theme_type][p_name];
	}

	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	Color color = theme_owner->get_theme_item_in_types(Theme::DATA_TYPE_COLOR, p_name, theme_types);
	theme_color_cache[p_theme_type][p_name] = color;
	return color;
}

int Window::get_theme_constant(const StringName &p_name, const StringName &p_theme_type) const {
	if (theme_constant_cache.has(p_theme_type) && theme_constant_cache[p_theme_type].has(p_name)) {
		return theme_constant_cache[p_theme_type][p_name];
	}

	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	int constant = theme_owner->get_theme_item_in_types(Theme::DATA_TYPE_CONSTANT, p_name, theme_types);
	theme_constant_cache[p_theme_type][p_name] = constant;
	return constant;
}

bool Window::has_theme_icon(const StringName &p_name, const StringName &p_theme_type) const {
	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	return theme_owner->has_theme_item_in_types(Theme::DATA_TYPE_ICON, p_name, theme_types);
}

bool Window::has_theme_stylebox(const StringName &p_name, const StringName &p_theme_type) const {
	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	return theme_owner->has_theme_item_in_types(Theme::DATA_TYPE_STYLEBOX, p_name, theme_types);
}

bool Window::has_theme_font(const StringName &p_name, const StringName &p_theme_type) const {
	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	return theme_owner->has_theme_item_in_types(Theme::DATA_TYPE_FONT, p_name, theme_types);
}

bool Window::has_theme_font_size(const StringName &p_name, const StringName &p_theme_type) const {
	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	return theme_owner->has_theme_item_in_types(Theme::DATA_TYPE_FONT_SIZE, p_name, theme_types);
}

bool Window::has_theme_color(const StringName &p_name, const StringName &p_theme_type) const {
	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	return theme_owner->has_theme_item_in_types(Theme::DATA_TYPE_COLOR, p_name, theme_types);
}

bool Window::has_theme_constant(const StringName &p_name, const StringName &p_theme_type) const {
	List<StringName> theme_types;
	theme_owner->get_theme_type_dependencies(this, p_theme_type, &theme_types);
	return theme_owner->has_theme_item_in_types(Theme::DATA_TYPE_CONSTANT, p_name, theme_types);
}

float Window::get_theme_default_base_scale() const {
	return theme_owner->get_theme_default_base_scale();
}

Ref<Font> Window::get_theme_default_font() const {
	return theme_owner->get_theme_default_font();
}

int Window::get_theme_default_font_size() const {
	return theme_owner->get_theme_default_font_size();
}

Rect2i Window::get_parent_rect() const {
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2i());
	if (is_embedded()) {
		//viewport
		Node *n = get_parent();
		ERR_FAIL_COND_V(!n, Rect2i());
		Viewport *p = n->get_viewport();
		ERR_FAIL_COND_V(!p, Rect2i());

		return p->get_visible_rect();
	} else {
		int x = get_position().x;
		int closest_dist = 0x7FFFFFFF;
		Rect2i closest_rect;
		for (int i = 0; i < DisplayServer::get_singleton()->get_screen_count(); i++) {
			Rect2i s(DisplayServer::get_singleton()->screen_get_position(i), DisplayServer::get_singleton()->screen_get_size(i));
			int d;
			if (x >= s.position.x && x < s.size.x) {
				//contained
				closest_rect = s;
				break;
			} else if (x < s.position.x) {
				d = s.position.x - x;
			} else {
				d = x - (s.position.x + s.size.x);
			}

			if (d < closest_dist) {
				closest_dist = d;
				closest_rect = s;
			}
		}
		return closest_rect;
	}
}

void Window::set_clamp_to_embedder(bool p_enable) {
	clamp_to_embedder = p_enable;
}

bool Window::is_clamped_to_embedder() const {
	return clamp_to_embedder;
}

void Window::set_layout_direction(Window::LayoutDirection p_direction) {
	ERR_FAIL_INDEX((int)p_direction, 4);

	layout_dir = p_direction;
	propagate_notification(Control::NOTIFICATION_LAYOUT_DIRECTION_CHANGED);
}

Window::LayoutDirection Window::get_layout_direction() const {
	return layout_dir;
}

bool Window::is_layout_rtl() const {
	if (layout_dir == LAYOUT_DIRECTION_INHERITED) {
		Window *parent_w = Object::cast_to<Window>(get_parent());
		if (parent_w) {
			return parent_w->is_layout_rtl();
		} else {
			if (GLOBAL_GET(SNAME("internationalization/rendering/force_right_to_left_layout_direction"))) {
				return true;
			}
			String locale = TranslationServer::get_singleton()->get_tool_locale();
			return TS->is_locale_right_to_left(locale);
		}
	} else if (layout_dir == LAYOUT_DIRECTION_LOCALE) {
		if (GLOBAL_GET(SNAME("internationalization/rendering/force_right_to_left_layout_direction"))) {
			return true;
		}
		String locale = TranslationServer::get_singleton()->get_tool_locale();
		return TS->is_locale_right_to_left(locale);
	} else {
		return (layout_dir == LAYOUT_DIRECTION_RTL);
	}
}

void Window::set_auto_translate(bool p_enable) {
	if (p_enable == auto_translate) {
		return;
	}

	auto_translate = p_enable;

	notification(MainLoop::NOTIFICATION_TRANSLATION_CHANGED);
}

bool Window::is_auto_translating() const {
	return auto_translate;
}

void Window::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name == "theme_type_variation") {
		List<StringName> names;

		// Only the default theme and the project theme are used for the list of options.
		// This is an imposed limitation to simplify the logic needed to leverage those options.
		ThemeDB::get_singleton()->get_default_theme()->get_type_variation_list(get_class_name(), &names);
		if (ThemeDB::get_singleton()->get_project_theme().is_valid()) {
			ThemeDB::get_singleton()->get_project_theme()->get_type_variation_list(get_class_name(), &names);
		}
		names.sort_custom<StringName::AlphCompare>();

		Vector<StringName> unique_names;
		String hint_string;
		for (const StringName &E : names) {
			// Skip duplicate values.
			if (unique_names.has(E)) {
				continue;
			}

			hint_string += String(E) + ",";
			unique_names.append(E);
		}

		p_property.hint_string = hint_string;
	}
}

Transform2D Window::get_screen_transform() const {
	Transform2D embedder_transform = Transform2D();
	if (_get_embedder()) {
		embedder_transform.translate_local(get_position());
		embedder_transform = _get_embedder()->get_screen_transform() * embedder_transform;
	}
	return embedder_transform * Viewport::get_screen_transform();
}

void Window::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_title", "title"), &Window::set_title);
	ClassDB::bind_method(D_METHOD("get_title"), &Window::get_title);

	ClassDB::bind_method(D_METHOD("set_current_screen", "index"), &Window::set_current_screen);
	ClassDB::bind_method(D_METHOD("get_current_screen"), &Window::get_current_screen);

	ClassDB::bind_method(D_METHOD("set_position", "position"), &Window::set_position);
	ClassDB::bind_method(D_METHOD("get_position"), &Window::get_position);

	ClassDB::bind_method(D_METHOD("set_size", "size"), &Window::set_size);
	ClassDB::bind_method(D_METHOD("get_size"), &Window::get_size);
	ClassDB::bind_method(D_METHOD("reset_size"), &Window::reset_size);

	ClassDB::bind_method(D_METHOD("get_real_size"), &Window::get_real_size);

	ClassDB::bind_method(D_METHOD("set_max_size", "max_size"), &Window::set_max_size);
	ClassDB::bind_method(D_METHOD("get_max_size"), &Window::get_max_size);

	ClassDB::bind_method(D_METHOD("set_min_size", "min_size"), &Window::set_min_size);
	ClassDB::bind_method(D_METHOD("get_min_size"), &Window::get_min_size);

	ClassDB::bind_method(D_METHOD("set_mode", "mode"), &Window::set_mode);
	ClassDB::bind_method(D_METHOD("get_mode"), &Window::get_mode);

	ClassDB::bind_method(D_METHOD("set_flag", "flag", "enabled"), &Window::set_flag);
	ClassDB::bind_method(D_METHOD("get_flag", "flag"), &Window::get_flag);

	ClassDB::bind_method(D_METHOD("is_maximize_allowed"), &Window::is_maximize_allowed);

	ClassDB::bind_method(D_METHOD("request_attention"), &Window::request_attention);

	ClassDB::bind_method(D_METHOD("move_to_foreground"), &Window::move_to_foreground);

	ClassDB::bind_method(D_METHOD("set_visible", "visible"), &Window::set_visible);
	ClassDB::bind_method(D_METHOD("is_visible"), &Window::is_visible);

	ClassDB::bind_method(D_METHOD("hide"), &Window::hide);
	ClassDB::bind_method(D_METHOD("show"), &Window::show);

	ClassDB::bind_method(D_METHOD("set_transient", "transient"), &Window::set_transient);
	ClassDB::bind_method(D_METHOD("is_transient"), &Window::is_transient);

	ClassDB::bind_method(D_METHOD("set_exclusive", "exclusive"), &Window::set_exclusive);
	ClassDB::bind_method(D_METHOD("is_exclusive"), &Window::is_exclusive);

	ClassDB::bind_method(D_METHOD("can_draw"), &Window::can_draw);
	ClassDB::bind_method(D_METHOD("has_focus"), &Window::has_focus);
	ClassDB::bind_method(D_METHOD("grab_focus"), &Window::grab_focus);

	ClassDB::bind_method(D_METHOD("set_ime_active", "active"), &Window::set_ime_active);
	ClassDB::bind_method(D_METHOD("set_ime_position", "position"), &Window::set_ime_position);

	ClassDB::bind_method(D_METHOD("is_embedded"), &Window::is_embedded);

	ClassDB::bind_method(D_METHOD("get_contents_minimum_size"), &Window::get_contents_minimum_size);

	ClassDB::bind_method(D_METHOD("set_content_scale_size", "size"), &Window::set_content_scale_size);
	ClassDB::bind_method(D_METHOD("get_content_scale_size"), &Window::get_content_scale_size);

	ClassDB::bind_method(D_METHOD("set_content_scale_mode", "mode"), &Window::set_content_scale_mode);
	ClassDB::bind_method(D_METHOD("get_content_scale_mode"), &Window::get_content_scale_mode);

	ClassDB::bind_method(D_METHOD("set_content_scale_aspect", "aspect"), &Window::set_content_scale_aspect);
	ClassDB::bind_method(D_METHOD("get_content_scale_aspect"), &Window::get_content_scale_aspect);

	ClassDB::bind_method(D_METHOD("set_content_scale_factor", "factor"), &Window::set_content_scale_factor);
	ClassDB::bind_method(D_METHOD("get_content_scale_factor"), &Window::get_content_scale_factor);

	ClassDB::bind_method(D_METHOD("set_use_font_oversampling", "enable"), &Window::set_use_font_oversampling);
	ClassDB::bind_method(D_METHOD("is_using_font_oversampling"), &Window::is_using_font_oversampling);

	ClassDB::bind_method(D_METHOD("set_wrap_controls", "enable"), &Window::set_wrap_controls);
	ClassDB::bind_method(D_METHOD("is_wrapping_controls"), &Window::is_wrapping_controls);
	ClassDB::bind_method(D_METHOD("child_controls_changed"), &Window::child_controls_changed);

	ClassDB::bind_method(D_METHOD("_update_child_controls"), &Window::_update_child_controls);

	ClassDB::bind_method(D_METHOD("set_theme", "theme"), &Window::set_theme);
	ClassDB::bind_method(D_METHOD("get_theme"), &Window::get_theme);

	ClassDB::bind_method(D_METHOD("set_theme_type_variation", "theme_type"), &Window::set_theme_type_variation);
	ClassDB::bind_method(D_METHOD("get_theme_type_variation"), &Window::get_theme_type_variation);

	ClassDB::bind_method(D_METHOD("get_theme_icon", "name", "theme_type"), &Window::get_theme_icon, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_theme_stylebox", "name", "theme_type"), &Window::get_theme_stylebox, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_theme_font", "name", "theme_type"), &Window::get_theme_font, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_theme_font_size", "name", "theme_type"), &Window::get_theme_font_size, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_theme_color", "name", "theme_type"), &Window::get_theme_color, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_theme_constant", "name", "theme_type"), &Window::get_theme_constant, DEFVAL(""));

	ClassDB::bind_method(D_METHOD("has_theme_icon", "name", "theme_type"), &Window::has_theme_icon, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("has_theme_stylebox", "name", "theme_type"), &Window::has_theme_stylebox, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("has_theme_font", "name", "theme_type"), &Window::has_theme_font, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("has_theme_font_size", "name", "theme_type"), &Window::has_theme_font_size, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("has_theme_color", "name", "theme_type"), &Window::has_theme_color, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("has_theme_constant", "name", "theme_type"), &Window::has_theme_constant, DEFVAL(""));

	ClassDB::bind_method(D_METHOD("get_theme_default_base_scale"), &Window::get_theme_default_base_scale);
	ClassDB::bind_method(D_METHOD("get_theme_default_font"), &Window::get_theme_default_font);
	ClassDB::bind_method(D_METHOD("get_theme_default_font_size"), &Window::get_theme_default_font_size);

	ClassDB::bind_method(D_METHOD("set_layout_direction", "direction"), &Window::set_layout_direction);
	ClassDB::bind_method(D_METHOD("get_layout_direction"), &Window::get_layout_direction);
	ClassDB::bind_method(D_METHOD("is_layout_rtl"), &Window::is_layout_rtl);

	ClassDB::bind_method(D_METHOD("set_auto_translate", "enable"), &Window::set_auto_translate);
	ClassDB::bind_method(D_METHOD("is_auto_translating"), &Window::is_auto_translating);

	ClassDB::bind_method(D_METHOD("popup", "rect"), &Window::popup, DEFVAL(Rect2i()));
	ClassDB::bind_method(D_METHOD("popup_on_parent", "parent_rect"), &Window::popup_on_parent);
	ClassDB::bind_method(D_METHOD("popup_centered_ratio", "ratio"), &Window::popup_centered_ratio, DEFVAL(0.8));
	ClassDB::bind_method(D_METHOD("popup_centered", "minsize"), &Window::popup_centered, DEFVAL(Size2i()));
	ClassDB::bind_method(D_METHOD("popup_centered_clamped", "minsize", "fallback_ratio"), &Window::popup_centered_clamped, DEFVAL(Size2i()), DEFVAL(0.75));

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "position", PROPERTY_HINT_NONE, "suffix:px"), "set_position", "get_position");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "size", PROPERTY_HINT_NONE, "suffix:px"), "set_size", "get_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Windowed,Minimized,Maximized,Fullscreen"), "set_mode", "get_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "current_screen"), "set_current_screen", "get_current_screen");

	ADD_GROUP("Flags", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "visible"), "set_visible", "is_visible");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wrap_controls"), "set_wrap_controls", "is_wrapping_controls");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "transient"), "set_transient", "is_transient");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "exclusive"), "set_exclusive", "is_exclusive");
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "unresizable"), "set_flag", "get_flag", FLAG_RESIZE_DISABLED);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "borderless"), "set_flag", "get_flag", FLAG_BORDERLESS);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "always_on_top"), "set_flag", "get_flag", FLAG_ALWAYS_ON_TOP);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "transparent"), "set_flag", "get_flag", FLAG_TRANSPARENT);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "unfocusable"), "set_flag", "get_flag", FLAG_NO_FOCUS);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "popup_window"), "set_flag", "get_flag", FLAG_POPUP);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "extend_to_title"), "set_flag", "get_flag", FLAG_EXTEND_TO_TITLE);

	ADD_GROUP("Limits", "");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "min_size", PROPERTY_HINT_NONE, "suffix:px"), "set_min_size", "get_min_size");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "max_size", PROPERTY_HINT_NONE, "suffix:px"), "set_max_size", "get_max_size");

	ADD_GROUP("Content Scale", "content_scale_");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "content_scale_size"), "set_content_scale_size", "get_content_scale_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "content_scale_mode", PROPERTY_HINT_ENUM, "Disabled,Canvas Items,Viewport"), "set_content_scale_mode", "get_content_scale_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "content_scale_aspect", PROPERTY_HINT_ENUM, "Ignore,Keep,Keep Width,Keep Height,Expand"), "set_content_scale_aspect", "get_content_scale_aspect");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "content_scale_factor"), "set_content_scale_factor", "get_content_scale_factor");

	ADD_GROUP("Theme", "theme_");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "theme", PROPERTY_HINT_RESOURCE_TYPE, "Theme"), "set_theme", "get_theme");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "theme_type_variation", PROPERTY_HINT_ENUM_SUGGESTION), "set_theme_type_variation", "get_theme_type_variation");

	ADD_GROUP("Auto Translate", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_translate"), "set_auto_translate", "is_auto_translating");

	ADD_SIGNAL(MethodInfo("window_input", PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_RESOURCE_TYPE, "InputEvent")));
	ADD_SIGNAL(MethodInfo("files_dropped", PropertyInfo(Variant::PACKED_STRING_ARRAY, "files")));
	ADD_SIGNAL(MethodInfo("mouse_entered"));
	ADD_SIGNAL(MethodInfo("mouse_exited"));
	ADD_SIGNAL(MethodInfo("focus_entered"));
	ADD_SIGNAL(MethodInfo("focus_exited"));
	ADD_SIGNAL(MethodInfo("close_requested"));
	ADD_SIGNAL(MethodInfo("go_back_requested"));
	ADD_SIGNAL(MethodInfo("visibility_changed"));
	ADD_SIGNAL(MethodInfo("about_to_popup"));
	ADD_SIGNAL(MethodInfo("theme_changed"));
	ADD_SIGNAL(MethodInfo("titlebar_changed"));

	BIND_CONSTANT(NOTIFICATION_VISIBILITY_CHANGED);
	BIND_CONSTANT(NOTIFICATION_THEME_CHANGED);
	BIND_CONSTANT(NOTIFICATION_POST_POPUP);

	BIND_ENUM_CONSTANT(MODE_WINDOWED);
	BIND_ENUM_CONSTANT(MODE_MINIMIZED);
	BIND_ENUM_CONSTANT(MODE_MAXIMIZED);
	BIND_ENUM_CONSTANT(MODE_FULLSCREEN);
	BIND_ENUM_CONSTANT(MODE_EXCLUSIVE_FULLSCREEN);

	BIND_ENUM_CONSTANT(FLAG_RESIZE_DISABLED);
	BIND_ENUM_CONSTANT(FLAG_BORDERLESS);
	BIND_ENUM_CONSTANT(FLAG_ALWAYS_ON_TOP);
	BIND_ENUM_CONSTANT(FLAG_TRANSPARENT);
	BIND_ENUM_CONSTANT(FLAG_NO_FOCUS);
	BIND_ENUM_CONSTANT(FLAG_POPUP);
	BIND_ENUM_CONSTANT(FLAG_EXTEND_TO_TITLE);
	BIND_ENUM_CONSTANT(FLAG_MAX);

	BIND_ENUM_CONSTANT(CONTENT_SCALE_MODE_DISABLED);
	BIND_ENUM_CONSTANT(CONTENT_SCALE_MODE_CANVAS_ITEMS);
	BIND_ENUM_CONSTANT(CONTENT_SCALE_MODE_VIEWPORT);

	BIND_ENUM_CONSTANT(CONTENT_SCALE_ASPECT_IGNORE);
	BIND_ENUM_CONSTANT(CONTENT_SCALE_ASPECT_KEEP);
	BIND_ENUM_CONSTANT(CONTENT_SCALE_ASPECT_KEEP_WIDTH);
	BIND_ENUM_CONSTANT(CONTENT_SCALE_ASPECT_KEEP_HEIGHT);
	BIND_ENUM_CONSTANT(CONTENT_SCALE_ASPECT_EXPAND);

	BIND_ENUM_CONSTANT(LAYOUT_DIRECTION_INHERITED);
	BIND_ENUM_CONSTANT(LAYOUT_DIRECTION_LOCALE);
	BIND_ENUM_CONSTANT(LAYOUT_DIRECTION_LTR);
	BIND_ENUM_CONSTANT(LAYOUT_DIRECTION_RTL);
}

Window::Window() {
	theme_owner = memnew(ThemeOwner);
	RS::get_singleton()->viewport_set_update_mode(get_viewport_rid(), RS::VIEWPORT_UPDATE_DISABLED);
}

Window::~Window() {
	memdelete(theme_owner);
}
