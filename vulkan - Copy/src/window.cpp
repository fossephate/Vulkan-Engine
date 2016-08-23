#include "window.h"
#include "renderer.h"
#include "shared.h"

#include <assert.h>

window::window(renderer * r, uint32_t _size_x, uint32_t _size_y, std::string name)
{
	_renderer = r;
	_surface_size_x = _size_x;
	_surface_size_y = _size_y;
	_window_name = name;

	_initOSWindow();
	_initSurface();
	_initSwapchain();
	_initSwapchainImages();
}

window::~window()
{
	_deInitSwapchainImages();
	_deInitSwapchain();
	_deInitSurface();
	_deInitOSWindow();
}

void window::close()
{
	_continue_running = false;
}

bool window::update()
{
	_updateOSWindow();
	return _continue_running;
}


void window::_initSurface()
{
	_initOSSurface();

	auto gpu = _renderer->getVulkanPhysicalDevice();

	VkBool32 WSI_supported = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(gpu, _renderer->getVulkanGraphicsQueueFamilyIndex(), _surface, &WSI_supported);
	if (!WSI_supported) {
		assert(0 && "WSI not supported.");
		std::exit(-1);
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, _surface, &_surface_capabilities);
	if (_surface_capabilities.currentExtent.width < UINT32_MAX) {
		_surface_size_x = _surface_capabilities.currentExtent.width;
		_surface_size_y = _surface_capabilities.currentExtent.height;
	}

	{
		uint32_t format_count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, _surface, &format_count, nullptr);
		if (format_count == 0) {
			assert(0 && "Surface formats missing.");
			std::exit(-1);
		}

		std::vector<VkSurfaceFormatKHR> formats(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, _surface, &format_count, formats.data());
		
		// if the first format is undefined then we can choose any one that we want
		if (formats[0].format == VK_FORMAT_UNDEFINED) {
			_surface_format.format		= VK_FORMAT_B8G8R8A8_UNORM;
			_surface_format.colorSpace	= VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		// otherwise, just pick the first one
		} else {
			_surface_format = formats[0];
		}

	}
}

void window::_deInitSurface()
{
	vkDestroySurfaceKHR(_renderer->getVulkanInstance(), _surface, nullptr);
}

void window::_initSwapchain()
{
	/*
	if (_swapchain_image_count > _surface_capabilities.maxImageCount) {
		_swapchain_image_count = _surface_capabilities.maxImageCount;
	}
	if (_swapchain_image_count < _surface_capabilities.minImageCount+1) {
		_swapchain_image_count = _surface_capabilities.minImageCount+1;
	}*/

	// The code above will work just fine in our tutorials and likely on every possible implementation of vulkan as well
	// so this change isn't that important. Just to be absolutely sure we don't go over or below the given limits we should check this a
	// little bit different though. maxImageCount can actually be zero in which case the amount of swapchain images do not have an
	// upper limit other than available memory. It's also possible that the swapchain image amount is locked to a certain
	// value on certain systems. The code below takes into consideration both of these possibilities.
	if (_swapchain_image_count < _surface_capabilities.minImageCount + 1) {
		_swapchain_image_count = _surface_capabilities.minImageCount + 1;
	}

	if (_surface_capabilities.maxImageCount > 0 && _swapchain_image_count > _surface_capabilities.maxImageCount) {
		_swapchain_image_count = _surface_capabilities.maxImageCount;
	}

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;// if all else fails, this present mode is guaranteed
	{
		uint32_t present_mode_count = 0;// how many present modes this device supports
		errorCheck(
		vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->getVulkanPhysicalDevice(), _surface, &present_mode_count, nullptr)
		);

		std::vector<VkPresentModeKHR> present_mode_list(present_mode_count);

		errorCheck(
		vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->getVulkanPhysicalDevice(), _surface, &present_mode_count, present_mode_list.data())
		);

		// enable mailbox present mode if it's available
		for (auto m : present_mode_list) {
			if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
				present_mode = m;
			}
		}
	}

	VkSwapchainCreateInfoKHR swapchain_create_info{};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

	//swapchain_create_info.pNext					= ;
	//swapchain_create_info.flags					= ;
	swapchain_create_info.surface					= _surface;
	swapchain_create_info.minImageCount				= _swapchain_image_count;// double buffering by default
	swapchain_create_info.imageFormat				= _surface_format.format;
	swapchain_create_info.imageColorSpace			= _surface_format.colorSpace;

	//swapchain_create_info.imageExtent				=;// extent = size
	swapchain_create_info.imageExtent.width			= _surface_size_x;
	swapchain_create_info.imageExtent.height		= _surface_size_y;

	swapchain_create_info.imageArrayLayers			= 1;// higher for stereoscopic, don't really need
	swapchain_create_info.imageUsage				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode			= VK_SHARING_MODE_EXCLUSIVE;// or VK_SHARING_MODE_CONCURRENT
	swapchain_create_info.queueFamilyIndexCount		= 0;// ignored if above is exclusive
	swapchain_create_info.pQueueFamilyIndices		= nullptr;// ignored if above is exclusive
	swapchain_create_info.preTransform				= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;// describes how the surface should be rotated, or mirrored
	swapchain_create_info.compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.presentMode				= present_mode;
	swapchain_create_info.clipped					= VK_TRUE;// false forces it to render everything every time
	swapchain_create_info.oldSwapchain				= VK_NULL_HANDLE;// useful for resizing window


	// check these functions for errors
	errorCheck(
	vkCreateSwapchainKHR(_renderer->getVulkanDevice(), &swapchain_create_info, nullptr, &_swapchain)
	);

	errorCheck(
	vkGetSwapchainImagesKHR(_renderer->getVulkanDevice(), _swapchain, &_swapchain_image_count, nullptr)
	);


}

void window::_deInitSwapchain()
{
	vkDestroySwapchainKHR(_renderer->getVulkanDevice(), _swapchain, nullptr);
}

void window::_initSwapchainImages()
{
	_swapchain_images.resize(_swapchain_image_count);
	_swapchain_image_views.resize(_swapchain_image_count);

	errorCheck(
		vkGetSwapchainImagesKHR(_renderer->getVulkanDevice(), _swapchain, &_swapchain_image_count, _swapchain_images.data())
	);

	for (uint32_t i = 0; i < _swapchain_image_count; ++i) {
		VkImageViewCreateInfo image_view_create_info{};
		image_view_create_info.sType								= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		//image_view_create_info.pNext								= ;
		//image_view_create_info.flags								= ;
		image_view_create_info.image								= _swapchain_images[i];
		image_view_create_info.viewType								= VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format								= _surface_format.format;
		image_view_create_info.components.r							= VK_COMPONENT_SWIZZLE_IDENTITY;//VK_COMPONENT_SWIZZLE_R
		image_view_create_info.components.g							= VK_COMPONENT_SWIZZLE_IDENTITY;//VK_COMPONENT_SWIZZLE_G
		image_view_create_info.components.b							= VK_COMPONENT_SWIZZLE_IDENTITY;//VK_COMPONENT_SWIZZLE_B
		image_view_create_info.components.a							= VK_COMPONENT_SWIZZLE_IDENTITY;//VK_COMPONENT_SWIZZLE_A
		image_view_create_info.subresourceRange.aspectMask			= VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseMipLevel		= 0;
		image_view_create_info.subresourceRange.levelCount			= 1;
		image_view_create_info.subresourceRange.baseArrayLayer		= 0;
		image_view_create_info.subresourceRange.layerCount			= 1;


		errorCheck(
			vkCreateImageView(_renderer->getVulkanDevice(), &image_view_create_info, nullptr, &_swapchain_image_views[i])
		);
	}

}

void window::_deInitSwapchainImages()
{
	for (auto view : _swapchain_image_views) {
		vkDestroyImageView(_renderer->getVulkanDevice(), view, nullptr);
	}
}
