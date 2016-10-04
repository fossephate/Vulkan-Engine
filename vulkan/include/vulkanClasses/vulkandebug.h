#pragma once

#include <math.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <iostream>

#ifdef _WIN32
	#include <windows.h>
	#include <fcntl.h>
	#include <io.h>
#endif

#include <vulkan/vulkan.hpp>

#ifdef __ANDROID__
	#include "vulkanAndroid.h"
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vkDebug
{
	// Default validation layers
	extern int validationLayerCount;
	extern const char *validationLayerNames[];

	// Default debug callback
	vk::Bool32 messageCallback(
		vk::DebugReportFlagsEXT flags,
		vk::DebugReportObjectTypeEXT objType,
		uint64_t srcObject,
		size_t location,
		int32_t msgCode,
		const char* pLayerPrefix,
		const char* pMsg,
		void* pUserData);

	// Load debug function pointers and set debug callback
	// if callBack is NULL, default message callback will be used
	void setupDebugging(
		vk::Instance instance, 
		vk::DebugReportFlagsEXT flags, 
		vk::DebugReportCallbackEXT callBack);
	// Clear debug callback
	void freeDebugCallback(vk::Instance instance);

	// Setup and functions for the VK_EXT_debug_marker_extension
	// Extension spec can be found at https://github.com/KhronosGroup/Vulkan-Docs/blob/1.0-VK_EXT_debug_marker/doc/specs/vulkan/appendices/VK_EXT_debug_marker.txt
	// Note that the extension will only be present if run from an offline debugging application
	// The actual check for extension presence and enabling it on the device is done in the example base class
	// See vulkanApp::createInstance and vulkanApp::createDevice (base/vulkanApp.cpp)
	namespace DebugMarker
	{
		// Set to true if function pointer for the debug marker are available
		extern bool active;

		// Get function pointers for the debug report extensions from the device
		void setup(vk::Device device);

		// Sets the debug name of an object
		// All Objects in Vulkan are represented by their 64-bit handles which are passed into this function
		// along with the object type
		void setObjectName(vk::Device device, uint64_t object, vk::DebugReportObjectTypeEXT objectType, const char *name);

		// Set the tag for an object
		void setObjectTag(vk::Device device, uint64_t object, vk::DebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);

		// Start a new debug marker region
		void beginRegion(vk::CommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color);

		// Insert a new debug marker into the command buffer
		void insert(vk::CommandBuffer cmdbuffer, std::string markerName, glm::vec4 color);

		// End the current debug marker region
		void endRegion(vk::CommandBuffer cmdBuffer);

		// Object specific naming functions
		void setCommandBufferName(vk::Device device, vk::CommandBuffer cmdBuffer, const char * name);
		void setQueueName(vk::Device device, vk::Queue queue, const char * name);
		void setImageName(vk::Device device, vk:: Image image, const char * name);
		void setSamplerName(vk::Device device, vk::Sampler sampler, const char * name);
		void setBufferName(vk::Device device, vk::Buffer buffer, const char * name);
		void setDeviceMemoryName(vk::Device device, vk::DeviceMemory memory, const char * name);
		void setShaderModuleName(vk::Device device, vk::ShaderModule shaderModule, const char * name);
		void setPipelineName(vk::Device device, vk::Pipeline pipeline, const char * name);
		void setPipelineLayoutName(vk::Device device, vk::PipelineLayout pipelineLayout, const char * name);
		void setRenderPassName(vk::Device device, vk::RenderPass renderPass, const char * name);
		void setFramebufferName(vk::Device device, vk::Framebuffer framebuffer, const char * name);
		void setDescriptorSetLayoutName(vk::Device device, vk::DescriptorSetLayout descriptorSetLayout, const char * name);
		void setDescriptorSetName(vk::Device device, vk::DescriptorSet descriptorSet, const char * name);
		void setSemaphoreName(vk::Device device, vk::Semaphore semaphore, const char * name);
		void setFenceName(vk::Device device, vk::Fence fence, const char * name);
		void setEventName(vk::Device device, vk::Event _event, const char * name);
	};

}
