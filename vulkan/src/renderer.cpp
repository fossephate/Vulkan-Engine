#include "renderer.h"
#include "shared.h"


#include <cstdlib>
#include <assert.h>
#include <vector>

#include <iostream>
#include <sstream>


#ifdef _WIN32
#include <windows.h>
#endif // _WIN32


// _instance is a private var

renderer::renderer() {
	_setupDebug();
	_initInstance();
	_initDebug();
	_initDevice();
}

renderer::~renderer() {
	_deInitDevice();
	_deInitDebug();
	_deInitInstance();
}

// private methods that create and destroy an instance
void renderer::_initInstance() {

	VkApplicationInfo application_info {};// default init of class
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.apiVersion = VK_MAKE_VERSION(1, 0, 2);// 13? 2? ???
	application_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);// pretty sure this isn't important
	//application_info.engineVersion = VK_MAKE_VERSION(1, 0, 13);
	application_info.pApplicationName = "Vulkan test";

	VkInstanceCreateInfo instance_create_info {};// default init
	instance_create_info.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo			= &application_info;// sets pointer to application info?
	instance_create_info.enabledLayerCount			= _instance_layers.size();
	instance_create_info.ppEnabledLayerNames		= _instance_layers.data();
	instance_create_info.enabledExtensionCount		= _instance_extensions.size();
	instance_create_info.ppEnabledExtensionNames	= _instance_extensions.data();
	instance_create_info.pNext						= &debug_callback_create_info;// new

	

	errorCheck(vkCreateInstance(&instance_create_info, nullptr, &_instance));// takes type vkresult and logs errors if there are any
	
	/*auto err = vkCreateInstance( &instance_create_info, nullptr, &_instance );

	if (VK_SUCCESS != err) {
		assert(0 && "Vulkan ERROR: Create instance failed.");
		std::exit(-1);
	}*/

}

void renderer::_deInitInstance()
{
	// destroys the instance
	vkDestroyInstance(_instance, nullptr);
	// extra destroys the instance? lol
	_instance = nullptr;
}

void renderer::_initDevice()
{

	{
		uint32_t gpu_count = 0;
		vkEnumeratePhysicalDevices(_instance, &gpu_count, nullptr);
		std::vector<VkPhysicalDevice> gpu_list( gpu_count );
		vkEnumeratePhysicalDevices( _instance, &gpu_count, gpu_list.data() );
		_gpu = gpu_list[0];
		vkGetPhysicalDeviceProperties(_gpu, &_gpu_properties);
	}


	{
		uint32_t family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, nullptr);
		std::vector<VkQueueFamilyProperties> family_property_list(family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, family_property_list.data());

		bool found = false;
		for (uint32_t i = 0; i < family_count; ++i) {
			if (family_property_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				found = true;
				_graphics_family_index = i;

			}
		}

		if (!found) {
			assert(0 && "Vulkan ERROR: Queue family supporting graphics not found.");
			std::exit( -1 );
		}

	}

	{
		// debugging instance layers
		uint32_t layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> layer_propery_list(layer_count);// init with size of layer_count
		vkEnumerateInstanceLayerProperties(&layer_count, layer_propery_list.data());// list of all layers installed on system
		
		std::cout << "Instance Layers: \n";
		for (auto &i : layer_propery_list) {
			std::cout << " " << i.layerName << "\t\t" << i.description << std::endl;
		}
		std::cout << std::endl;
	}

	/*// device layers deprecated:
	{
		// debugging device layers?
		uint32_t layer_count = 0;
		vkEnumerateDeviceLayerProperties(_gpu, &layer_count, nullptr);
		std::vector<VkLayerProperties> layer_propery_list(layer_count);// init with size of layer_count
		vkEnumerateDeviceLayerProperties(_gpu, &layer_count, layer_propery_list.data());// list of all layers installed on system

		std::cout << "Device Layers: \n";
		for (auto &i : layer_propery_list) {
			std::cout << " " << i.layerName << "\t\t" << i.description << std::endl;
		}
		std::cout << std::endl;
	}*/

	float queue_priorities[]{ 1.0f };
	VkDeviceQueueCreateInfo device_queue_create_info{};
	device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_info.queueFamilyIndex = _graphics_family_index;
	device_queue_create_info.queueCount = 1;
	device_queue_create_info.pQueuePriorities = queue_priorities;


	VkDeviceCreateInfo device_create_info{};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &device_queue_create_info;

	/*// device layers deprecated
	device_create_info.enabledLayerCount = _device_layers.size();
	device_create_info.ppEnabledLayerNames = _device_layers.data();*/

	device_create_info.enabledExtensionCount = _device_extensions.size();
	device_create_info.ppEnabledExtensionNames = _device_extensions.data();



	errorCheck(vkCreateDevice(_gpu, &device_create_info, nullptr, &_device));// takes type vkresult and logs errors if there are any

	/*auto err = vkCreateDevice(_gpu, &device_create_info, nullptr, &_device);
	if (VK_SUCCESS != err) {
		assert(0 && "Vulkan ERROR: Device creation failed.");
		std::exit( -1 );
	}*/
}

void renderer::_deInitDevice()
{
	vkDestroyDevice(_device, nullptr);
	_device = nullptr;
}


// long function definition
VKAPI_ATTR VkBool32 VKAPI_CALL
vulkanDebugCallback(
	VkDebugReportFlagsEXT		flags,
	VkDebugReportObjectTypeEXT	obj_type,
	uint64_t					src_obj,
	size_t						location,
	int32_t						msg_code,
	const char *				layer_prefix,
	const char *				msg,
	void *						user_data
	)
{
	std::ostringstream stream;

	stream << "VKDBG: ";
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		stream << "INFO: ";
	}
	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		stream << "WARNING: ";
	}
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		stream << "PERFORMANCE: ";
	}
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		stream << "ERROR: ";
	}
	if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		stream << "DEBUG: ";
	}
	if (flags & VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT) {
		stream << "????: ";
	}

	stream << "@[" << layer_prefix << "]: ";
	stream << msg << std::endl;

	std::cout << stream.str();


	// code is only (passed to compiler) if on windows
#ifdef _WIN32
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		MessageBox(NULL, stream.str().c_str(), "Vulkan Error!", 0);
	}
#endif // _WIN32


	return false;// stops from reaching vulkan core

}


void renderer::_setupDebug()
{

	debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debug_callback_create_info.pfnCallback = vulkanDebugCallback;

	debug_callback_create_info.flags =
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT |
		0;

	//_instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");// not installed (yet)

	/*// not installed (yet)
	_instance_layers.push_back("VK_LAYER_LUNARG_threading");
	_instance_layers.push_back("VK_LAYER_LUNARG_draw_state");
	_instance_layers.push_back("VK_LAYER_LUNARG_image");
	_instance_layers.push_back("VK_LAYER_LUNARG_mem_tracker");
	_instance_layers.push_back("VK_LAYER_LUNARG_object_tracker");
	_instance_layers.push_back("VK_LAYER_LUNARG_param_checker");*/

	_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);



	//_device_layers.push_back("VK_LAYER_LUNARG_standard_validation");// device layers deprecated

	/*// device layers deprecated
	_device_layers.push_back("VK_LAYER_LUNARG_threading");
	_device_layers.push_back("VK_LAYER_LUNARG_draw_state");
	_device_layers.push_back("VK_LAYER_LUNARG_image");
	_device_layers.push_back("VK_LAYER_LUNARG_mem_tracker");
	_device_layers.push_back("VK_LAYER_LUNARG_object_tracker");
	_device_layers.push_back("VK_LAYER_LUNARG_param_checker");*/


}

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;

void renderer::_initDebug()
{
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");// casted
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT");// casted

	// in case of death
	if (nullptr == fvkCreateDebugReportCallbackEXT || nullptr == fvkDestroyDebugReportCallbackEXT) {
		// die
		assert(0 && "Vulkan ERROR: Can't fetch debug function pointers.");
		std::exit(-1);
	}

	//VkDebugReportCallbackCreateInfoEXT debug_callback_create_info{};// default init


	// store debug info that we just got
	fvkCreateDebugReportCallbackEXT(_instance, &debug_callback_create_info, nullptr, &_debug_report);// _debug_report is a pointer

	//vkCreateDebugReportCallbackEXT(_instance, nullptr, nullptr, nullptr);
}

void renderer::_deInitDebug()
{
	// destroy debug instance
	fvkDestroyDebugReportCallbackEXT(_instance, _debug_report, nullptr);
	// extra destroy
	_debug_report = nullptr;
}
