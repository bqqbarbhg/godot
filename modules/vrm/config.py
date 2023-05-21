def can_build(env, platform):
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "EditorSceneFormatImporterVRM",
        "VRMColliderGroup",
        "VRMConstants",
        "VRMExtension",
        "VRMMeta",
        "VRMSecondary",
        "VRMSpringBone",
        "VRMSpringBoneLogic",
        "VRMTopLevel",
    ]


def get_doc_path():
    return "doc_classes"
