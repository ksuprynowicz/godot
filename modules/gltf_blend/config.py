def can_build(env, platform):
    return not env["disable_3d"] and env["tools"]


def configure(env):
    pass


def get_doc_classes():
    return [
        "EditorSceneFormatImporterBlend",
    ]


def get_doc_path():
    return "doc_classes"
