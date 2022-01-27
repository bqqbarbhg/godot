/*************************************************************************/
/*  openxr_action_sets.cpp                                               */
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

#include "openxr_action_sets.h"

void OpenXRActionSets::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_action_sets", "action_sets"), &OpenXRActionSets::set_action_sets);
	ClassDB::bind_method(D_METHOD("get_action_sets"), &OpenXRActionSets::get_action_sets);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "action_sets", PROPERTY_HINT_RESOURCE_TYPE, "OpenXRActionSet", PROPERTY_USAGE_NO_EDITOR), "set_action_sets", "get_action_sets");

	ClassDB::bind_method(D_METHOD("add_action_set", "action_set"), &OpenXRActionSets::add_action_set);
	ClassDB::bind_method(D_METHOD("remove_action_set", "action_set"), &OpenXRActionSets::remove_action_set);

	ClassDB::bind_method(D_METHOD("set_interaction_profiles", "interaction_profiles"), &OpenXRActionSets::set_interaction_profiles);
	ClassDB::bind_method(D_METHOD("get_interaction_profiles"), &OpenXRActionSets::get_interaction_profiles);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "interaction_profiles", PROPERTY_HINT_RESOURCE_TYPE, "OpenXRInteractionProfile", PROPERTY_USAGE_NO_EDITOR), "set_interaction_profiles", "get_interaction_profiles");

	ClassDB::bind_method(D_METHOD("add_interaction_profile", "interaction_profile"), &OpenXRActionSets::add_interaction_profile);
	ClassDB::bind_method(D_METHOD("remove_interaction_profile", "interaction_profile"), &OpenXRActionSets::remove_interaction_profile);

	ClassDB::bind_method(D_METHOD("create_default_action_sets"), &OpenXRActionSets::create_default_action_sets);
	ClassDB::bind_method(D_METHOD("verify_action_sets", "fix_errors"), &OpenXRActionSets::verify_action_sets);
}

void OpenXRActionSets::set_action_sets(Array p_action_sets) {
	clear_action_sets();
	for (int i = 0; i < p_action_sets.size(); i++) {
		Ref<OpenXRActionSet> action_set = p_action_sets[i];
		add_action_set(action_set);
	}
}

Array OpenXRActionSets::get_action_sets() const {
	Array arr;

	for (int i = 0; i < action_sets.size(); i++) {
		arr.push_back(action_sets[i]);
	}

	return arr;
}

void OpenXRActionSets::add_action_set(Ref<OpenXRActionSet> p_action_set) {
	ERR_FAIL_COND(p_action_set.is_valid());

	if (action_sets.find(p_action_set) == -1) {
		// Not sure if Vector properly refcounts so taking the long way around..
		Ref<OpenXRActionSet> new_action_set;
		if (action_sets.push_back(new_action_set)) {
			action_sets.ptrw()[action_sets.size() - 1] = p_action_set;
		}
	}
}

void OpenXRActionSets::remove_action_set(Ref<OpenXRActionSet> p_action_set) {
	int idx = action_sets.find(p_action_set);
	if (idx != -1) {
		// Not sure if Vector properly refcounts so taking the long way around..
		action_sets.ptrw()[idx].unref();
		action_sets.remove_at(idx);
	}
}

void OpenXRActionSets::clear_action_sets() {
	// Not sure if Vector properly refcounts so taking the long way around..
	for (int i = 0; i < action_sets.size(); i++) {
		action_sets.ptrw()[i].unref();
	}
	action_sets.clear();
}

void OpenXRActionSets::set_interaction_profiles(Array p_interaction_profiles) {
	clear_interaction_profiles();
	for (int i = 0; i < p_interaction_profiles.size(); i++) {
		Ref<OpenXRInteractionProfile> interaction_profile = p_interaction_profiles[i];
		add_interaction_profile(interaction_profile);
	}
}

Array OpenXRActionSets::get_interaction_profiles() const {
	Array arr;

	for (int i = 0; i < interaction_profiles.size(); i++) {
		arr.push_back(interaction_profiles[i]);
	}

	return arr;
}

void OpenXRActionSets::add_interaction_profile(Ref<OpenXRInteractionProfile> p_interaction_profile) {
	ERR_FAIL_COND(p_interaction_profile.is_valid());

	if (interaction_profiles.find(p_interaction_profile) == -1) {
		// Not sure if Vector properly refcounts so taking the long way around..
		Ref<OpenXRInteractionProfile> new_interaction_profile;
		if (interaction_profiles.push_back(new_interaction_profile)) {
			interaction_profiles.ptrw()[interaction_profiles.size() - 1] = p_interaction_profile;
		}
	}
}

void OpenXRActionSets::remove_interaction_profile(Ref<OpenXRInteractionProfile> p_interaction_profile) {
	int idx = interaction_profiles.find(p_interaction_profile);
	if (idx != -1) {
		// Not sure if Vector properly refcounts so taking the long way around..
		interaction_profiles.ptrw()[idx].unref();
		interaction_profiles.remove_at(idx);
	}
}

void OpenXRActionSets::clear_interaction_profiles() {
	// Not sure if Vector properly refcounts so taking the long way around..
	for (int i = 0; i < interaction_profiles.size(); i++) {
		interaction_profiles.ptrw()[i].unref();
	}
	interaction_profiles.clear();
}

void OpenXRActionSets::create_default_action_sets() {
	// Create our Godot action set
	Ref<OpenXRActionSet> action_set = OpenXRActionSet::new_action_set("godot", "Godot action set");
	add_action_set(action_set);

	// Create our actions
	Ref<OpenXRAction> trigger = action_set->add_new_action("trigger", "Trigger", OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> trigger_click = action_set->add_new_action("trigger_click", "Trigger click", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> trigger_touch = action_set->add_new_action("trigger_touch", "Trigger touching", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip = action_set->add_new_action("grip", "Grip", OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_click = action_set->add_new_action("grip_click", "Grip click", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_touch = action_set->add_new_action("grip_touch", "Grip touching", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary = action_set->add_new_action("primary", "Primary joystick/thumbstick/trackpad", OpenXRAction::OPENXR_ACTION_VECTOR2, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary_click = action_set->add_new_action("primary_click", "Primary joystick/thumbstick/trackpad click", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary_touch = action_set->add_new_action("primary_touch", "Primary joystick/thumbstick/trackpad touching", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary = action_set->add_new_action("secondary", "Secondary joystick/thumbstick/trackpad", OpenXRAction::OPENXR_ACTION_VECTOR2, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary_click = action_set->add_new_action("secondary_click", "Secondary joystick/thumbstick/trackpad click", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary_touch = action_set->add_new_action("secondary_touch", "Secondary joystick/thumbstick/trackpad touching", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> menu_button = action_set->add_new_action("menu_button", "Menu button", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> select_button = action_set->add_new_action("select_button", "Select button", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> ax_button = action_set->add_new_action("ax_button", "A/X button", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> ax_touch = action_set->add_new_action("ax_touch", "A/X touching", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> by_button = action_set->add_new_action("by_button", "B/Y button", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> by_touch = action_set->add_new_action("by_touch", "B/Y touching", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> aim_pose = action_set->add_new_action("aim_pose", "Aim pose", OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_pose = action_set->add_new_action("grip_pose", "Grip pose", OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> haptic = action_set->add_new_action("haptic", "Haptic", OpenXRAction::OPENXR_ACTION_HAPTIC, "/user/hand/left,/user/hand/right");

	// Create our interaction profiles
	Ref<OpenXRInteractionProfile> profile = OpenXRInteractionProfile::new_profile("/interaction_profiles/khr/simple_controller");
	profile->add_new_binding(aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(menu_button, "/user/hand/left/input/menu/click,/user/hand/right/input/menu/click");
	profile->add_new_binding(select_button, "/user/hand/left/input/select/click,/user/hand/right/input/select/click");
	// generic has no support for triggers, grip, A/B buttons, nor joystick/trackpad inputs
	profile->add_new_binding(haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our Vive controller profile
	profile = OpenXRInteractionProfile::new_profile("/interaction_profiles/htc/vive_controller");
	profile->add_new_binding(aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(menu_button, "/user/hand/left/input/menu/click,/user/hand/right/input/menu/click");
	profile->add_new_binding(select_button, "/user/hand/left/input/system/click,/user/hand/right/input/system/click");
	// wmr controller has no a/b/x/y buttons
	profile->add_new_binding(trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(trigger_click, "/user/hand/left/input/trigger/click,/user/hand/right/input/trigger/click");
	profile->add_new_binding(grip, "/user/hand/left/input/squeeze/click,/user/hand/right/input/squeeze/click"); // OpenXR will convert bool to float
	profile->add_new_binding(grip_click, "/user/hand/left/input/squeeze/click,/user/hand/right/input/squeeze/click");
	// primary on our vive controller is our trackpad
	profile->add_new_binding(primary, "/user/hand/left/input/trackpad,/user/hand/right/input/trackpad");
	profile->add_new_binding(primary_click, "/user/hand/left/input/trackpad/click,/user/hand/right/input/trackpad/click");
	profile->add_new_binding(primary_touch, "/user/hand/left/input/trackpad/touch,/user/hand/right/input/trackpad/touch");
	// vive controllers have no secondary input
	profile->add_new_binding(haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our WMR controller profile
	profile = OpenXRInteractionProfile::new_profile("/interaction_profiles/microsoft/motion_controller");
	profile->add_new_binding(aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	// wmr controllers have no select button we can use
	profile->add_new_binding(menu_button, "/user/hand/left/input/menu/click,/user/hand/right/input/menu/click");
	// wmr controller has no a/b/x/y buttons
	profile->add_new_binding(trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(trigger_click, "/user/hand/left/input/trigger/click,/user/hand/right/input/trigger/click");
	profile->add_new_binding(grip, "/user/hand/left/input/squeeze/click,/user/hand/right/input/squeeze/click"); // OpenXR will convert bool to float
	profile->add_new_binding(grip_click, "/user/hand/left/input/squeeze/click,/user/hand/right/input/squeeze/click");
	// primary on our wmr controller is our thumbstick
	profile->add_new_binding(primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click, "/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch, "/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
	// secondary on our wmr controller is our trackpad
	profile->add_new_binding(secondary, "/user/hand/left/input/trackpad,/user/hand/right/input/trackpad");
	profile->add_new_binding(secondary_click, "/user/hand/left/input/trackpad/click,/user/hand/right/input/trackpad/click");
	profile->add_new_binding(secondary_touch, "/user/hand/left/input/trackpad/touch,/user/hand/right/input/trackpad/touch");
	profile->add_new_binding(haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our HP MR controller profile
	profile = OpenXRInteractionProfile::new_profile("/interaction_profiles/hp/mixed_reality_controller");
	profile->add_new_binding(aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	// hpmr controllers have no select button we can use
	profile->add_new_binding(menu_button, "/user/hand/left/input/menu/click,/user/hand/right/input/menu/click");
	// hpmr controllers only register click, not touch, on our a/b/x/y buttons
	profile->add_new_binding(ax_button, "/user/hand/left/input/x/click,/user/hand/right/input/a/click"); // x on left hand, a on right hand
	profile->add_new_binding(by_button, "/user/hand/left/input/y/click,/user/hand/right/input/b/click"); // y on left hand, b on right hand
	profile->add_new_binding(trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(trigger_click, "/user/hand/left/input/trigger/click,/user/hand/right/input/trigger/click");
	profile->add_new_binding(grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(grip_click, "/user/hand/left/input/squeeze/click,/user/hand/right/input/squeeze/click");
	// primary on our hpmr controller is our thumbstick
	profile->add_new_binding(primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click, "/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch, "/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
	// secondary on our hpmr controller is our trackpad
	profile->add_new_binding(secondary, "/user/hand/left/input/trackpad,/user/hand/right/input/trackpad");
	profile->add_new_binding(secondary_click, "/user/hand/left/input/trackpad/click,/user/hand/right/input/trackpad/click");
	profile->add_new_binding(secondary_touch, "/user/hand/left/input/trackpad/touch,/user/hand/right/input/trackpad/touch");
	profile->add_new_binding(haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our Meta touch controller profile
	profile = OpenXRInteractionProfile::new_profile("/interaction_profiles/oculus/touch_controller");
	profile->add_new_binding(aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	// touch controllers have no select button we can use
	profile->add_new_binding(menu_button, "/user/hand/left/input/menu/click,/user/hand/right/input/menu/click");
	profile->add_new_binding(ax_button, "/user/hand/left/input/x/click,/user/hand/right/input/a/click"); // x on left hand, a on right hand
	profile->add_new_binding(ax_touch, "/user/hand/left/input/x/touch,/user/hand/right/input/a/touch");
	profile->add_new_binding(by_button, "/user/hand/left/input/y/click,/user/hand/right/input/b/click"); // y on left hand, b on right hand
	profile->add_new_binding(by_touch, "/user/hand/left/input/y/touch,/user/hand/right/input/b/touch");
	profile->add_new_binding(trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(trigger_click, "/user/hand/left/input/trigger/click,/user/hand/right/input/trigger/click");
	profile->add_new_binding(trigger_touch, "/user/hand/left/input/trigger/touch,/user/hand/right/input/trigger/touch");
	profile->add_new_binding(grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(grip_click, "/user/hand/left/input/squeeze/click,/user/hand/right/input/squeeze/click");
	profile->add_new_binding(grip_touch, "/user/hand/left/input/squeeze/touch,/user/hand/right/input/squeeze/touch");
	// primary on our touch controller is our thumbstick
	profile->add_new_binding(primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click, "/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch, "/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
	// touch controller has no secondary input
	profile->add_new_binding(haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our Valve index controller profile
	profile = OpenXRInteractionProfile::new_profile("/interaction_profiles/valve/index_controller");
	profile->add_new_binding(aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	// index controllers have no select button we can use
	profile->add_new_binding(menu_button, "/user/hand/left/input/menu/click,/user/hand/right/input/menu/click");
	profile->add_new_binding(ax_button, "/user/hand/left/input/a/click,/user/hand/right/input/a/click"); // a on both controllers
	profile->add_new_binding(ax_touch, "/user/hand/left/input/a/touch,/user/hand/right/input/a/touch");
	profile->add_new_binding(by_button, "/user/hand/left/input/b/click,/user/hand/right/input/b/click"); // b on both controllers
	profile->add_new_binding(by_touch, "/user/hand/left/input/b/touch,/user/hand/right/input/b/touch");
	profile->add_new_binding(trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(trigger_click, "/user/hand/left/input/trigger/click,/user/hand/right/input/trigger/click");
	profile->add_new_binding(trigger_touch, "/user/hand/left/input/trigger/touch,/user/hand/right/input/trigger/touch");
	profile->add_new_binding(grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(grip_click, "/user/hand/left/input/squeeze/click,/user/hand/right/input/squeeze/click");
	profile->add_new_binding(grip_touch, "/user/hand/left/input/squeeze/touch,/user/hand/right/input/squeeze/touch");
	// primary on our index controller is our thumbstick
	profile->add_new_binding(primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click, "/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch, "/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
	// secondary on our index controller is our trackpad
	profile->add_new_binding(secondary, "/user/hand/left/input/trackpad,/user/hand/right/input/trackpad");
	profile->add_new_binding(secondary_click, "/user/hand/left/input/trackpad/click,/user/hand/right/input/trackpad/click");
	profile->add_new_binding(secondary_touch, "/user/hand/left/input/trackpad/touch,/user/hand/right/input/trackpad/touch");
	profile->add_new_binding(haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);
}

void OpenXRActionSets::create_editor_action_sets() {
	// TODO implement
}

bool OpenXRActionSets::verify_action_sets(bool p_fix_errors) {
	// TODO implement this

	// Check if all actions sets have unique names
	// Actions can be reused in multiple action sets
	// The active action set with the highest priority will determine the actions value

	// check if there are no double action names (and fix them up if needed)
	// if action is reused in multiple action sets, make sure priority is different

	// check that all actions used in interaction profiles

	return true;
}

OpenXRActionSets::~OpenXRActionSets() {
	clear_action_sets();
	clear_interaction_profiles();
}
