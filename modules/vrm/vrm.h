
#ifndef VRM_H
#define VRM_H

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "editor/import/resource_importer_scene.h"

#include "modules/gltf/gltf_document.h"
#include "modules/gltf/extensions/gltf_document_extension.h"
#include "modules/gltf/gltf_state.h"

class VRMImportPlugin : public EditorSceneFormatImporter {
	GDCLASS(VRMImportPlugin, EditorSceneFormatImporter);

public:
	virtual uint32_t get_import_flags() const {
		return IMPORT_SCENE | IMPORT_ANIMATION;
	}
	virtual void get_extensions(List<String> *r_extensions) const {
		r_extensions->push_back("vrm");
	}
	virtual Node *import_scene(const String &p_path, uint32_t p_flags, const HashMap<StringName, Variant> &p_options, List<String> *r_missing_deps, Error *r_err = nullptr) {
		Ref<GLTFDocument> gltf = memnew(GLTFDocument);
		p_flags |= IMPORT_USE_NAMED_SKIN_BINDS;
		Ref<GLTFDocumentExtension> vrm_extension;
		vrm_extension.instantiate();
		gltf->register_gltf_document_extension(vrm_extension, true);
		Ref<GLTFState> state = memnew(GLTFState);
		state->set_handle_binary_image(GLTFState::HANDLE_BINARY_EMBED_AS_BASISU);
		Error err = gltf->append_from_file(p_path, state, p_flags);
		if (err != OK) {
			gltf->unregister_gltf_document_extension(vrm_extension);
			if (r_err) {
				*r_err = err;
			}
			return nullptr;
		}
		Node *generated_scene = gltf->generate_scene(state);
		gltf->unregister_gltf_document_extension(vrm_extension);
		if (r_err) {
			*r_err = OK;
		}
		return generated_scene;
	}
	virtual void get_import_options(const String &p_path, List<ResourceImporter::ImportOption> *r_options) {
	}
	virtual Variant get_option_visibility(const String &p_path, bool p_for_animation, const String &p_option, const HashMap<StringName, Variant> &p_options) {
		return Variant();
	}
	VRMImportPlugin() {}
};

class VRMEditorPlugin : public EditorPlugin {
	GDCLASS(VRMEditorPlugin, EditorPlugin);

private:
	Ref<VRMImportPlugin> import_plugin;

public:
	VRMEditorPlugin() {
		import_plugin.instantiate();
		add_scene_format_importer_plugin(import_plugin);
	}
	~VRMEditorPlugin() {
		remove_scene_format_importer_plugin(import_plugin);
		import_plugin.unref();
	}
};

#endif // VRM_H