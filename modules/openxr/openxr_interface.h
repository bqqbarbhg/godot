/*************************************************************************/
/*  openxr_interface.h                                                   */
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

#ifndef OPENXR_INTERFACE_H
#define OPENXR_INTERFACE_H

#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_positional_tracker.h"

#include "drivers/openxr/openxr_device.h"

#include "openxr_action_sets.h"

class OpenXRInterface : public XRInterface {
	GDCLASS(OpenXRInterface, XRInterface);

private:
	OpenXRDevice *openxr_device;
	bool initialized = false;
	XRInterface::TrackingStatus tracking_state;

	// At a minimum we need a tracker for our head
	Ref<XRPositionalTracker> head;
	Transform3D head_transform;
	Vector3 head_linear_velocity;
	Vector3 head_angular_velocity;
	Transform3D transform_for_view[2]; // We currently assume 2, but could be 4 for VARJO which we do not support yet

	void _set_default_pos(Transform3D &p_transform, double p_world_scale, uint64_t p_eye);
	void _load_action_sets();

protected:
	static void _bind_methods();

public:
	virtual StringName get_name() const override;
	virtual uint32_t get_capabilities() const override;

	virtual TrackingStatus get_tracking_status() const override;

	bool initialise_on_startup() const;
	virtual bool is_initialized() const override;
	virtual bool initialize() override;
	virtual void uninitialize() override;

	virtual bool supports_play_area_mode(XRInterface::PlayAreaMode p_mode) override;
	virtual XRInterface::PlayAreaMode get_play_area_mode() const override;
	virtual bool set_play_area_mode(XRInterface::PlayAreaMode p_mode) override;

	virtual Size2 get_render_target_size() override;
	virtual uint32_t get_view_count() override;
	virtual Transform3D get_camera_transform() override;
	virtual Transform3D get_transform_for_view(uint32_t p_view, const Transform3D &p_cam_transform) override;
	virtual CameraMatrix get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) override;

	virtual void process() override;
	virtual void pre_render() override;
	bool pre_draw_viewport(RID p_render_target) override;
	virtual Vector<BlitToScreen> post_draw_viewport(RID p_render_target, const Rect2 &p_screen_rect) override;
	virtual void end_frame() override;

	OpenXRInterface();
	~OpenXRInterface();
};

#endif // !OPENXR_INTERFACE_H
