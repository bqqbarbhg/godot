/**************************************************************************/
/*  psd_texture.cpp                                                       */
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

#include "psd_texture.h"

#include "PsdNativeFile_Godot.h"
#include "core/error/error_list.h"
#include "core/error/error_macros.h"
#include "core/templates/pair.h"
#include "core/variant/variant.h"
#include "scene/resources/texture.h"
#include "thirdparty/libwebp/src/webp/mux.h"

PSD_USING_NAMESPACE;

// helpers for reading PSDs
namespace {
static const unsigned int CHANNEL_NOT_FOUND = UINT_MAX;

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T, typename DataHolder>
static void *ExpandChannelToCanvas(Allocator *allocator, const DataHolder *layer, const void *data, unsigned int canvasWidth, unsigned int canvasHeight) {
	T *canvasData = static_cast<T *>(allocator->Allocate(sizeof(T) * canvasWidth * canvasHeight, 16u));
	memset(canvasData, 0u, sizeof(T) * canvasWidth * canvasHeight);

	imageUtil::CopyLayerData(static_cast<const T *>(data), canvasData, layer->left, layer->top, layer->right, layer->bottom, canvasWidth, canvasHeight);

	return canvasData;
}

//canvasData[0] = ExpandChannelToCanvas(document, &allocator, layer, &layer->channels[indexR]);

static void *ExpandChannelToCanvas(const Document *document, Allocator *allocator, Layer *layer, Channel *channel) {
	if (document->bitsPerChannel == 8)
		return ExpandChannelToCanvas<uint8_t>(allocator, layer, channel->data, document->width, document->height);
	else if (document->bitsPerChannel == 16)
		return ExpandChannelToCanvas<uint16_t>(allocator, layer, channel->data, document->width, document->height);
	else if (document->bitsPerChannel == 32)
		return ExpandChannelToCanvas<float32_t>(allocator, layer, channel->data, document->width, document->height);

	return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
static void *ExpandMaskToCanvas(const Document *document, Allocator *allocator, T *mask) {
	if (document->bitsPerChannel == 8)
		return ExpandChannelToCanvas<uint8_t>(allocator, mask, mask->data, document->width, document->height);
	else if (document->bitsPerChannel == 16)
		return ExpandChannelToCanvas<uint16_t>(allocator, mask, mask->data, document->width, document->height);
	else if (document->bitsPerChannel == 32)
		return ExpandChannelToCanvas<float32_t>(allocator, mask, mask->data, document->width, document->height);

	return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
unsigned int FindChannel(Layer *layer, int16_t channelType) {
	for (unsigned int i = 0; i < layer->channelCount; ++i) {
		Channel *channel = &layer->channels[i];
		if (channel->data && channel->type == channelType)
			return i;
	}

	return CHANNEL_NOT_FOUND;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
T *CreateInterleavedImage(Allocator *allocator, const void *srcR, const void *srcG, const void *srcB, unsigned int width, unsigned int height) {
	T *image = static_cast<T *>(allocator->Allocate(width * height * 4u * sizeof(T), 16u));

	const T *r = static_cast<const T *>(srcR);
	const T *g = static_cast<const T *>(srcG);
	const T *b = static_cast<const T *>(srcB);
	imageUtil::InterleaveRGB(r, g, b, T(0), image, width, height);

	return image;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
T *CreateInterleavedImage(Allocator *allocator, const void *srcR, const void *srcG, const void *srcB, const void *srcA, unsigned int width, unsigned int height) {
	T *image = static_cast<T *>(allocator->Allocate(width * height * 4u * sizeof(T), 16u));

	const T *r = static_cast<const T *>(srcR);
	const T *g = static_cast<const T *>(srcG);
	const T *b = static_cast<const T *>(srcB);
	const T *a = static_cast<const T *>(srcA);
	imageUtil::InterleaveRGBA(r, g, b, a, image, width, height);

	return image;
}
} // namespace

Error PSDTexture::load_image(Ref<Image> p_image, Ref<FileAccess> f, BitField<ImageFormatLoader::LoaderFlags> p_flags, float p_scale) {
	uint64_t len = f->get_length();
	Vector<uint8_t> data;
	data.resize(len);
	f->get_buffer(data.ptrw(), len);

	layers.clear();

	const std::wstring rawFile = L"";

	MallocAllocator allocator;
	NativeFile file(&allocator);

	file.OpenBuffer(data.ptr(), data.size());

	// create a new document that can be used for extracting different sections from the PSD.
	// additionally, the document stores information like width, height, bits per pixel, etc.
	Document *document = CreateDocument(&file, &allocator);
	ERR_FAIL_COND_V_MSG(!document, ERR_FILE_CORRUPT, "PSD document initialization failed.");

	ERR_FAIL_COND_V_MSG(document->colorMode != colorMode::RGB, ERR_INVALID_DATA, "PSD uses unsupported color mode.");

	// extract image resources section.
	// this gives access to the ICC profile, EXIF data and XMP metadata.
	{
		ImageResourcesSection *imageResourcesSection = ParseImageResourcesSection(document, &file, &allocator);

		DestroyImageResourcesSection(imageResourcesSection, &allocator);
	}

	// extract all layers and masks.
	// bool hasTransparencyMask = false; // Not being used.
	LayerMaskSection *layerMaskSection = ParseLayerMaskSection(document, &file, &allocator);
	Vector<Ref<Image>> images;
	if (layerMaskSection) {
		// hasTransparencyMask = layerMaskSection->hasTransparencyMask; // Not being used.

		// extract all layers one by one. this should be done in parallel for maximum efficiency.
		for (unsigned int i = 0; i < layerMaskSection->layerCount; ++i) {
			Layer *layer = &layerMaskSection->layers[i];
			if (layer->type != layerType::ANY) {
				continue;
			}
			ExtractLayer(document, &file, &allocator, layer);

			// check availability of R, G, B, and A channels.
			// we need to determine the indices of channels individually, because there is no guarantee that R is the first channel,
			// G is the second, B is the third, and so on.
			const unsigned int indexR = FindChannel(layer, channelType::R);
			const unsigned int indexG = FindChannel(layer, channelType::G);
			const unsigned int indexB = FindChannel(layer, channelType::B);
			const unsigned int indexA = FindChannel(layer, channelType::TRANSPARENCY_MASK);

			unsigned int layerWidth = document->width;
			unsigned int layerHeight = document->height;
			if (cropToCanvas == false) {
				layerHeight = (unsigned int)(layer->bottom - layer->top);
				layerWidth = (unsigned int)(layer->right - layer->left);
			}

			// note that channel data is only as big as the layer it belongs to, e.g. it can be smaller or bigger than the canvas,
			// depending on where it is positioned. therefore, we use the provided utility functions to expand/shrink the channel data
			// to the canvas size. of course, you can work with the channel data directly if you need to.
			void *canvasData[4] = {};
			unsigned int channelCount = 0u;
			int channelType = -1;

			if ((indexR != CHANNEL_NOT_FOUND) && (indexG != CHANNEL_NOT_FOUND) && (indexB != CHANNEL_NOT_FOUND)) {
				// RGB channels were found.
				if (cropToCanvas == true) {
					canvasData[0] = ExpandChannelToCanvas(document, &allocator, layer, &layer->channels[indexR]);
					canvasData[1] = ExpandChannelToCanvas(document, &allocator, layer, &layer->channels[indexG]);
					canvasData[2] = ExpandChannelToCanvas(document, &allocator, layer, &layer->channels[indexB]);
				} else {
					canvasData[0] = layer->channels[indexR].data;
					canvasData[1] = layer->channels[indexG].data;
					canvasData[2] = layer->channels[indexB].data;
				}
				channelCount = 3u;
				channelType = COLOR_SPACE_NAME::RGB;

				if (indexA != CHANNEL_NOT_FOUND) {
					// A channel was also found.
					if (cropToCanvas == true) {
						canvasData[3] = ExpandChannelToCanvas(document, &allocator, layer, &layer->channels[indexA]);
					} else {
						canvasData[3] = layer->channels[indexA].data;
					}
					channelCount = 4u;
					channelType = COLOR_SPACE_NAME::RGBA;
				}
			}

			// interleave the different pieces of planar canvas data into one RGB or RGBA image, depending on what channels
			// we found, and what color mode the document is stored in.
			uint8_t *image8 = nullptr;
			uint16_t *image16 = nullptr;
			float32_t *image32 = nullptr;
			if (channelCount == 3u) {
				if (document->bitsPerChannel == 8) {
					image8 = CreateInterleavedImage<uint8_t>(&allocator, canvasData[0], canvasData[1], canvasData[2], layerWidth, layerHeight);
				} else if (document->bitsPerChannel == 16) {
					image16 = CreateInterleavedImage<uint16_t>(&allocator, canvasData[0], canvasData[1], canvasData[2], layerWidth, layerHeight);
				} else if (document->bitsPerChannel == 32) {
					image32 = CreateInterleavedImage<float32_t>(&allocator, canvasData[0], canvasData[1], canvasData[2], layerWidth, layerHeight);
				}
			} else if (channelCount == 4u) {
				if (document->bitsPerChannel == 8) {
					image8 = CreateInterleavedImage<uint8_t>(&allocator, canvasData[0], canvasData[1], canvasData[2], canvasData[3], layerWidth, layerHeight);
				} else if (document->bitsPerChannel == 16) {
					image16 = CreateInterleavedImage<uint16_t>(&allocator, canvasData[0], canvasData[1], canvasData[2], canvasData[3], layerWidth, layerHeight);
				} else if (document->bitsPerChannel == 32) {
					image32 = CreateInterleavedImage<float32_t>(&allocator, canvasData[0], canvasData[1], canvasData[2], canvasData[3], layerWidth, layerHeight);
				}
			}

			// ONLY free canvasData if the channel was actually copied! Otherwise the channel data is already deleted here!
			if (cropToCanvas == true) {
				allocator.Free(canvasData[0]);
				allocator.Free(canvasData[1]);
				allocator.Free(canvasData[2]);
				allocator.Free(canvasData[3]);
			}

			// get the layer name.
			// Unicode data is preferred because it is not truncated by Photoshop, but unfortunately it is optional.
			// fall back to the ASCII name in case no Unicode name was found.
			std::wstringstream ssLayerName;
			if (layer->utf16Name) {
				ssLayerName << reinterpret_cast<wchar_t *>(layer->utf16Name);
			} else {
				ssLayerName << layer->name.c_str();
			}
			std::wstring wslayerName = ssLayerName.str();
			const wchar_t *layerName = wslayerName.c_str();

			// at this point, image8, image16 or image32 store either a 8-bit, 16-bit, or 32-bit image, respectively.
			// the image data is stored in interleaved RGB or RGBA, and has the size "document->width*document->height".
			// it is up to you to do whatever you want with the image data. in the sample, we simply write the image to a .TGA file.

			if (document->bitsPerChannel == 8u) {
				if (channelType == -1) {
					continue;
				}
				Vector<uint8_t> data_vector;

				int bytesPerPixel = 4;
				Image::Format imageFormat = Image::FORMAT_RGBA8;

				if (channelType == COLOR_SPACE_NAME::RGB) {
					bytesPerPixel = 3;
					imageFormat = Image::FORMAT_RGB8;
				} else if (channelType == COLOR_SPACE_NAME::RGBA) {
					bytesPerPixel = 4;
					imageFormat = Image::FORMAT_RGBA8;
				} else if (channelType == COLOR_SPACE_NAME::MONOCHROME) {
					bytesPerPixel = 1;
					imageFormat = Image::FORMAT_L8;
				}

				for (unsigned int i = 0; i < layerWidth * layerHeight * bytesPerPixel; i++) {
					data_vector.push_back(image8[i]);
				}

				Ref<Image> image_layer = Image::create_from_data(layerWidth, layerHeight, false, imageFormat, data_vector);
				image_layer->set_name(layerName);
				images.push_back(image_layer);
			}

			allocator.Free(image8);
			allocator.Free(image16);
			allocator.Free(image32);
		}
		DestroyLayerMaskSection(layerMaskSection, &allocator);
	}
	Ref<Image> img = images[0];
	images.remove_at(0);
	for (Ref<Image> new_img : images) {
		if (new_img->is_empty()) {
			continue;
		}
		Rect2i rect(0, 0, document->width, document->height);
		img->blend_rect(new_img, rect, Vector2i(0, 0));
	}
	if (p_scale != 0.0f && p_scale != 1.0f) {
		img->resize(p_scale * img->get_width(), p_scale * img->get_height());
	}
	p_image->set_data(img->get_width(), img->get_height(), false, Image::FORMAT_RGBA8, img->get_data());
	// don't forget to destroy the document, and close the file.
	DestroyDocument(document, &allocator);
	if (!images.size()) {
		return OK;
	}
	return OK;
}

void PSDTexture::_bind_methods() {
}

PSDTexture::PSDTexture() {
}

PSDTexture::~PSDTexture() {
}
void PSDTexture::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("psd");
}
