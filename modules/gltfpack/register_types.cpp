/*************************************************************************/
/*  register_types.cpp                                                   */
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

#include "register_types.h"
#include "core/string/ustring.h"

#ifdef TOOLS_ENABLED

#include "../../thirdparty/meshoptimizer/gltf/gltfpack.h"

void initialize_gltfpack_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	char *input = nullptr;
	char *output = nullptr;
	{
		String path;
		String filename;
		cgltf_data *data = 0;
		std::vector<Mesh> meshes;
		std::vector<Animation> animations;
		std::string extras;

		std::string iext = getExtension(input);
		std::string oext = output ? getExtension(output) : "";

		if (iext == ".gltf" || iext == ".glb") {
			const char *error = 0;
			data = parseGltf(input, meshes, animations, extras, &error);

			if (error) {
				fprintf(stderr, "Error loading %s: %s\n", input, error);
				return 2;
			}
		} else if (iext == ".obj") {
			const char *error = 0;
			data = parseObj(input, meshes, &error);

			if (!data) {
				fprintf(stderr, "Error loading %s: %s\n", input, error);
				return 2;
			}
		} else {
			fprintf(stderr, "Error loading %s: unknown extension (expected .gltf or .glb or .obj)\n", input);
			return 2;
		}

#ifndef WITH_BASISU
		if (data->images_count && settings.texture_ktx2) {
			fprintf(stderr, "Error: gltfpack was built without BasisU support, texture compression is not available\n");
#ifdef __wasi__
			fprintf(stderr, "Note: node.js builds do not support BasisU due to lack of platform features; download a native build from https://github.com/zeux/meshoptimizer/releases\n");
#endif
			return 3;
		}
#endif

		if (oext == ".glb") {
			settings.texture_embed = true;
		}

		std::string json, bin, fallback;
		size_t fallback_size = 0;
		process(data, input, output, report, meshes, animations, extras, settings, json, bin, fallback, fallback_size);

		cgltf_free(data);

		if (!output) {
			return 0;
		}

		if (oext == ".gltf") {
			std::string binpath = output;
			binpath.replace(binpath.size() - 5, 5, ".bin");

			std::string fbpath = output;
			fbpath.replace(fbpath.size() - 5, 5, ".fallback.bin");

			FILE *outjson = fopen(output, "wb");
			FILE *outbin = fopen(binpath.c_str(), "wb");
			FILE *outfb = settings.fallback ? fopen(fbpath.c_str(), "wb") : NULL;
			if (!outjson || !outbin || (!outfb && settings.fallback)) {
				fprintf(stderr, "Error saving %s\n", output);
				return 4;
			}

			std::string bufferspec = getBufferSpec(getBaseName(binpath.c_str()), bin.size(), settings.fallback ? getBaseName(fbpath.c_str()) : NULL, fallback_size, settings.compress);

			fprintf(outjson, "{");
			fwrite(bufferspec.c_str(), bufferspec.size(), 1, outjson);
			fprintf(outjson, ",");
			fwrite(json.c_str(), json.size(), 1, outjson);
			fprintf(outjson, "}");

			fwrite(bin.c_str(), bin.size(), 1, outbin);

			if (settings.fallback)
				fwrite(fallback.c_str(), fallback.size(), 1, outfb);

			int rc = 0;
			rc |= fclose(outjson);
			rc |= fclose(outbin);
			if (outfb)
				rc |= fclose(outfb);

			if (rc) {
				fprintf(stderr, "Error saving %s\n", output);
				return 4;
			}
		} else if (oext == ".glb") {
			std::string fbpath = output;
			fbpath.replace(fbpath.size() - 4, 4, ".fallback.bin");

			FILE *out = fopen(output, "wb");
			FILE *outfb = settings.fallback ? fopen(fbpath.c_str(), "wb") : NULL;
			if (!out || (!outfb && settings.fallback)) {
				fprintf(stderr, "Error saving %s\n", output);
				return 4;
			}

			std::string bufferspec = getBufferSpec(NULL, bin.size(), settings.fallback ? getBaseName(fbpath.c_str()) : NULL, fallback_size, settings.compress);

			json.insert(0, "{" + bufferspec + ",");
			json.push_back('}');

			while (json.size() % 4)
				json.push_back(' ');

			while (bin.size() % 4)
				bin.push_back('\0');

			writeU32(out, 0x46546C67);
			writeU32(out, 2);
			writeU32(out, uint32_t(12 + 8 + json.size() + 8 + bin.size()));

			writeU32(out, uint32_t(json.size()));
			writeU32(out, 0x4E4F534A);
			fwrite(json.c_str(), json.size(), 1, out);

			writeU32(out, uint32_t(bin.size()));
			writeU32(out, 0x004E4942);
			fwrite(bin.c_str(), bin.size(), 1, out);

			if (settings.fallback)
				fwrite(fallback.c_str(), fallback.size(), 1, outfb);

			int rc = 0;
			rc |= fclose(out);
			if (outfb)
				rc |= fclose(outfb);

			if (rc) {
				fprintf(stderr, "Error saving %s\n", output);
				return 4;
			}
		} else {
			fprintf(stderr, "Error saving %s: unknown extension (expected .gltf or .glb)\n", output);
			return 4;
		}
	}
}

void uninitialize_gltfpack_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

#endif
