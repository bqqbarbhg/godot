/*************************************************************************/
/*  label_3d.h                                                           */
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

#ifndef LABEL_3D_H
#define LABEL_3D_H

#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/font.h"

#include "servers/text_server.h"

class Label3D : public GeometryInstance3D {
	GDCLASS(Label3D, GeometryInstance3D);

public:
	enum DrawFlags {
		FLAG_SHADED,
		FLAG_DOUBLE_SIDED,
		FLAG_DISABLE_DEPTH_TEST,
		FLAG_FIXED_SIZE,
		FLAG_MAX
	};

	enum AlphaCutMode {
		ALPHA_CUT_DISABLED,
		ALPHA_CUT_DISCARD,
		ALPHA_CUT_OPAQUE_PREPASS
	};

	enum AutowrapMode {
		AUTOWRAP_OFF,
		AUTOWRAP_ARBITRARY,
		AUTOWRAP_WORD,
		AUTOWRAP_WORD_SMART
	};

private:
	real_t pixel_size = 0.01;
	bool flags[FLAG_MAX] = {};
	AlphaCutMode alpha_cut = ALPHA_CUT_DISABLED;
	float alpha_scissor_threshold = 0.5;

	AABB aabb;

	mutable Ref<TriangleMesh> triangle_mesh;
	RID mesh;
	struct SurfaceData {
		PackedVector3Array mesh_vertices;
		PackedVector3Array mesh_normals;
		PackedFloat32Array mesh_tangents;
		PackedColorArray mesh_colors;
		PackedVector2Array mesh_uvs;
		PackedInt32Array indices;
		int offset = 0;
		float z_shift = 0.0;
		RID material;
	};

	Map<uint64_t, SurfaceData> surfaces;

	HorizontalAlignment horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER;
	VerticalAlignment vertical_alignment = VERTICAL_ALIGNMENT_CENTER;
	String text;
	String xl_text;
	bool uppercase = false;

	AutowrapMode autowrap_mode = AUTOWRAP_OFF;
	float width = 500.0;

	int font_size = 16;
	Ref<Font> font_override;
	Color modulate = Color(1, 1, 1, 1);

	int outline_size = 0;
	Color outline_modulate = Color(0, 0, 0, 1);

	float line_spacing = 0.f;

	Dictionary opentype_features;
	String language;
	TextServer::Direction text_direction = TextServer::DIRECTION_AUTO;
	TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;
	Array st_args;

	RID text_rid;
	Vector<RID> lines_rid;

	RID base_material;
	StandardMaterial3D::BillboardMode billboard_mode = StandardMaterial3D::BILLBOARD_DISABLED;
	StandardMaterial3D::TextureFilter texture_filter = StandardMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;

	bool pending_update = false;

	bool dirty_lines = true;
	bool dirty_font = true;
	bool dirty_text = true;

	void _generate_glyph_surfaces(const Glyph &p_glyph, Vector2 &r_offset, const Color &p_modulate, int p_priority = 0, int p_outline_size = 0);

protected:
	GDVIRTUAL2RC(Array, _structured_text_parser, Array, String)

	void _notification(int p_what);

	static void _bind_methods();

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	void _validate_property(PropertyInfo &property) const override;

	void _im_update();
	void _font_changed();
	void _queue_update();

	void _shape();

public:
	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_vertical_alignment(VerticalAlignment p_alignment);
	VerticalAlignment get_vertical_alignment() const;

	void set_text(const String &p_string);
	String get_text() const;

	void set_text_direction(TextServer::Direction p_text_direction);
	TextServer::Direction get_text_direction() const;

	void set_opentype_feature(const String &p_name, int p_value);
	int get_opentype_feature(const String &p_name) const;
	void clear_opentype_features();

	void set_language(const String &p_language);
	String get_language() const;

	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override() const;

	void set_structured_text_bidi_override_options(Array p_args);
	Array get_structured_text_bidi_override_options() const;

	void set_uppercase(bool p_uppercase);
	bool is_uppercase() const;

	void set_font(const Ref<Font> &p_font);
	Ref<Font> get_font() const;
	Ref<Font> _get_font_or_default() const;

	void set_font_size(int p_size);
	int get_font_size() const;

	void set_outline_size(int p_size);
	int get_outline_size() const;

	void set_line_spacing(float p_size);
	float get_line_spacing() const;

	void set_modulate(const Color &p_color);
	Color get_modulate() const;

	void set_outline_modulate(const Color &p_color);
	Color get_outline_modulate() const;

	void set_autowrap_mode(AutowrapMode p_mode);
	AutowrapMode get_autowrap_mode() const;

	void set_width(float p_width);
	float get_width() const;

	void set_pixel_size(real_t p_amount);
	real_t get_pixel_size() const;

	void set_draw_flag(DrawFlags p_flag, bool p_enable);
	bool get_draw_flag(DrawFlags p_flag) const;

	void set_alpha_cut_mode(AlphaCutMode p_mode);
	AlphaCutMode get_alpha_cut_mode() const;

	void set_alpha_scissor_threshold(float p_threshold);
	float get_alpha_scissor_threshold() const;

	void set_billboard_mode(StandardMaterial3D::BillboardMode p_mode);
	StandardMaterial3D::BillboardMode get_billboard_mode() const;

	void set_texture_filter(StandardMaterial3D::TextureFilter p_filter);
	StandardMaterial3D::TextureFilter get_texture_filter() const;

	virtual AABB get_aabb() const override;
	Ref<TriangleMesh> generate_triangle_mesh() const;

	Label3D();
	~Label3D();
};

VARIANT_ENUM_CAST(Label3D::AutowrapMode);
VARIANT_ENUM_CAST(Label3D::DrawFlags);
VARIANT_ENUM_CAST(Label3D::AlphaCutMode);

#endif // LABEL_3D_H
