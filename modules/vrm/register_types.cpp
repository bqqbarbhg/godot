#include "register_types.h"

#include "vrm_toplevel.h"
#include "vrm.h"
#include "vrm_springbone.h"
#include "vrm_spring_bone_logic.h"
#include "vrm_meta.h"

void initialize_vrm_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
            return;
    }
    ClassDB::register_class<VRMImportPlugin>();
    ClassDB::register_class<VRMEditorPlugin>();
    ClassDB::register_class<VRMTopLevel>();
    ClassDB::register_class<VRMMeta>();
}

void uninitialize_vrm_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
            return;
    }
}