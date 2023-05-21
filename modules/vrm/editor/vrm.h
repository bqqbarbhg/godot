/**************************************************************************/
/*  vrm.h                                                                 */
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

#ifndef VRM_H
#define VRM_H

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "editor/import/resource_importer_scene.h"

#include "../vrm_extension.h"
#include "modules/gltf/extensions/gltf_document_extension.h"
#include "modules/gltf/gltf_document.h"
#include "modules/gltf/gltf_state.h"

class EditorSceneFormatImporterVRM : public EditorSceneFormatImporter {
	GDCLASS(EditorSceneFormatImporterVRM, EditorSceneFormatImporter);

public:
	virtual uint32_t get_import_flags() const override {
		return IMPORT_SCENE;
	}
	virtual void get_extensions(List<String> *r_extensions) const override {
		r_extensions->push_back("vrm");
	}
	virtual Node *import_scene(const String &p_path, uint32_t p_flags, const HashMap<StringName, Variant> &p_options, List<String> *r_missing_deps, Error *r_err = nullptr) {
		Ref<GLTFDocument> gltf = memnew(GLTFDocument);
		p_flags |= IMPORT_USE_NAMED_SKIN_BINDS;
		Ref<VRMExtension> vrm_extension;
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
	virtual void get_import_options(const String &p_path, List<ResourceImporter::ImportOption> *r_options) override {
	}
	virtual Variant get_option_visibility(const String &p_path, bool p_for_animation, const String &p_option, const HashMap<StringName, Variant> &p_options) override {
		return Variant();
	}
	EditorSceneFormatImporterVRM() {}
};

#endif // VRM_H