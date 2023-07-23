/**************************************************************************/
/*  template_modifier.cpp                                                 */
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

#include "template_modifier.h"
#include "core/config/project_settings.h"

void TemplateModifier::ByteStream::save(uint8_t p_value, Vector<uint8_t> &r_bytes) const {
	save(p_value, r_bytes, 1);
}

void TemplateModifier::ByteStream::save(uint16_t p_value, Vector<uint8_t> &r_bytes) const {
	save(p_value, r_bytes, 2);
}

void TemplateModifier::ByteStream::save(uint32_t p_value, Vector<uint8_t> &r_bytes) const {
	save(p_value, r_bytes, 4);
}

void TemplateModifier::ByteStream::save(const String &p_value, Vector<uint8_t> &r_bytes) const {
	r_bytes.append_array(p_value.to_utf16_buffer());
	save((uint16_t)0, r_bytes);
}

void TemplateModifier::ByteStream::save(uint32_t p_value, Vector<uint8_t> &r_bytes, uint32_t p_count) const {
	for (uint32_t i = 0; i < p_count; i++) {
		r_bytes.append((uint8_t)(p_value & 0xff));
		p_value >>= 8;
	}
}

Vector<uint8_t> TemplateModifier::ByteStream::save() const {
	return Vector<uint8_t>();
}

Vector<uint8_t> TemplateModifier::Structure::save() const {
	Vector<uint8_t> bytes;
	ByteStream::save(length, bytes);
	ByteStream::save(value_length, bytes);
	ByteStream::save(type, bytes);
	ByteStream::save(key, bytes);
	while (bytes.size() % 4) {
		bytes.append(0);
	}
	return bytes;
}

Vector<uint8_t> &TemplateModifier::Structure::add_length(Vector<uint8_t> &r_bytes) const {
	r_bytes.write[0] = r_bytes.size() & 0xff;
	r_bytes.write[1] = r_bytes.size() >> 8 & 0xff;
	return r_bytes;
}

Vector<uint8_t> TemplateModifier::FixedFileInfo::save() const {
	Vector<uint8_t> bytes;
	ByteStream::save(signature, bytes);
	ByteStream::save(struct_version, bytes);
	ByteStream::save(file_version_ms, bytes);
	ByteStream::save(file_version_ls, bytes);
	ByteStream::save(product_version_ms, bytes);
	ByteStream::save(product_version_ls, bytes);
	ByteStream::save(file_flags_mask, bytes);
	ByteStream::save(file_flags, bytes);
	ByteStream::save(file_os, bytes);
	ByteStream::save(file_type, bytes);
	ByteStream::save(file_subtype, bytes);
	ByteStream::save(file_date_ms, bytes);
	ByteStream::save(file_date_ls, bytes);
	return bytes;
}

void TemplateModifier::FixedFileInfo::set_file_version(const String &p_file_version) {
	Vector<String> parts = p_file_version.split(".");
	while (parts.size() < 4) {
		parts.append("0");
	}
	file_version_ms = parts[0].to_int() << 16 | (parts[1].to_int() & 0xffff);
	file_version_ls = parts[2].to_int() << 16 | (parts[3].to_int() & 0xffff);
}

void TemplateModifier::FixedFileInfo::set_product_version(const String &p_product_version) {
	Vector<String> parts = p_product_version.split(".");
	while (parts.size() < 4) {
		parts.append("0");
	}
	product_version_ms = parts[0].to_int() << 16 | (parts[1].to_int() & 0xffff);
	product_version_ls = parts[2].to_int() << 16 | (parts[3].to_int() & 0xffff);
}

Vector<uint8_t> TemplateModifier::StringStructure::save() const {
	Vector<uint8_t> bytes = Structure::save();
	ByteStream::save(value, bytes);
	return add_length(bytes);
}

TemplateModifier::StringStructure::StringStructure() {
	type = 1;
}

TemplateModifier::StringStructure::StringStructure(String p_key, String p_value) {
	type = 1;
	value_length = p_value.length() + 1;
	key = p_key;
	value = p_value;
}

Vector<uint8_t> TemplateModifier::StringTable::save() const {
	Vector<uint8_t> bytes = Structure::save();
	for (StringStructure string : strings) {
		bytes.append_array(string.save());
		while (bytes.size() % 4) {
			bytes.append(0);
		}
	}
	return add_length(bytes);
}

void TemplateModifier::StringTable::put(const String &p_key, const String &p_value) {
	strings.append(StringStructure(p_key, p_value));
}

TemplateModifier::StringTable::StringTable() {
	key = "040904b0";
	type = 1;
}

TemplateModifier::StringFileInfo::StringFileInfo() {
	key = "StringFileInfo";
	value_length = 0;
	type = 1;
}

Vector<uint8_t> TemplateModifier::StringFileInfo::save() const {
	Vector<uint8_t> bytes = Structure::save();
	bytes.append_array(string_table.save());
	return add_length(bytes);
}

Vector<uint8_t> TemplateModifier::Var::save() const {
	Vector<uint8_t> bytes = Structure::save();
	ByteStream::save(value, bytes);
	return add_length(bytes);
}

TemplateModifier::Var::Var() {
	value_length = 4;
	key = "Translation";
}

Vector<uint8_t> TemplateModifier::VarFileInfo::save() const {
	Vector<uint8_t> bytes = Structure::save();
	bytes.append_array(var.save());
	return add_length(bytes);
}

TemplateModifier::VarFileInfo::VarFileInfo() {
	type = 1;
	key = "VarFileInfo";
}

Vector<uint8_t> TemplateModifier::VersionInfo::save() const {
	Vector<uint8_t> fixed_file_info = value.save();
	Vector<uint8_t> bytes = Structure::save();
	bytes.append_array(fixed_file_info);
	bytes.append_array(string_file_info.save());
	while (bytes.size() % 4) {
		bytes.append(0);
	}
	bytes.append_array(var_file_info.save());
	return add_length(bytes);
}

TemplateModifier::VersionInfo::VersionInfo() {
	key = "VS_VERSION_INFO";
	value_length = 52;
}

Vector<uint8_t> TemplateModifier::IconEntry::save() const {
	Vector<uint8_t> bytes;
	ByteStream::save(width, bytes);
	ByteStream::save(height, bytes);
	ByteStream::save(colors, bytes);
	ByteStream::save((uint8_t)0, bytes);
	ByteStream::save(planes, bytes);
	ByteStream::save(bits_per_pixel, bytes);
	ByteStream::save(image_size, bytes);
	ByteStream::save((uint16_t)image_offset, bytes);
	return bytes;
}

void TemplateModifier::IconEntry::load(Ref<FileAccess> p_file) {
	width = p_file->get_8(); // Width in pixels.
	height = p_file->get_8(); // Height in pixels.
	colors = p_file->get_8(); // Number of colors in the palette (0 - no palette).
	p_file->get_8(); // Reserved.
	planes = p_file->get_16(); // Number of color planes.
	bits_per_pixel = p_file->get_16(); // Bits per pixel.
	image_size = p_file->get_32(); // Image data size in bytes.
	image_offset = p_file->get_32(); // Image data offset.
}

Vector<uint8_t> TemplateModifier::GroupIcon::save() const {
	Vector<uint8_t> bytes;
	ByteStream::save(reserved, bytes);
	ByteStream::save(type, bytes);
	ByteStream::save(image_count, bytes);
	for (IconEntry icon_entry : icon_entries) {
		bytes.append_array(icon_entry.save());
	}
	return bytes;
}

void TemplateModifier::GroupIcon::load(Ref<FileAccess> p_icon_file) {
	if (p_icon_file->get_32() != 0x10000) { // Wrong reserved bytes
		print_error("Wrong icon file type.");
		return;
	}
	if (p_icon_file->get_16() != IMAGE_COUNT) {
		print_error("Wrong number of icon images.");
		return;
	}

	for (uint16_t i = 0; i < IMAGE_COUNT; i++) {
		IconEntry icon_entry;
		icon_entry.load(p_icon_file);
		icon_entries.append(icon_entry);
	}

	int id = 1;
	for (IconEntry &icon_entry : icon_entries) {
		Vector<uint8_t> image;
		image.resize(icon_entry.image_size);
		p_icon_file->seek(icon_entry.image_offset);
		p_icon_file->get_buffer(image.ptrw(), image.size());
		icon_entry.image_offset = id++;
		images.append(image);
	}
}

void TemplateModifier::GroupIcon::fill_with_godot_blue() {
	uint32_t id = 1;
	for (uint8_t size : SIZES) {
		Ref<Image> image = Image::create_empty(size ? size : 256, size ? size : 256, false, Image::FORMAT_RGB8);
		image->fill(Color::hex(0x478cbfff));
		Vector<uint8_t> data = image->save_png_to_buffer();
		IconEntry icon_entry;
		icon_entry.width = size;
		icon_entry.height = size;
		icon_entry.bits_per_pixel = 24;
		icon_entry.image_size = data.size();
		icon_entry.image_offset = id++;
		icon_entries.append(icon_entry);
		images.append(data);
	}
}

Vector<uint8_t> TemplateModifier::SectionEntry::save() const {
	Vector<uint8_t> bytes;
	bytes.append_array(name.to_utf8_buffer());
	while (bytes.size() < 8) {
		bytes.append(0);
	}
	ByteStream::save(virtual_size, bytes);
	ByteStream::save(virtual_address, bytes);
	ByteStream::save(size_of_raw_data, bytes);
	ByteStream::save(pointer_to_raw_data, bytes);
	ByteStream::save(pointer_to_relocations, bytes);
	ByteStream::save(pointer_to_line_numbers, bytes);
	ByteStream::save(number_of_relocations, bytes);
	ByteStream::save(number_of_line_numbers, bytes);
	ByteStream::save(characteristics, bytes);
	return bytes;
}

void TemplateModifier::SectionEntry::load(Ref<FileAccess> p_file) {
	uint8_t section_name[8];
	p_file->get_buffer(section_name, 8);
	name = String::utf8((char *)section_name, 8);
	virtual_size = p_file->get_32();
	virtual_address = p_file->get_32();
	size_of_raw_data = p_file->get_32();
	pointer_to_raw_data = p_file->get_32();
	pointer_to_relocations = p_file->get_32();
	pointer_to_line_numbers = p_file->get_32();
	number_of_relocations = p_file->get_16();
	number_of_line_numbers = p_file->get_16();
	characteristics = p_file->get_32();
}

Vector<uint8_t> TemplateModifier::ResourceDataEntry::save() const {
	Vector<uint8_t> bytes;
	ByteStream::save(rva, bytes);
	ByteStream::save(size, bytes);
	ByteStream::save(0, bytes, 8);
	return bytes;
}

uint32_t TemplateModifier::get_pe_header_offset(const String &p_executable_path) const {
	Ref<FileAccess> executable = FileAccess::open(p_executable_path, FileAccess::READ);
	if (executable.is_null()) {
		return 0;
	}

	executable->seek(POINTER_TO_PE_HEADER_OFFSET);
	uint32_t pe_header_offset = executable->get_32();

	executable->seek(pe_header_offset);
	uint32_t magic = executable->get_32();

	return magic == 0x00004550 ? pe_header_offset : 0;
}

uint32_t TemplateModifier::snap(uint32_t p_value, uint32_t p_size) const {
	return p_value + (p_value % p_size ? p_size - (p_value % p_size) : 0);
}

Vector<uint8_t> TemplateModifier::create_resources(uint32_t p_virtual_address, const GroupIcon &p_group_icon, const VersionInfo &p_version_info) const {
	Vector<uint8_t> resources;
	resources.resize(sizeof(RESOURCE_DIRECTORY_TABLES));
	memcpy(resources.ptrw(), RESOURCE_DIRECTORY_TABLES, sizeof(RESOURCE_DIRECTORY_TABLES));

	Vector<Vector<uint8_t>> data_entries;
	for (Vector<uint8_t> image : p_group_icon.images) {
		data_entries.append(image);
	}
	data_entries.append(p_group_icon.save());
	data_entries.append(p_version_info.save());

	uint32_t offset = sizeof(RESOURCE_DIRECTORY_TABLES) + data_entries.size() * ResourceDataEntry::SIZE;

	for (Vector<uint8_t> data_entry : data_entries) {
		ResourceDataEntry resource_data_entry;
		resource_data_entry.rva = p_virtual_address + offset;
		resource_data_entry.size = data_entry.size();
		resources.append_array(resource_data_entry.save());
		offset += resource_data_entry.size;
		while (offset % 4) {
			offset += 1;
		}
	}

	for (Vector<uint8_t> data_entry : data_entries) {
		resources.append_array(data_entry);
		while (resources.size() % 4) {
			resources.append(0);
		}
	}

	return resources;
}

TemplateModifier::VersionInfo TemplateModifier::create_version_info(const HashMap<String, String> &strings) const {
	StringTable string_table;
	for (KeyValue<String, String> E : strings) {
		string_table.put(E.key, E.value);
	}

	StringFileInfo string_file_info;
	string_file_info.string_table = string_table;

	FixedFileInfo fixed_file_info;
	if (strings.has("FileVersion")) {
		fixed_file_info.set_file_version(strings["FileVersion"]);
	}
	if (strings.has("ProductVersion")) {
		fixed_file_info.set_product_version(strings["ProductVersion"]);
	}

	VersionInfo version_info;
	version_info.value = fixed_file_info;
	version_info.string_file_info = string_file_info;

	return version_info;
}

String TemplateModifier::get_icon_path(const Ref<EditorExportPreset> &p_preset, bool p_console_icon) const {
	String icon_path = ProjectSettings::get_singleton()->globalize_path(p_preset->get("application/icon"));
	if (p_console_icon) {
		String console_icon_path = ProjectSettings::get_singleton()->globalize_path(p_preset->get("application/console_wrapper_icon"));
		if (!console_icon_path.is_empty() && FileAccess::exists(console_icon_path)) {
			icon_path = console_icon_path;
		}
	}
	return icon_path;
}

TemplateModifier::GroupIcon TemplateModifier::create_group_icon(const String &p_icon_path) const {
	GroupIcon group_icon = GroupIcon();

	Ref<FileAccess> icon_file = FileAccess::open(p_icon_path, FileAccess::READ);
	if (icon_file.is_null()) {
		group_icon.fill_with_godot_blue();
		return group_icon;
	}

	group_icon.load(icon_file);

	return group_icon;
}

Error TemplateModifier::truncate(const String &path, uint32_t size) const {
	Error error;

	Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ, &error);
	ERR_FAIL_COND_V(error, ERR_CANT_OPEN);

	String truncated_path = path + ".truncated";
	Ref<FileAccess> truncated = FileAccess::open(truncated_path, FileAccess::WRITE, &error);
	ERR_FAIL_COND_V(error, ERR_CANT_CREATE);

	truncated->store_buffer(file->get_buffer(size));

	file->close();
	truncated->close();

	DirAccess::remove_absolute(path);
	DirAccess::rename_absolute(truncated_path, path);

	return error;
}

HashMap<String, String> TemplateModifier::get_strings(const Ref<EditorExportPreset> &p_preset) const {
	String file_version = p_preset->get("application/file_version");
	String product_version = p_preset->get("application/product_version");
	String company_name = p_preset->get("application/company_name");
	String product_name = p_preset->get("application/product_name");
	String file_description = p_preset->get("application/file_description");
	String copyright = p_preset->get("application/copyright");
	String trademarks = p_preset->get("application/trademarks");

	HashMap<String, String> strings;
	if (!file_version.is_empty()) {
		strings["FileVersion"] = file_version;
	}
	if (!product_version.is_empty()) {
		strings["ProductVersion"] = product_version;
	}
	if (!company_name.is_empty()) {
		strings["CompanyName"] = company_name;
	}
	if (!product_name.is_empty()) {
		strings["ProductName"] = product_name;
	}
	if (!file_description.is_empty()) {
		strings["FileDescription"] = file_description;
	}
	if (!copyright.is_empty()) {
		strings["LegalCopyright"] = copyright;
	}
	if (!trademarks.is_empty()) {
		strings["LegalTrademarks"] = trademarks;
	}

	return strings;
}

Error TemplateModifier::modify_template(const Ref<EditorExportPreset> &p_preset, const String &template_path, bool p_console_icon) const {
	Vector<SectionEntry> section_entries = get_section_entries(template_path);
	ERR_FAIL_COND_V(section_entries.size() < 2, ERR_CANT_OPEN);
	ERR_FAIL_COND_V(section_entries[section_entries.size() - 2].name != String(".rsrc"), ERR_CANT_OPEN);
	ERR_FAIL_COND_V(section_entries[section_entries.size() - 1].name != String(".reloc"), ERR_CANT_OPEN);
	// TODO fail on not sorted by physical address?

	String icon_path = get_icon_path(p_preset, p_console_icon);
	GroupIcon group_icon = create_group_icon(icon_path);

	VersionInfo version_info = create_version_info(get_strings(p_preset));

	SectionEntry resources_section_entry = section_entries.get(section_entries.size() - 2);

	Vector<uint8_t> resources = create_resources(resources_section_entry.virtual_address, group_icon, version_info);

	resources_section_entry.virtual_size = resources.size();
	resources.resize_zeroed(snap(resources.size(), BLOCK_SIZE));
	resources_section_entry.size_of_raw_data = resources.size();

	SectionEntry relocations_section_entry = section_entries.get(section_entries.size() - 1);
	relocations_section_entry.pointer_to_raw_data = resources_section_entry.pointer_to_raw_data + resources_section_entry.size_of_raw_data;
	relocations_section_entry.virtual_address = resources_section_entry.virtual_address + snap(resources_section_entry.virtual_size, PAGE_SIZE);

	uint32_t size_of_image = relocations_section_entry.virtual_address + snap(relocations_section_entry.virtual_size, PAGE_SIZE);

	uint32_t pe_header_offset = get_pe_header_offset(template_path);

	Error error;
	Ref<FileAccess> template_file = FileAccess::open(template_path, FileAccess::READ_WRITE, &error);
	ERR_FAIL_COND_V(error != OK, ERR_CANT_OPEN);

	Vector<uint8_t> relocations;
	relocations.resize(relocations_section_entry.size_of_raw_data);
	template_file->seek(relocations_section_entry.pointer_to_raw_data);
	template_file->store_buffer(relocations);
	uint32_t template_size = template_file->get_position();

	template_file->seek(pe_header_offset + SIZE_OF_OPTIONAL_HEADER_OFFSET);
	bool pe32plus = template_file->get_16() == 240;

	uint32_t optional_header_offset = pe_header_offset + COFF_HEADER_SIZE;

	template_file->seek(optional_header_offset + SIZE_OF_IMAGE_OFFSET);
	template_file->store_32(size_of_image);

	template_file->seek(optional_header_offset + (pe32plus ? 132 : 116));
	template_file->store_32(resources_section_entry.virtual_size);

	template_file->seek(optional_header_offset + (pe32plus ? 152 : 136));
	template_file->store_32(relocations_section_entry.virtual_address);
	template_file->store_32(relocations_section_entry.virtual_size);

	template_file->seek(optional_header_offset + (pe32plus ? 240 : 224) + SectionEntry::SIZE * (section_entries.size() - 2));
	template_file->store_buffer(resources_section_entry.save());
	template_file->store_buffer(relocations_section_entry.save());

	template_file->seek(resources_section_entry.pointer_to_raw_data);
	template_file->store_buffer(resources);
	template_file->store_buffer(relocations);

	if (template_file->get_position() < template_size) {
		template_file->close();
		truncate(template_path, relocations_section_entry.pointer_to_raw_data + relocations_section_entry.size_of_raw_data);
	}

	return OK;
}

Vector<TemplateModifier::SectionEntry> TemplateModifier::get_section_entries(const String &executable_path) const {
	Vector<SectionEntry> section_entries;

	uint32_t pe_header_offset = get_pe_header_offset(executable_path);
	if (pe_header_offset == 0) {
		return section_entries;
	}

	Ref<FileAccess> executable = FileAccess::open(executable_path, FileAccess::READ);
	if (executable.is_null()) {
		return section_entries;
	}

	executable->seek(pe_header_offset + 6);
	int num_sections = executable->get_16();
	executable->seek(pe_header_offset + 20);
	uint16_t size_of_optional_header = executable->get_16();
	executable->seek(pe_header_offset + COFF_HEADER_SIZE + size_of_optional_header);

	for (int i = 0; i < num_sections; ++i) {
		SectionEntry section_entry;
		section_entry.load(executable);
		section_entries.append(section_entry);
	}

	return section_entries;
}

Error TemplateModifier::modify(const Ref<EditorExportPreset> &p_preset, const String &template_path, bool p_console_icon) {
	TemplateModifier template_modifier;
	return template_modifier.modify_template(p_preset, template_path, p_console_icon);
}
