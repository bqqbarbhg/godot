#include "register_types.h"

#include "vrm.h"
#include "vrm_collidergroup.h"
#include "vrm_constants.h"
#include "vrm_extension.h"
#include "vrm_meta.h"
#include "vrm_secondary.h"
#include "vrm_spring_bone_logic.h"
#include "vrm_springbone.h"
#include "vrm_toplevel.h"

void initialize_vrm_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
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

void uninitialize_vrm_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}