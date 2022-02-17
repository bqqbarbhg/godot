/*************************************************************************/
/*  openxr_interface.cpp                                                 */
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

#include "openxr_interface.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "servers/rendering/rendering_server_globals.h"

void OpenXRInterface::_bind_methods() {
	// todo
}

StringName OpenXRInterface::get_name() const {
	return StringName("OpenXR");
};

uint32_t OpenXRInterface::get_capabilities() const {
	return XRInterface::XR_VR + XRInterface::XR_STEREO;
};

XRInterface::TrackingStatus OpenXRInterface::get_tracking_status() const {
	return tracking_state;
}

void OpenXRInterface::_load_action_map() {
	ERR_FAIL_NULL(openxr_api);

	// This may seem a bit duplicitous to a little bit of background info here.
	// OpenXRActionMap (with all its sub resource classes) is a class that allows us to configure and store an action map in.
	// This gives the user the ability to edit the action map in a UI and customise the actions.
	// OpenXR however requires us to submit an action map and it takes over from that point and we can no longer change it.
	// This system does that push and we store the info needed to then work with this action map going forward.

	// Within our openxr device we maintain a number of classes that wrap the relevant OpenXR objects for this.
	// Within OpenXRInterface we have a few internal classes that keep track of what we've created.
	// This allow us to process the relevant actions each frame.

	// just in case clean up
	free_action_sets();
	free_trackers();

	Ref<OpenXRActionMap> action_map;
	if (Engine::get_singleton()->is_editor_hint()) {
#ifdef TOOLS_ENABLED
		action_map.instantiate();
		action_map->create_editor_action_sets();
#endif
	} else {
		String default_tres_name = openxr_api->get_default_action_map_resource_name();

		// Check if we can load our default
		if (ResourceLoader::exists(default_tres_name)) {
			action_map = ResourceLoader::load(default_tres_name);
		}

		// Check if we need to create default action set
		if (action_map.is_null()) {
			action_map.instantiate();
			action_map->create_default_action_sets();
#ifdef TOOLS_ENABLED
			// Save our action sets so our user can
			action_map->set_path(default_tres_name, true);
			ResourceSaver::save(default_tres_name, action_map);
#endif
		}
	}

	// process our action map
	if (action_map.is_valid()) {
		Map<Ref<OpenXRAction>, RID> action_rids;

		Array action_sets = action_map->get_action_sets();
		for (int i = 0; i < action_sets.size(); i++) {
			// Create our action set
			Ref<OpenXRActionSet> xr_action_set = action_sets[i];
			ActionSet *action_set = create_action_set(xr_action_set->get_name(), xr_action_set->get_localised_name(), xr_action_set->get_priority());
			if (!action_set) {
				continue;
			}

			// Now create our actions for these
			Array actions = xr_action_set->get_actions();
			for (int j = 0; j < actions.size(); j++) {
				Ref<OpenXRAction> xr_action = actions[j];

				PackedStringArray toplevel_paths = xr_action->get_toplevel_paths();
				Vector<RID> toplevel_rids;
				Vector<Tracker *> trackers;

				for (int k = 0; k < toplevel_paths.size(); k++) {
					Tracker *tracker = get_tracker(toplevel_paths[k]);
					if (tracker) {
						toplevel_rids.push_back(tracker->path_rid);
						trackers.push_back(tracker);
					}
				}

				Action *action = create_action(action_set, xr_action->get_name(), xr_action->get_localised_name(), xr_action->get_action_type(), toplevel_rids);
				if (action) {
					// we link our actions back to our trackers so we know which actions to check when we're processing our trackers
					for (int t = 0; t < trackers.size(); t++) {
						link_action_to_tracker(trackers[t], action);
					}

					// add this to our map for creating our interaction profiles
					action_rids[xr_action] = action->action_rid;
				}
			}
		}

		// now do our suggestions
		Array interaction_profiles = action_map->get_interaction_profiles();
		for (int i = 0; i < interaction_profiles.size(); i++) {
			Vector<OpenXRAPI::Binding> bindings;
			Ref<OpenXRInteractionProfile> xr_interaction_profile = interaction_profiles[i];

			Array xr_bindings = xr_interaction_profile->get_bindings();
			for (int j = 0; j < xr_bindings.size(); j++) {
				Ref<OpenXRIPBinding> xr_binding = xr_bindings[j];
				Ref<OpenXRAction> xr_action = xr_binding->get_action();
				OpenXRAPI::Binding binding;

				if (action_rids.has(xr_action)) {
					binding.action = action_rids[xr_action];
				} else {
					print_line("Action ", xr_action->get_name(), " isn't part of an action set!");
					continue;
				}

				PackedStringArray xr_paths = xr_binding->get_paths();
				for (int k = 0; k < xr_paths.size(); k++) {
					binding.path = xr_paths[k];
					bindings.push_back(binding);
				}
			}

			openxr_api->suggest_bindings(xr_interaction_profile->get_interaction_profile_path(), bindings);
		}
	}
}

OpenXRInterface::ActionSet *OpenXRInterface::create_action_set(const String &p_action_set_name, const String &p_localised_name, const int p_priority) {
	ERR_FAIL_NULL_V(openxr_api, nullptr);

	// find if it already exists
	for (int i = 0; i < action_sets.size(); i++) {
		if (action_sets[i]->action_set_name == p_action_set_name) {
			// already exists in this set
			return nullptr;
		}
	}

	ActionSet *action_set = memnew(ActionSet);
	action_set->action_set_name = p_action_set_name;
	action_set->is_active = true;
	action_set->action_set_rid = openxr_api->action_set_create(p_action_set_name, p_localised_name, p_priority);
	action_sets.push_back(action_set);

	return action_set;
}

void OpenXRInterface::free_action_sets() {
	ERR_FAIL_NULL(openxr_api);

	for (int i = 0; i < action_sets.size(); i++) {
		ActionSet *action_set = action_sets[i];

		openxr_api->path_free(action_set->action_set_rid);
		free_actions(action_set);

		memfree(action_set);
	}
	action_sets.clear();
}

OpenXRInterface::Action *OpenXRInterface::create_action(ActionSet *p_action_set, const String &p_action_name, const String &p_localised_name, OpenXRAction::ActionType p_action_type, const Vector<RID> p_toplevel_paths) {
	ERR_FAIL_NULL_V(openxr_api, nullptr);

	for (int i = 0; i < p_action_set->actions.size(); i++) {
		if (p_action_set->actions[i]->action_name == p_action_name) {
			// already exists in this set
			return nullptr;
		}
	}

	Action *action = memnew(Action);
	action->action_name = p_action_name;
	action->action_type = p_action_type;
	action->action_rid = openxr_api->action_create(p_action_set->action_set_rid, p_action_name, p_localised_name, p_action_type, p_toplevel_paths);
	p_action_set->actions.push_back(action);

	return action;
}

OpenXRInterface::Action *OpenXRInterface::find_action(const String &p_action_name) {
	// We just find the first action by this name

	for (int i = 0; i < action_sets.size(); i++) {
		for (int j = 0; j < action_sets[i]->actions.size(); j++) {
			if (action_sets[i]->actions[j]->action_name == p_action_name) {
				return action_sets[i]->actions[j];
			}
		}
	}

	// not found
	return nullptr;
}

void OpenXRInterface::free_actions(ActionSet *p_action_set) {
	ERR_FAIL_NULL(openxr_api);

	for (int i = 0; i < p_action_set->actions.size(); i++) {
		Action *action = p_action_set->actions[i];

		openxr_api->action_free(action->action_rid);

		memdelete(action);
	}
	p_action_set->actions.clear();
}

OpenXRInterface::Tracker *OpenXRInterface::get_tracker(const String &p_path_name) {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, nullptr);
	ERR_FAIL_NULL_V(openxr_api, nullptr);

	Tracker *tracker = nullptr;
	for (int i = 0; i < trackers.size(); i++) {
		tracker = trackers[i];
		if (tracker->path_name == p_path_name) {
			return tracker;
		}
	}

	// create our positional tracker
	Ref<XRPositionalTracker> positional_tracker;
	positional_tracker.instantiate();

	// We have standardised some names to make things nicer to the user so lets recognise the toplevel paths related to these.
	if (p_path_name == "/user/hand/left") {
		positional_tracker->set_tracker_type(XRServer::TRACKER_CONTROLLER);
		positional_tracker->set_tracker_name("left_hand");
		positional_tracker->set_tracker_desc("Left hand controller");
		positional_tracker->set_tracker_hand(XRPositionalTracker::TRACKER_HAND_LEFT);
	} else if (p_path_name == "/user/hand/right") {
		positional_tracker->set_tracker_type(XRServer::TRACKER_CONTROLLER);
		positional_tracker->set_tracker_name("right_hand");
		positional_tracker->set_tracker_desc("Right hand controller");
		positional_tracker->set_tracker_hand(XRPositionalTracker::TRACKER_HAND_RIGHT);
	} else {
		positional_tracker->set_tracker_type(XRServer::TRACKER_CONTROLLER);
		positional_tracker->set_tracker_name(p_path_name);
		positional_tracker->set_tracker_desc(p_path_name);
	}
	xr_server->add_tracker(positional_tracker);

	// create a new entry
	tracker = memnew(Tracker);
	tracker->path_name = p_path_name;
	tracker->path_rid = openxr_api->path_create(p_path_name);
	tracker->positional_tracker = positional_tracker;
	trackers.push_back(tracker);

	return tracker;
}

OpenXRInterface::Tracker *OpenXRInterface::find_tracker(const String &p_positional_tracker_name) {
	for (int i = 0; i < trackers.size(); i++) {
		Tracker *tracker = trackers[i];
		if (tracker->positional_tracker.is_valid() && tracker->positional_tracker->get_tracker_name() == p_positional_tracker_name) {
			return tracker;
		}
	}

	return nullptr;
}

void OpenXRInterface::link_action_to_tracker(Tracker *p_tracker, Action *p_action) {
	if (p_tracker->actions.find(p_action) == -1) {
		p_tracker->actions.push_back(p_action);
	}
}

void OpenXRInterface::handle_tracker(Tracker *p_tracker) {
	ERR_FAIL_NULL(openxr_api);
	ERR_FAIL_COND(p_tracker->positional_tracker.is_null());

	// handle all the actions
	for (int i = 0; i < p_tracker->actions.size(); i++) {
		Action *action = p_tracker->actions[i];
		switch (action->action_type) {
			case OpenXRAction::OPENXR_ACTION_BOOL: {
				bool pressed = openxr_api->get_action_bool(action->action_rid, p_tracker->path_rid);
				p_tracker->positional_tracker->set_input(action->action_name, Variant(pressed));
			} break;
			case OpenXRAction::OPENXR_ACTION_FLOAT: {
				real_t value = openxr_api->get_action_float(action->action_rid, p_tracker->path_rid);
				p_tracker->positional_tracker->set_input(action->action_name, Variant(value));
			} break;
			case OpenXRAction::OPENXR_ACTION_VECTOR2: {
				Vector2 value = openxr_api->get_action_vector2(action->action_rid, p_tracker->path_rid);
				p_tracker->positional_tracker->set_input(action->action_name, Variant(value));
			} break;
			case OpenXRAction::OPENXR_ACTION_POSE: {
				Transform3D transform;
				Vector3 linear, angular;
				XRPose::TrackingConfidence confidence = openxr_api->get_action_pose(action->action_rid, p_tracker->path_rid, transform, linear, angular);
				if (confidence != XRPose::XR_TRACKING_CONFIDENCE_NONE) {
					String name;
					// We can't have dual action names in OpenXR hence we added _pose, but default, aim and grip and default pose action names in Godot so rename them on the tracker.
					// NOTE need to decide on whether we should keep the naming convention or rename it on Godots side
					if (action->action_name == "default_pose") {
						name = "default";
					} else if (action->action_name == "aim_pose") {
						name = "aim";
					} else if (action->action_name == "grip_pose") {
						name = "grip";
					} else {
						name = action->action_name;
					}
					p_tracker->positional_tracker->set_pose(name, transform, linear, angular, confidence);
				}
			} break;
			default: {
				// not yet supported
			} break;
		}
	}
}

void OpenXRInterface::trigger_haptic_pulse(const String &p_action_name, const StringName &p_tracker_name, double p_frequency, double p_amplitude, double p_duration_sec, double p_delay_sec) {
	ERR_FAIL_NULL(openxr_api);
	Action *action = find_action(p_action_name);
	ERR_FAIL_NULL(action);
	Tracker *tracker = find_tracker(p_tracker_name);
	ERR_FAIL_NULL(tracker);

	// TODO OpenXR does not support delay, so we may need to add support for that somehow...

	XrDuration duration = XrDuration(p_duration_sec * 1000000000.0); // seconds -> nanoseconds

	openxr_api->trigger_haptic_pulse(action->action_rid, tracker->path_rid, p_frequency, p_amplitude, duration);
}

void OpenXRInterface::free_trackers() {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);
	ERR_FAIL_NULL(openxr_api);

	for (int i = 0; i < trackers.size(); i++) {
		Tracker *tracker = trackers[i];

		openxr_api->path_free(tracker->path_rid);
		xr_server->remove_tracker(tracker->positional_tracker);
		tracker->positional_tracker.unref();

		memdelete(tracker);
	}
	trackers.clear();
}

bool OpenXRInterface::initialise_on_startup() const {
	if (openxr_api == nullptr) {
		return false;
	} else if (!openxr_api->is_initialized()) {
		return false;
	} else {
		return true;
	}
}

bool OpenXRInterface::is_initialized() const {
	return initialized;
};

bool OpenXRInterface::initialize() {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, false);

	if (openxr_api == nullptr) {
		return false;
	} else if (!openxr_api->is_initialized()) {
		return false;
	} else if (initialized) {
		return true;
	}

	// load up our action sets before setting up our session, note that our profiles are suggestions, OpenXR takes ownership of (re)binding
	_load_action_map();

	if (!openxr_api->initialise_session()) {
		return false;
	}

	// we must create a tracker for our head
	head.instantiate();
	head->set_tracker_type(XRServer::TRACKER_HEAD);
	head->set_tracker_name("head");
	head->set_tracker_desc("Players head");
	xr_server->add_tracker(head);

	// attach action sets
	for (int i = 0; i < action_sets.size(); i++) {
		openxr_api->action_set_attach(action_sets[i]->action_set_rid);
	}

	// make this our primary interface
	xr_server->set_primary_interface(this);

	initialized = true;

	return initialized;
}

void OpenXRInterface::uninitialize() {
	// Our OpenXR driver will clean itself up properly when Godot exits, so we just do some basic stuff here

	// end the session if we need to?

	// cleanup stuff
	free_action_sets();
	free_trackers();

	XRServer *xr_server = XRServer::get_singleton();
	if (xr_server) {
		if (head.is_valid()) {
			xr_server->remove_tracker(head);

			head.unref();
		}
	}

	initialized = false;
}

bool OpenXRInterface::supports_play_area_mode(XRInterface::PlayAreaMode p_mode) {
	return false;
}

XRInterface::PlayAreaMode OpenXRInterface::get_play_area_mode() const {
	return XRInterface::XR_PLAY_AREA_UNKNOWN;
}

bool OpenXRInterface::set_play_area_mode(XRInterface::PlayAreaMode p_mode) {
	return false;
}

Size2 OpenXRInterface::get_render_target_size() {
	if (openxr_api == nullptr) {
		return Size2();
	} else {
		return openxr_api->get_recommended_target_size();
	}
}

uint32_t OpenXRInterface::get_view_count() {
	// TODO set this based on our configuration
	return 2;
}

void OpenXRInterface::_set_default_pos(Transform3D &p_transform, double p_world_scale, uint64_t p_eye) {
	p_transform = Transform3D();

	// if we're not tracking, don't put our head on the floor...
	p_transform.origin.y = 1.5 * p_world_scale;

	// overkill but..
	if (p_eye == 1) {
		p_transform.origin.x = 0.03 * p_world_scale;
	} else if (p_eye == 2) {
		p_transform.origin.x = -0.03 * p_world_scale;
	}
}

Transform3D OpenXRInterface::get_camera_transform() {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, Transform3D());

	Transform3D hmd_transform;
	double world_scale = xr_server->get_world_scale();

	// head_transform should be updated in process

	hmd_transform.basis = head_transform.basis;
	hmd_transform.origin = head_transform.origin * world_scale;

	return hmd_transform;
}

Transform3D OpenXRInterface::get_transform_for_view(uint32_t p_view, const Transform3D &p_cam_transform) {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, Transform3D());

	Transform3D t;
	if (openxr_api && openxr_api->get_view_transform(p_view, t)) {
		// update our cached value if we have a valid transform
		transform_for_view[p_view] = t;
	} else {
		// reuse cached value
		t = transform_for_view[p_view];
	}

	// Apply our world scale
	double world_scale = xr_server->get_world_scale();
	t.origin *= world_scale;

	return p_cam_transform * xr_server->get_reference_frame() * t;
}

CameraMatrix OpenXRInterface::get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) {
	CameraMatrix cm;

	if (openxr_api) {
		if (openxr_api->get_view_projection(p_view, p_z_near, p_z_far, cm)) {
			return cm;
		}
	}

	// Failed to get from our OpenXR device? Default to some sort of sensible camera matrix..
	cm.set_for_hmd(p_view + 1, 1.0, 6.0, 14.5, 4.0, 1.5, p_z_near, p_z_far);

	return cm;
}

void OpenXRInterface::process() {
	if (openxr_api) {
		// do our normal process
		if (openxr_api->process()) {
			Transform3D t;
			Vector3 linear_velocity;
			Vector3 angular_velocity;
			XRPose::TrackingConfidence confidence = openxr_api->get_head_center(t, linear_velocity, angular_velocity);
			if (confidence != XRPose::XR_TRACKING_CONFIDENCE_NONE) {
				// Only update our transform if we have one to update it with
				// note that poses are stored without world scale and reference frame applied!
				head_transform = t;
				head_linear_velocity = linear_velocity;
				head_angular_velocity = angular_velocity;
			}
		}

		// handle our action sets....
		Vector<RID> active_sets;
		for (int i = 0; i < action_sets.size(); i++) {
			if (action_sets[i]->is_active) {
				active_sets.push_back(action_sets[i]->action_set_rid);
			}
		}

		if (openxr_api->sync_action_sets(active_sets)) {
			for (int i = 0; i < trackers.size(); i++) {
				handle_tracker(trackers[i]);
			}
		}
	}

	if (head.is_valid()) {
		// TODO figure out how to get our velocities

		head->set_pose("default", head_transform, head_linear_velocity, head_angular_velocity);

		// TODO set confidence on pose once we support tracking this..
	}
}

void OpenXRInterface::pre_render() {
	if (openxr_api) {
		openxr_api->pre_render();
	}
}

bool OpenXRInterface::pre_draw_viewport(RID p_render_target) {
	if (openxr_api) {
		return openxr_api->pre_draw_viewport(p_render_target);
	} else {
		// don't render
		return false;
	}
}

Vector<BlitToScreen> OpenXRInterface::post_draw_viewport(RID p_render_target, const Rect2 &p_screen_rect) {
	Vector<BlitToScreen> blit_to_screen;

	// If separate HMD we should output one eye to screen
	if (p_screen_rect != Rect2()) {
		BlitToScreen blit;

		blit.render_target = p_render_target;
		blit.multi_view.use_layer = true;
		blit.multi_view.layer = 0;
		blit.lens_distortion.apply = false;

		Size2 render_size = get_render_target_size();
		Rect2 dst_rect = p_screen_rect;
		float new_height = dst_rect.size.x * (render_size.y / render_size.x);
		if (new_height > dst_rect.size.y) {
			dst_rect.position.y = (0.5 * dst_rect.size.y) - (0.5 * new_height);
			dst_rect.size.y = new_height;
		} else {
			float new_width = dst_rect.size.y * (render_size.x / render_size.y);

			dst_rect.position.x = (0.5 * dst_rect.size.x) - (0.5 * new_width);
			dst_rect.size.x = new_width;
		}

		blit.dst_rect = dst_rect;
		blit_to_screen.push_back(blit);
	}

	if (openxr_api) {
		openxr_api->post_draw_viewport(p_render_target);
	}

	return blit_to_screen;
}

void OpenXRInterface::end_frame() {
	if (openxr_api) {
		openxr_api->end_frame();
	}
}

OpenXRInterface::OpenXRInterface() {
	openxr_api = OpenXRAPI::get_singleton();

	// while we don't have head tracking, don't put the headset on the floor...
	_set_default_pos(head_transform, 1.0, 0);
	_set_default_pos(transform_for_view[0], 1.0, 1);
	_set_default_pos(transform_for_view[1], 1.0, 2);
}

OpenXRInterface::~OpenXRInterface() {
	openxr_api = nullptr;
}
