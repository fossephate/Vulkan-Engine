#pragma once

#include "platform.h"
#include <vector>
#include <string>

class window;


class renderer {
	public:
		renderer();
		~renderer();

		window * createWindow(uint32_t _size_x, uint32_t _size_y, std::string window_name);

		bool	run();

		const VkInstance						getVulkanInstance() const;
		const VkPhysicalDevice					getVulkanPhysicalDevice() const;
		const VkDevice							getVulkanDevice() const;
		const VkQueue							getVulkanQueue() const;
		const uint32_t							getVulkanGraphicsQueueFamilyIndex() const;
		const VkPhysicalDeviceProperties	&	getVulkanPhysicalDeviceProperties() const;


	private:
		void _setupLayersAndExtensions();
		void _initInstance();
		void _deInitInstance();

		void _initDevice();
		void _deInitDevice();

		void _setupDebug();
		void _initDebug();
		void _deInitDebug();

		VkInstance						_instance					= VK_NULL_HANDLE;// init private var as nullptr
		VkPhysicalDevice				_gpu						= VK_NULL_HANDLE;// init private var as nullptr
		VkDevice						_device						= VK_NULL_HANDLE;// init private var as nullptr
		VkPhysicalDeviceProperties		_gpu_properties				= {};// init private var as {}
		// queue to send to gpu
		VkQueue							_queue						= VK_NULL_HANDLE;

		uint32_t						_graphics_family_index		= 0;


		window *_window = nullptr;



		std::vector<const char*> _instance_layers;
		std::vector<const char*> _instance_extensions;
		//std::vector<const char*> _device_layers;// deprecated
		std::vector<const char*> _device_extensions;


		VkDebugReportCallbackEXT _debug_report = VK_NULL_HANDLE;
		VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {};
};