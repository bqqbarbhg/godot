/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "vrm_collidergroup.h"
#include "vrm_constants.h"
#include "vrm_extension.h"
#include "vrm_meta.h"
#include "vrm_secondary.h"
#include "vrm_spring_bone_logic.h"
#include "vrm_springbone.h"
#include "vrm_toplevel.h"

#if defined(TOOLS_ENABLED)
#include "editor/editor_settings.h"
#include "editor/vrm.h"

static void _editor_init() {
	Ref<VRMImportPlugin> import_vrm;
	import_vrm.instantiate();
	ResourceImporterScene::add_importer(import_vrm);
}
#endif

void initialize_vrm_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		ClassDB::register_class<VRMImportPlugin>();
		ClassDB::register_class<VRMEditorPlugin>();
		ClassDB::register_class<VRMMeta>();
		ClassDB::register_class<VRMColliderGroup>();
		ClassDB::register_class<VRMConstants>();
		ClassDB::register_class<VRMExtension>();
		ClassDB::register_class<VRMSecondary>();
		ClassDB::register_class<VRMSpringBoneLogic>();
		ClassDB::register_class<VRMSpringBone>();
		ClassDB::register_class<VRMTopLevel>();
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		// Editor-specific API.
		ClassDB::APIType prev_api = ClassDB::get_current_api();
		ClassDB::set_current_api(ClassDB::API_EDITOR);

		GDREGISTER_CLASS(VRMImportPlugin);

		ClassDB::set_current_api(prev_api);
		EditorNode::add_init_callback(_editor_init);
	}

#endif // TOOLS_ENABLED
}

void uninitialize_vrm_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}