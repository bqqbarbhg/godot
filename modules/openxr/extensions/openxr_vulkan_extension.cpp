/*************************************************************************/
/*  openxr_vulkan_extension.cpp                                          */
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

#include "core/string/print_string.h"

#include "modules/openxr/extensions/openxr_vulkan_extension.h"
#include "modules/openxr/openxr_api.h"
#include "modules/openxr/openxr_util.h"
#include "servers/rendering/renderer_rd/renderer_storage_rd.h"
#include "servers/rendering/rendering_server_globals.h"
#include "servers/rendering_server.h"

// need to include Vulkan so we know of type definitions
#define XR_USE_GRAPHICS_API_VULKAN

#ifdef WINDOWS_ENABLED
// Including windows.h here is absolutely evil, we shouldn't be doing this outside of platform
// however due to the way the openxr headers are put together, we have no choice.
#include <windows.h>
#endif

// include platform dependent structs
#include <openxr/openxr_platform.h>

PFN_xrGetVulkanGraphicsRequirements2KHR xrGetVulkanGraphicsRequirements2KHR_ptr = nullptr;
PFN_xrCreateVulkanInstanceKHR xrCreateVulkanInstanceKHR_ptr = nullptr;
PFN_xrGetVulkanGraphicsDevice2KHR xrGetVulkanGraphicsDevice2KHR_ptr = nullptr;
PFN_xrCreateVulkanDeviceKHR xrCreateVulkanDeviceKHR_ptr = nullptr;

OpenXRVulkanExtension::OpenXRVulkanExtension(OpenXRAPI *p_openxr_api) :
		OpenXRGraphicsExtensionWrapper(p_openxr_api) {
	VulkanContext::set_vulkan_hooks(this);

	request_extensions[XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME] = nullptr; // must be available

	ERR_FAIL_NULL(openxr_api);
}

OpenXRVulkanExtension::~OpenXRVulkanExtension() {
	VulkanContext::set_vulkan_hooks(nullptr);
}

void OpenXRVulkanExtension::on_instance_created(const XrInstance p_instance) {
	XrResult result;

	ERR_FAIL_NULL(openxr_api);

	// Obtain pointers to functions we're accessing here, they are (not yet) part of core.
	result = xrGetInstanceProcAddr(p_instance, "xrGetVulkanGraphicsRequirements2KHR", (PFN_xrVoidFunction *)&xrGetVulkanGraphicsRequirements2KHR_ptr);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to xrGetVulkanGraphicsRequirements2KHR entry point [", openxr_api->get_error_string(result), "]");
	}

	result = xrGetInstanceProcAddr(p_instance, "xrCreateVulkanInstanceKHR", (PFN_xrVoidFunction *)&xrCreateVulkanInstanceKHR_ptr);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to xrCreateVulkanInstanceKHR entry point [", openxr_api->get_error_string(result), "]");
	}

	result = xrGetInstanceProcAddr(p_instance, "xrGetVulkanGraphicsDevice2KHR", (PFN_xrVoidFunction *)&xrGetVulkanGraphicsDevice2KHR_ptr);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to xrGetVulkanGraphicsDevice2KHR entry point [", openxr_api->get_error_string(result), "]");
	}

	result = xrGetInstanceProcAddr(p_instance, "xrCreateVulkanDeviceKHR", (PFN_xrVoidFunction *)&xrCreateVulkanDeviceKHR_ptr);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to xrCreateVulkanDeviceKHR entry point [", openxr_api->get_error_string(result), "]");
	}
}

XrResult OpenXRVulkanExtension::xrGetVulkanGraphicsRequirements2KHR(XrInstance p_instance, XrSystemId p_system_id, XrGraphicsRequirementsVulkanKHR *p_graphics_requirements) {
	ERR_FAIL_NULL_V(xrGetVulkanGraphicsRequirements2KHR_ptr, XR_ERROR_HANDLE_INVALID);

	return (*xrGetVulkanGraphicsRequirements2KHR_ptr)(p_instance, p_system_id, p_graphics_requirements);
}

bool OpenXRVulkanExtension::check_graphics_api_support(XrVersion p_desired_version) {
	ERR_FAIL_NULL_V(openxr_api, false);

	XrGraphicsRequirementsVulkan2KHR vulkan_requirements = {
		XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR, // type
		nullptr, // next
		0, // minApiVersionSupported
		0 // maxApiVersionSupported
	};

	XrResult result = xrGetVulkanGraphicsRequirements2KHR(openxr_api->get_instance(), openxr_api->get_system_id(), &vulkan_requirements);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get vulkan graphics requirements [", openxr_api->get_error_string(result), "]");
		return false;
	}

	// #ifdef DEBUG
	print_line("OpenXR: XrGraphicsRequirementsVulkan2KHR:");
	print_line(" - minApiVersionSupported: ", OpenXRUtil::make_xr_version_string(vulkan_requirements.minApiVersionSupported));
	print_line(" - maxApiVersionSupported: ", OpenXRUtil::make_xr_version_string(vulkan_requirements.maxApiVersionSupported));
	// #endif

	if (p_desired_version < vulkan_requirements.minApiVersionSupported) {
		("OpenXR: Requested Vulkan version does not meet the minimum version this runtime supports.");
		print_line("- desired_version ", OpenXRUtil::make_xr_version_string(p_desired_version));
		print_line("- minApiVersionSupported ", OpenXRUtil::make_xr_version_string(vulkan_requirements.minApiVersionSupported));
		print_line("- maxApiVersionSupported ", OpenXRUtil::make_xr_version_string(vulkan_requirements.maxApiVersionSupported));
		return false;
	}

	if (p_desired_version > vulkan_requirements.maxApiVersionSupported) {
		print_line("OpenXR: Requested Vulkan version exceeds the maximum version this runtime has been tested on and is known to support.");
		print_line("- desired_version ", OpenXRUtil::make_xr_version_string(p_desired_version));
		print_line("- minApiVersionSupported ", OpenXRUtil::make_xr_version_string(vulkan_requirements.minApiVersionSupported));
		print_line("- maxApiVersionSupported ", OpenXRUtil::make_xr_version_string(vulkan_requirements.maxApiVersionSupported));
	}

	return true;
}

XrResult OpenXRVulkanExtension::xrCreateVulkanInstanceKHR(XrInstance p_instance, const XrVulkanInstanceCreateInfoKHR *p_create_info, VkInstance *r_vulkan_instance, VkResult *r_vulkan_result) {
	ERR_FAIL_NULL_V(xrCreateVulkanInstanceKHR_ptr, XR_ERROR_HANDLE_INVALID);

	return (*xrCreateVulkanInstanceKHR_ptr)(p_instance, p_create_info, r_vulkan_instance, r_vulkan_result);
}

bool OpenXRVulkanExtension::create_vulkan_instance(const VkInstanceCreateInfo *p_vulkan_create_info, VkInstance *r_instance) {
	// get the vulkan version we are creating
	uint32_t vulkan_version = p_vulkan_create_info->pApplicationInfo->apiVersion;
	uint32_t major_version = VK_VERSION_MAJOR(vulkan_version);
	uint32_t minor_version = VK_VERSION_MINOR(vulkan_version);
	uint32_t patch_version = VK_VERSION_PATCH(vulkan_version);
	XrVersion desired_version = XR_MAKE_VERSION(major_version, minor_version, patch_version);

	// check if this is supported
	if (!check_graphics_api_support(desired_version)) {
		return false;
	}

	XrVulkanInstanceCreateInfoKHR xr_vulkan_instance_info = {
		XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR, // type
		nullptr, // next
		openxr_api->get_system_id(), // systemId
		0, // createFlags
		vkGetInstanceProcAddr, // pfnGetInstanceProcAddr
		p_vulkan_create_info, // vulkanCreateInfo
		nullptr, // vulkanAllocator
	};

	VkResult vk_result = VK_SUCCESS;
	XrResult result = xrCreateVulkanInstanceKHR(openxr_api->get_instance(), &xr_vulkan_instance_info, &vulkan_instance, &vk_result);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to create vulkan instance [", openxr_api->get_error_string(result), "]");
		return false;
	}

	ERR_FAIL_COND_V_MSG(vk_result == VK_ERROR_INCOMPATIBLE_DRIVER, false,
			"Cannot find a compatible Vulkan installable client driver (ICD).\n\n"
			"vkCreateInstance Failure");
	ERR_FAIL_COND_V_MSG(vk_result == VK_ERROR_EXTENSION_NOT_PRESENT, false,
			"Cannot find a specified extension library.\n"
			"Make sure your layers path is set appropriately.\n"
			"vkCreateInstance Failure");
	ERR_FAIL_COND_V_MSG(vk_result, false,
			"vkCreateInstance failed.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n"
			"vkCreateInstance Failure");

	*r_instance = vulkan_instance;

	return true;
}

XrResult OpenXRVulkanExtension::xrGetVulkanGraphicsDevice2KHR(XrInstance p_instance, const XrVulkanGraphicsDeviceGetInfoKHR *p_get_info, VkPhysicalDevice *r_vulkan_physical_device) {
	ERR_FAIL_NULL_V(xrGetVulkanGraphicsDevice2KHR_ptr, XR_ERROR_HANDLE_INVALID);

	return (*xrGetVulkanGraphicsDevice2KHR_ptr)(p_instance, p_get_info, r_vulkan_physical_device);
}

bool OpenXRVulkanExtension::get_physical_device(VkPhysicalDevice *r_device) {
	ERR_FAIL_NULL_V(openxr_api, false);

	XrVulkanGraphicsDeviceGetInfoKHR get_info = {
		XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR, // type
		nullptr, // next
		openxr_api->get_system_id(), // systemId
		vulkan_instance, // vulkanInstance
	};

	XrResult result = xrGetVulkanGraphicsDevice2KHR(openxr_api->get_instance(), &get_info, &vulkan_physical_device);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to obtain vulkan physical device [", openxr_api->get_error_string(result), "]");
		return false;
	}

	*r_device = vulkan_physical_device;

	return true;
}

XrResult OpenXRVulkanExtension::xrCreateVulkanDeviceKHR(XrInstance p_instance, const XrVulkanDeviceCreateInfoKHR *p_create_info, VkDevice *r_device, VkResult *r_result) {
	ERR_FAIL_NULL_V(xrCreateVulkanDeviceKHR_ptr, XR_ERROR_HANDLE_INVALID);

	return (*xrCreateVulkanDeviceKHR_ptr)(p_instance, p_create_info, r_device, r_result);
}

bool OpenXRVulkanExtension::create_vulkan_device(const VkDeviceCreateInfo *p_device_create_info, VkDevice *r_device) {
	ERR_FAIL_NULL_V(openxr_api, false);

	// the first entry in our queue list should be the one we need to remember...
	vulkan_queue_family_index = p_device_create_info->pQueueCreateInfos[0].queueFamilyIndex;
	vulkan_queue_index = 0; // ??

	XrVulkanDeviceCreateInfoKHR create_info = {
		XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR, // type
		nullptr, // next
		openxr_api->get_system_id(), // systemId
		0, // createFlags
		vkGetInstanceProcAddr, // pfnGetInstanceProcAddr
		vulkan_physical_device, // vulkanPhysicalDevice
		p_device_create_info, // vulkanCreateInfo
		nullptr // vulkanAllocator
	};

	VkResult vk_result = VK_SUCCESS;
	XrResult result = xrCreateVulkanDeviceKHR(openxr_api->get_instance(), &create_info, &vulkan_device, &vk_result);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to create vulkan device [", openxr_api->get_error_string(result), "]");
		return false;
	}

	if (vk_result != VK_SUCCESS) {
		print_line("OpenXR: Failed to create vulkan device [vulkan error", vk_result, "]");
	}

	*r_device = vulkan_device;

	return true;
}

XrGraphicsBindingVulkanKHR OpenXRVulkanExtension::graphics_binding_vulkan;

void **OpenXRVulkanExtension::set_session_create_and_get_next_pointer(void **p_property) {
	graphics_binding_vulkan.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
	graphics_binding_vulkan.next = nullptr;
	graphics_binding_vulkan.instance = vulkan_instance;
	graphics_binding_vulkan.physicalDevice = vulkan_physical_device;
	graphics_binding_vulkan.device = vulkan_device;
	graphics_binding_vulkan.queueFamilyIndex = vulkan_queue_family_index;
	graphics_binding_vulkan.queueIndex = vulkan_queue_index;

	*p_property = &graphics_binding_vulkan;
	return const_cast<void **>(&graphics_binding_vulkan.next);
}

void OpenXRVulkanExtension::get_usable_swapchain_formats(Vector<int64_t> &p_usable_swap_chains) {
	// We might want to do more here especially if we keep things in linear color space
	// Possibly add in R10G10B10A2 as an option if we're using the mobile renderer.
	p_usable_swap_chains.push_back(VK_FORMAT_R8G8B8A8_SRGB);
	p_usable_swap_chains.push_back(VK_FORMAT_B8G8R8A8_SRGB);
	p_usable_swap_chains.push_back(VK_FORMAT_R8G8B8A8_UINT);
	p_usable_swap_chains.push_back(VK_FORMAT_B8G8R8A8_UINT);
}

bool OpenXRVulkanExtension::get_swapchain_image_data(XrSwapchain p_swapchain, int64_t p_swapchain_format, uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size, void **r_swapchain_graphics_data) {
	XrSwapchainImageVulkanKHR *images = nullptr;

	RenderingServer *rendering_server = RenderingServer::get_singleton();
	ERR_FAIL_NULL_V(rendering_server, false);
	RenderingDevice *rendering_device = rendering_server->get_rendering_device();
	ERR_FAIL_NULL_V(rendering_device, false);

	uint32_t swapchain_length;
	XrResult result = xrEnumerateSwapchainImages(p_swapchain, 0, &swapchain_length, nullptr);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get swapchaim image count [", openxr_api->get_error_string(result), "]");
		return false;
	}

	images = (XrSwapchainImageVulkanKHR *)memalloc(sizeof(XrSwapchainImageVulkanKHR) * swapchain_length);
	ERR_FAIL_NULL_V_MSG(images, false, "OpenXR Couldn't allocate memory for swap chain image");

	for (uint64_t i = 0; i < swapchain_length; i++) {
		images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
		images[i].next = nullptr;
		images[i].image = nullptr;
	}

	result = xrEnumerateSwapchainImages(p_swapchain, swapchain_length, &swapchain_length, (XrSwapchainImageBaseHeader *)images);
	if (XR_FAILED(result)) {
		print_line("OpenXR: Failed to get swapchaim images [", openxr_api->get_error_string(result), "]");
		memfree(images);
		return false;
	}

	// SwapchainGraphicsData *data = (SwapchainGraphicsData *)memalloc(sizeof(SwapchainGraphicsData));
	SwapchainGraphicsData *data = memnew(SwapchainGraphicsData);
	if (data == nullptr) {
		print_line("OpenXR: Failed to allocate memory for swapchain data");
		memfree(images);
		return false;
	}
	*r_swapchain_graphics_data = data;
	data->is_multiview = (p_array_size > 1);

	RenderingDevice::DataFormat format = RenderingDevice::DATA_FORMAT_R8G8B8A8_SRGB; // TODO set this based on p_swapchain_format
	RenderingDevice::TextureSamples samples = RenderingDevice::TEXTURE_SAMPLES_1; // TODO set this based on p_sample_count
	uint64_t usage_flags = RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT | RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;

	Vector<RID> image_rids;
	Vector<RID> framebuffers;

	// create Godot texture objects for each entry in our swapchain
	for (uint64_t i = 0; i < swapchain_length; i++) {
		RID image_rid = rendering_device->texture_create_from_extension(
				p_array_size == 1 ? RenderingDevice::TEXTURE_TYPE_2D : RenderingDevice::TEXTURE_TYPE_2D_ARRAY,
				format,
				samples,
				usage_flags,
				(uint64_t)images[i].image,
				p_width,
				p_height,
				1,
				p_array_size);

		image_rids.push_back(image_rid);

		{
			Vector<RID> fb;
			fb.push_back(image_rid);

			RID fb_rid = rendering_device->framebuffer_create(fb, RenderingDevice::INVALID_ID, p_array_size);
			framebuffers.push_back(fb_rid);
		}
	}

	data->image_rids = image_rids;
	data->framebuffers = framebuffers;

	memfree(images);

	return true;
}

bool OpenXRVulkanExtension::create_projection_fov(const XrFovf p_fov, double p_z_near, double p_z_far, CameraMatrix &r_camera_matrix) {
	// Even though this is a Vulkan renderer we're using OpenGL coordinate systems
	XrMatrix4x4f matrix;
	XrMatrix4x4f_CreateProjectionFov(&matrix, GRAPHICS_OPENGL, p_fov, (float)p_z_near, (float)p_z_far);

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			r_camera_matrix.matrix[j][i] = matrix.m[j * 4 + i];
		}
	}

	return true;
}

bool OpenXRVulkanExtension::copy_render_target_to_image(RID p_from_render_target, void *p_swapchain_graphics_data, int p_image_index) {
	SwapchainGraphicsData *data = (SwapchainGraphicsData *)p_swapchain_graphics_data;
	ERR_FAIL_NULL_V(data, false);
	ERR_FAIL_COND_V(p_from_render_target.is_null(), false);
	ERR_FAIL_NULL_V(RendererStorageRD::base_singleton, false);

	RID source_image = RendererStorageRD::base_singleton->render_target_get_rd_texture(p_from_render_target);
	ERR_FAIL_COND_V(source_image.is_null(), false);

	RID depth_image; // TODO implement

	ERR_FAIL_INDEX_V(p_image_index, data->framebuffers.size(), false);
	RID fb = data->framebuffers[p_image_index];
	ERR_FAIL_COND_V(fb.is_null(), false);

	// Our vulkan extension can only be used in conjunction with our vulkan renderer.
	// We need access to the effects object in order to have access to our copy logic.
	// Breaking all the rules but there is no nice way to do this.
	EffectsRD *effects = RendererStorageRD::base_singleton->get_effects();
	ERR_FAIL_NULL_V(effects, false);
	effects->copy_to_fb_rect(source_image, fb, Rect2i(), false, false, false, false, depth_image, data->is_multiview);

	return true;
}

void OpenXRVulkanExtension::cleanup_swapchain_graphics_data(void **p_swapchain_graphics_data) {
	if (*p_swapchain_graphics_data == nullptr) {
		return;
	}

	SwapchainGraphicsData *data = (SwapchainGraphicsData *)*p_swapchain_graphics_data;

	RenderingServer *rendering_server = RenderingServer::get_singleton();
	ERR_FAIL_NULL(rendering_server);
	RenderingDevice *rendering_device = rendering_server->get_rendering_device();
	ERR_FAIL_NULL(rendering_device);

	for (int i = 0; i < data->image_rids.size(); i++) {
		// This should clean up our RIDs and associated texture objects but shouldn't destroy the images, they are owned by our XrSwapchain
		rendering_device->free(data->image_rids[i]);
	}
	data->image_rids.clear();

	for (int i = 0; i < data->framebuffers.size(); i++) {
		// This should clean up our RIDs and associated texture objects but shouldn't destroy the images, they are owned by our XrSwapchain
		rendering_device->free(data->framebuffers[i]);
	}
	data->framebuffers.clear();

	memdelete(data);
	*p_swapchain_graphics_data = nullptr;
}

#define ENUM_TO_STRING_CASE(e) \
	case e: {                  \
		return String(#e);     \
	} break;

String OpenXRVulkanExtension::get_swapchain_format_name(int64_t p_swapchain_format) const {
	// This really should be in vulkan_context...
	VkFormat format = VkFormat(p_swapchain_format);
	switch (format) {
		ENUM_TO_STRING_CASE(VK_FORMAT_UNDEFINED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R4G4_UNORM_PACK8)
		ENUM_TO_STRING_CASE(VK_FORMAT_R4G4B4A4_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B4G4R4A4_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R5G6B5_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B5G6R5_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R5G5B5A1_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B5G5R5A1_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_A1R5G5B5_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_USCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SSCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_UINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SRGB_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_SNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_USCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_SSCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_UINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_SINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_SNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_USCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_SSCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_UINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_SINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32A32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32A32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32A32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64A64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64A64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64A64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B10G11R11_UFLOAT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_D16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_X8_D24_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_D32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_D16_UNORM_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_D24_UNORM_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_D32_SFLOAT_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGB_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGB_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGBA_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGBA_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC2_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC2_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC3_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC3_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC4_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC4_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC5_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC6H_UFLOAT_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC6H_SFLOAT_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC7_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC7_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11G11_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11G11_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_4x4_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_4x4_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x4_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x4_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x6_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x6_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x6_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x6_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x6_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x6_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x10_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x10_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x10_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x10_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x12_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x12_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8B8G8R8_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8G8_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R10X6_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R10X6G10X6_UNORM_2PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R12X4_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R12X4G12X4_UNORM_2PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16B16G16R16_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B16G16R16G16_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_MAX_ENUM)
		default: {
			return String("Swapchain format ") + String::num_int64(int64_t(p_swapchain_format));
		} break;
	}
}
