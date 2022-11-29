/*************************************************************************/
/*  vr_editor.cpp                                                        */
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

#include "vr_editor.h"

#include "editor/filesystem_dock.h"
#include "editor/import_dock.h"
#include "editor/inspector_dock.h"
#include "editor/node_dock.h"
#include "editor/scene_tree_dock.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr_server.h"

EditorNode *VREditor::init_editor(SceneTree *p_scene_tree) {
	Ref<XRInterface> xr_interface = XRServer::get_singleton()->find_interface("OpenXR");
	if (xr_interface.is_valid() && xr_interface->is_initialized()) {
		Viewport *vp = p_scene_tree->get_root();

		// Make sure V-Sync is OFF or our monitor frequency will limit our headset
		// TODO improve this to only override v-sync when the player is wearing the headset
		DisplayServer::get_singleton()->window_set_vsync_mode(DisplayServer::VSYNC_DISABLED);

		// Enable our viewport for VR use
		vp->set_vrs_mode(Viewport::VRS_XR);
		vp->set_use_xr(true);

		// Now add our VR editor
		VREditor *vr_pm = memnew(VREditor(vp));
		vp->add_child(vr_pm);

		return vr_pm->get_editor_node();
	} else {
		// Our XR interface was never setup properly, for now assume this means we're not running in XR mode
		// TODO improve this so if we were meant to be in XR mode but failed, especially if we're stand alone, we should hard exit.
		return nullptr;
	}
}

VREditor::VREditor(Viewport *p_xr_viewport) {
	xr_viewport = p_xr_viewport;

	// Make sure our editor settings are loaded...
	if (!EditorSettings::get_singleton()) {
		EditorSettings::create();
	}

	// Add our avatar
	avatar = memnew(VREditorAvatar);
	xr_viewport->add_child(avatar);

	// Create a window to add our normal editor in
	editor_window = memnew(VRWindow(Size2i(2000.0, 1000.0), 0.0015));
	editor_window->set_name("Editor");
	editor_window->set_curve_depth(0.2);
	editor_window->set_position(Vector3(0.0, 0.0, -1.0));
	avatar->add_window_to_hud(editor_window);

	editor_node = memnew(EditorNode);
	editor_window->get_scene_root()->add_child(editor_node);
}

VREditor::~VREditor() {
}
