/**************************************************************************/
/*  scene_distribution_interface.h                                        */
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

#ifndef SCENE_DISTRIBUTION_INTERFACE_H
#define SCENE_DISTRIBUTION_INTERFACE_H

#include "core/object/ref_counted.h"
#include <vector>

class SceneMultiplayer;

class SceneDistributionInterface : public RefCounted {
	GDCLASS(SceneDistributionInterface, RefCounted);

private:
	SceneMultiplayer *multiplayer = nullptr;

	static void _bind_methods();

	// The directory where external programs save the created glb file.
	String externally_created_glb_storage_path = "user://requested_glb/";
	// The script to call, to create a glb file.
	String externally_create_glb_script = "user:///scripts/create_glb.bat";

	// Here save requested glb files. If they are created and distributed, they are removed. Pending requests.
	HashSet<String> requested_glb_files;

	// Used only inside scene_distribution_interface.cpp
	void _distribute_glb(const String &p_path, int id);
	void _remove_glb_as_requested(const String &glb_name);

	// The peer that is able to create glb files with external tools.
	int _glb_creator_peer = -1;

public:
	// Used in _bind_methods() to be used in GDScript.
	void set_own_peer_as_glb_creator();
	void request_glb(const String &glb_name);

	// Used in scene_multiplayer.cpp.
	void set_glb_as_requested(const String &glb_name);
	HashSet<String> get_requested_glb_files();
	void request_to_externally_create_glb(const String &glb_name);
	void check_if_externally_created_glb_was_created();
	void set_glb_creator_peer(int peer);
	int get_glb_creator_peer();

	//will be called in multiplayer constructor
	SceneDistributionInterface(SceneMultiplayer *p_multiplayer) {
		multiplayer = p_multiplayer;
	}
};

#endif // SCENE_DISTRIBUTION_INTERFACE_H
