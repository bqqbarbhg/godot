def can_build(env, platform):
    return platform in ["linux", "linuxbsd", "osx", "macos"] or platform == "windows" and env.get("use_mingw")


def configure(env):
    pass


def get_doc_classes():
    return [
        "Wasm",
        "StreamPeerWasm",
    ]


def get_doc_path():
    return "doc_classes"
