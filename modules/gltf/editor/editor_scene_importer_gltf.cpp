/*************************************************************************/
/*  editor_scene_importer_gltf.cpp                                       */
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

#ifdef TOOLS_ENABLED

#include "editor_scene_importer_gltf.h"

#include "../gltf_document.h"
#include "../gltf_state.h"

#include "core/config/project_settings.h"
#include "editor/editor_settings.h"
#include "scene/resources/animation.h"

uint32_t EditorSceneFormatImporterGLTF::get_import_flags() const {
	return ImportFlags::IMPORT_SCENE | ImportFlags::IMPORT_ANIMATION;
}

void EditorSceneFormatImporterGLTF::get_extensions(List<String> *r_extensions) const {
	r_extensions->push_back("gltf");
	r_extensions->push_back("glb");
}

Node *EditorSceneFormatImporterGLTF::import_scene(const String &p_path, uint32_t p_flags,
		const HashMap<StringName, Variant> &p_options, int p_bake_fps,
		List<String> *r_missing_deps, Error *r_err) {
	bool gltfpack_enabled = GLOBAL_GET("filesystem/filter/gltfpack/enabled");
	String gltfpack_path;
	if (gltfpack_enabled && EditorSettings::get_singleton()->has_setting("filesystem/filter/gltfpack_path")) {
		gltfpack_path = EDITOR_GET("filesystem/filter/gltfpack_path");
	}
		Ref<GLTFDocument> doc;
		doc.instantiate();
		Ref<GLTFState> state;
		state.instantiate();
	if (!gltfpack_path.is_empty()) {
		// Get global paths for source and sink.
		const String source = p_path;
		const String source_global = ProjectSettings::get_singleton()->globalize_path(source);
		const String sink = ProjectSettings::get_singleton()->get_imported_files_path().plus_file(
				vformat("sink-gltfpack-%s-%s.glb", p_path.get_file().get_basename(), p_path.md5_text()));
		const String sink_global = ProjectSettings::get_singleton()->globalize_path(sink);

		List<String> args;
		args.push_back("-o");
		args.push_back(sink_global);
		args.push_back("-i");
		args.push_back(source_global);

		args.push_back("-kn"); // Keep named nodes and meshes attached to named nodes so that named nodes can be transformed externally.
		args.push_back("-km"); // Keep named materials and disable named material merging.
		args.push_back("-ke"); // Keep extras data.
		args.push_back("-tc"); // -tc: convert all textures to KTX2 with BasisU supercompression
		args.push_back("-tu"); // use UASTC when encoding textures (much higher quality and much larger size)
		// TODO: Fire 2022-08-14 Need to implement mesh optimizer gltf extension

		String standard_out;
		int ret;
		OS::get_singleton()->execute(gltfpack_path, args, &standard_out, &ret, true);
		print_verbose(gltfpack_path);
		Vector<String> args_printed;
		for (String arg : args) {
			args_printed.push_back(arg);
		}
		print_verbose(String(" ").join(args_printed));
		print_verbose(standard_out);

		if (ret != 0) {
			ERR_PRINT(vformat("gltfpack filter failed with error: %d.", ret));
			return nullptr;
		}
		Error err = doc->append_from_file(sink, state, p_flags, p_bake_fps);
		if (err != OK) {
			if (r_err) {
				*r_err = err;
			}
			return nullptr;
		}

		if (p_options.has("animation/import")) {
			state->set_create_animations(bool(p_options["animation/import"]));
		}
		return doc->generate_scene(state, p_bake_fps);
		
	} else {
		Error err = doc->append_from_file(p_path, state, p_flags, p_bake_fps);
		if (err != OK) {
			if (r_err) {
				*r_err = err;
			}
			return nullptr;
		}
		if (p_options.has("animation/import")) {
			state->set_create_animations(bool(p_options["animation/import"]));
		}
		return doc->generate_scene(state, p_bake_fps);
	}
	return nullptr;
}

#endif // TOOLS_ENABLED
