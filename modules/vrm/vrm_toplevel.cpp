/**************************************************************************/
/*  vrm_toplevel.cpp                                                      */
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

#include "vrm_toplevel.h"
#include "scene/main/node.h"

void VRMTopLevel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_vrm_skeleton", "path"), &VRMTopLevel::set_vrm_skeleton);
	ClassDB::bind_method(D_METHOD("get_vrm_skeleton"), &VRMTopLevel::get_vrm_skeleton);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "vrm_skeleton"), "set_vrm_skeleton", "get_vrm_skeleton");

	ClassDB::bind_method(D_METHOD("set_vrm_animplayer", "path"), &VRMTopLevel::set_vrm_animplayer);
	ClassDB::bind_method(D_METHOD("get_vrm_animplayer"), &VRMTopLevel::get_vrm_animplayer);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "vrm_animplayer"), "set_vrm_animplayer", "get_vrm_animplayer");

	ClassDB::bind_method(D_METHOD("set_vrm_meta", "meta"), &VRMTopLevel::set_vrm_meta);
	ClassDB::bind_method(D_METHOD("get_vrm_meta"), &VRMTopLevel::get_vrm_meta);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "vrm_meta", PROPERTY_HINT_RESOURCE_TYPE, "Resource"), "set_vrm_meta", "get_vrm_meta");
}

void VRMTopLevel::set_vrm_skeleton(const NodePath &path) {
	vrm_skeleton = path;
}

NodePath VRMTopLevel::get_vrm_skeleton() const {
	return vrm_skeleton;
}

void VRMTopLevel::set_vrm_animplayer(const NodePath &path) {
	vrm_animplayer = path;
}

NodePath VRMTopLevel::get_vrm_animplayer() const {
	return vrm_animplayer;
}

void VRMTopLevel::set_vrm_meta(const Ref<Resource> &meta) {
	vrm_meta = meta;
}

Ref<Resource> VRMTopLevel::get_vrm_meta() const {
	return vrm_meta;
}
