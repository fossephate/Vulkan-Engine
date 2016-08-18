#include "window.h"
#include "renderer.h"

#include <assert.h>

window::window(renderer * r, uint32_t _size_x, uint32_t _size_y, std::string name)
{
	_renderer = r;
	_surface_size_x = _size_x;
	_surface_size_y = _size_y;
	_window_name = name;

	_initOSWindow();
	_initSurface();
}

window::~window()
{
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
