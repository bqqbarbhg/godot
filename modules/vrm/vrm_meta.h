/**************************************************************************/
/*  vrm_meta.h                                                            */
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

#ifndef VRM_META_H
#define VRM_META_H

#include "core/io/resource.h"
#include "scene/resources/bone_map.h"
#include "scene/resources/texture.h"

class VRMMeta : public Resource {
	GDCLASS(VRMMeta, Resource);

private:
	String title;
	String version;
	String author;
	String contact_information;
	String reference_information;
	Ref<Texture> texture;
	String allowed_user_name;
	String violent_usage;
	String sexual_usage;
	String commercial_usage;
	String other_permission_url;
	String license_name;
	String other_license_url;
	Ref<BoneMap> humanoid_bone_mapping;
	NodePath humanoid_skeleton_path;
	Vector3 eye_offset;
	String exporter_version;
	String spec_version;

protected:
	static void _bind_methods();

public:
	String get_title() const { return title; }
	String get_version() const { return version; }
	String get_author() const { return author; }
	String get_contact_information() const { return contact_information; }
	String get_reference_information() const { return reference_information; }
	Ref<Texture> get_texture() const { return texture; }
	String get_allowed_user_name() const { return allowed_user_name; }
	String get_violent_usage() const { return violent_usage; }
	String get_sexual_usage() const { return sexual_usage; }
	String get_commercial_usage() const { return commercial_usage; }
	String get_other_permission_url() const { return other_permission_url; }
	String get_license_name() const { return license_name; }
	String get_other_license_url() const { return other_license_url; }
	Ref<BoneMap> get_humanoid_bone_mapping() const { return humanoid_bone_mapping; }
	NodePath get_humanoid_skeleton_path() const { return humanoid_skeleton_path; }
	Vector3 get_eye_offset() const { return eye_offset; }
	String get_exporter_version() const { return exporter_version; }
	String get_spec_version() const { return spec_version; }

	void set_title(const String &p_title) { title = p_title; }
	void set_version(const String &p_version) { version = p_version; }
	void set_author(const String &p_author) { author = p_author; }
	void set_contact_information(const String &p_contact_information) { contact_information = p_contact_information; }
	void set_reference_information(const String &p_reference_information) { reference_information = p_reference_information; }
	void set_texture(const Ref<Texture> &p_texture) { texture = p_texture; }
	void set_allowed_user_name(const String &p_allowed_user_name) { allowed_user_name = p_allowed_user_name; }
	void set_violent_usage(const String &p_violent_usage) { violent_usage = p_violent_usage; }
	void set_sexual_usage(const String &p_sexual_usage) { sexual_usage = p_sexual_usage; }
	void set_commercial_usage(const String &p_commercial_usage) { commercial_usage = p_commercial_usage; }
	void set_other_permission_url(const String &p_other_permission_url) { other_permission_url = p_other_permission_url; }
	void set_license_name(const String &p_license_name) { license_name = p_license_name; }
	void set_other_license_url(const String &p_other_license_url) { other_license_url = p_other_license_url; }
	void set_humanoid_bone_mapping(const Ref<BoneMap> &p_humanoid_bone_mapping) { humanoid_bone_mapping = p_humanoid_bone_mapping; }
	void set_humanoid_skeleton_path(const NodePath &p_humanoid_skeleton_path) { humanoid_skeleton_path = p_humanoid_skeleton_path; }
	void set_eye_offset(const Vector3 &p_eye_offset) { eye_offset = p_eye_offset; }
	void set_exporter_version(const String &p_exporter_version) { exporter_version = p_exporter_version; }
	void set_spec_version(const String &p_spec_version) { spec_version = p_spec_version; }
};

#endif // VRM_META_H