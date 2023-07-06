/**************************************************************************/
/*  vrm_meta.cpp                                                          */
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

#include "vrm_meta.h"

void VRMMeta::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_title"), &VRMMeta::get_title);
	ClassDB::bind_method(D_METHOD("set_title", "title"), &VRMMeta::set_title);
	ClassDB::bind_method(D_METHOD("get_version"), &VRMMeta::get_version);
	ClassDB::bind_method(D_METHOD("set_version", "version"), &VRMMeta::set_version);
	ClassDB::bind_method(D_METHOD("get_author"), &VRMMeta::get_author);
	ClassDB::bind_method(D_METHOD("set_author", "author"), &VRMMeta::set_author);
	ClassDB::bind_method(D_METHOD("get_contact_information"), &VRMMeta::get_contact_information);
	ClassDB::bind_method(D_METHOD("set_contact_information", "contact_information"), &VRMMeta::set_contact_information);
	ClassDB::bind_method(D_METHOD("get_reference_information"), &VRMMeta::get_reference_information);
	ClassDB::bind_method(D_METHOD("set_reference_information", "reference_information"), &VRMMeta::set_reference_information);
	ClassDB::bind_method(D_METHOD("get_texture"), &VRMMeta::get_texture);
	ClassDB::bind_method(D_METHOD("set_texture", "texture"), &VRMMeta::set_texture);
	ClassDB::bind_method(D_METHOD("get_allowed_user_name"), &VRMMeta::get_allowed_user_name);
	ClassDB::bind_method(D_METHOD("set_allowed_user_name", "allowed_user_name"), &VRMMeta::set_allowed_user_name);
	ClassDB::bind_method(D_METHOD("get_violent_usage"), &VRMMeta::get_violent_usage);
	ClassDB::bind_method(D_METHOD("set_violent_usage", "violent_usage"), &VRMMeta::set_violent_usage);
	ClassDB::bind_method(D_METHOD("get_sexual_usage"), &VRMMeta::get_sexual_usage);
	ClassDB::bind_method(D_METHOD("set_sexual_usage", "sexual_usage"), &VRMMeta::set_sexual_usage);
	ClassDB::bind_method(D_METHOD("get_commercial_usage"), &VRMMeta::get_commercial_usage);
	ClassDB::bind_method(D_METHOD("set_commercial_usage", "commercial_usage"), &VRMMeta::set_commercial_usage);
	ClassDB::bind_method(D_METHOD("get_other_permission_url"), &VRMMeta::get_other_permission_url);
	ClassDB::bind_method(D_METHOD("set_other_permission_url", "other_permission_url"), &VRMMeta::set_other_permission_url);
	ClassDB::bind_method(D_METHOD("get_license_name"), &VRMMeta::get_license_name);
	ClassDB::bind_method(D_METHOD("set_license_name", "license_name"), &VRMMeta::set_license_name);
	ClassDB::bind_method(D_METHOD("get_other_license_url"), &VRMMeta::get_other_license_url);
	ClassDB::bind_method(D_METHOD("set_other_license_url", "other_license_url"), &VRMMeta::set_other_license_url);
	ClassDB::bind_method(D_METHOD("get_humanoid_bone_mapping"), &VRMMeta::get_humanoid_bone_mapping);
	ClassDB::bind_method(D_METHOD("set_humanoid_bone_mapping", "humanoid_bone_mapping"), &VRMMeta::set_humanoid_bone_mapping);
	ClassDB::bind_method(D_METHOD("get_humanoid_skeleton_path"), &VRMMeta::get_humanoid_skeleton_path);
	ClassDB::bind_method(D_METHOD("set_humanoid_skeleton_path", "humanoid_skeleton_path"), &VRMMeta::set_humanoid_skeleton_path);
	ClassDB::bind_method(D_METHOD("get_eye_offset"), &VRMMeta::get_eye_offset);
	ClassDB::bind_method(D_METHOD("set_eye_offset", "eye_offset"), &VRMMeta::set_eye_offset);
	ClassDB::bind_method(D_METHOD("get_exporter_version"), &VRMMeta::get_exporter_version);
	ClassDB::bind_method(D_METHOD("set_exporter_version", "exporter_version"), &VRMMeta::set_exporter_version);
	ClassDB::bind_method(D_METHOD("get_spec_version"), &VRMMeta::get_spec_version);
	ClassDB::bind_method(D_METHOD("set_spec_version", "spec_version"), &VRMMeta::set_spec_version);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "version"), "set_version", "get_version");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "author"), "set_author", "get_author");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "contact_information"), "set_contact_information", "get_contact_information");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "reference_information"), "set_reference_information", "get_reference_information");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_texture", "get_texture");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "allowed_user_name"), "set_allowed_user_name", "get_allowed_user_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "violent_usage"), "set_violent_usage", "get_violent_usage");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "sexual_usage"), "set_sexual_usage", "get_sexual_usage");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "commercial_usage"), "set_commercial_usage", "get_commercial_usage");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "other_permission_url"), "set_other_permission_url", "get_other_permission_url");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "license_name"), "set_license_name", "get_license_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "other_license_url"), "set_other_license_url", "get_other_license_url");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "humanoid_bone_mapping", PROPERTY_HINT_RESOURCE_TYPE, "BoneMap"), "set_humanoid_bone_mapping", "get_humanoid_bone_mapping");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "humanoid_skeleton_path"), "set_humanoid_skeleton_path", "get_humanoid_skeleton_path");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "eye_offset"), "set_eye_offset", "get_eye_offset");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "exporter_version"), "set_exporter_version", "get_exporter_version");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "spec_version"), "set_spec_version", "get_spec_version");
}
