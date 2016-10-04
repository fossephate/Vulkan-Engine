#include "vulkanDebug.h"

namespace vkDebug
{
	int validationLayerCount = 1;
	const char *validationLayerNames[] = 
	{
		// This is a meta layer that enables all of the standard
		// validation layers in the correct order :
		// threading, parameter_validation, device_limits, object_tracker, image, core_validation, swapchain, and unique_objects
		"VK_LAYER_LUNARG_standard_validation"
	};

	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = VK_NULL_HANDLE;
	PFN_vkDebugReportMessageEXT dbgBreakCallback = VK_NULL_HANDLE;

	//vk::DebugReportCallbackCreateInfoEXT a;

	vk::DebugReportCallbackEXT msgCallback;

	vk::Bool32 messageCallback(
		vk::DebugReportFlagsEXT flags,
		vk::DebugReportObjectTypeEXT objType,
		uint64_t srcObject,
		size_t location,
		int32_t msgCode,
		const char* pLayerPrefix,
		const char* pMsg,
		void* pUserData)
	{
		// Select prefix depending on flags passed to the callback
		// Note that multiple flags may be set for a single validation message
		std::string prefix("");

		// Error that may result in undefined behaviour
		if (flags & vk::DebugReportFlagBitsEXT::eError) {
			prefix += "ERROR:";
		};
		// Warnings may hint at unexpected / non-spec API usage
		if (flags & vk::DebugReportFlagBitsEXT::eWarning) {
			prefix += "WARNING:";
		};
		// May indicate sub-optimal usage of the API
		if (flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning)
		{
			prefix += "PERFORMANCE:";
		};
		// Informal messages that may become handy during debugging
		if (flags & vk::DebugReportFlagBitsEXT::eInformation)
		{
			prefix += "INFO:";
		}
		// Diagnostic info from the Vulkan loader and layers
		// Usually not helpful in terms of API usage, but may help to debug layer and loader problems 
		if (flags & vk::DebugReportFlagBitsEXT::eDebug)
		{
			prefix += "DEBUG:";
		}

		// Display message to default output (console if activated)
		std::cout << prefix << " [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << "\n";

		fflush(stdout);

		// The return value of this callback controls wether the Vulkan call that caused
		// the validation message will be aborted or not
		// We return VK_FALSE as we DON'T want Vulkan calls that cause a validation message 
		// (and return a VkResult) to abort
		// If you instead want to have calls abort, pass in VK_TRUE and the function will 
		// return VK_ERROR_VALIDATION_FAILED_EXT 
		return VK_FALSE;
	}

	void setupDebugging(vk::Instance instance, vk::DebugReportFlagsEXT flags, vk::DebugReportCallbackEXT callBack)
	{
		
		//CreateDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
		CreateDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(instance.getProcAddr("vkCreateDebugReportCallbackEXT"));
		
		//DestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
		DestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(instance.getProcAddr("vkDestroyDebugReportCallbackEXT"));

		//dbgBreakCallback = reinterpret_cast<PFN_vkDebugReportMessageEXT>(vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT"));
		dbgBreakCallback = reinterpret_cast<PFN_vkDebugReportMessageEXT>(instance.getProcAddr("vkDebugReportMessageEXT"));


		vk::DebugReportCallbackCreateInfoEXT dbgCreateInfo;
		dbgCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)messageCallback;
		dbgCreateInfo.flags = flags;

		vk::Result err = CreateDebugReportCallback(
			instance,
			&dbgCreateInfo,
			nullptr,
			(callBack != VK_NULL_HANDLE) ? &callBack : &msgCallback);
		assert(!err);
	}
	
	void freeDebugCallback(vk::Instance instance)
	{
		if (msgCallback != VK_NULL_HANDLE)
		{
			//DestroyDebugReportCallback(instance, msgCallback, nullptr);
			instance.destroyDebugReportCallbackEXT(msgCallback, nullptr);
		}
	}

	namespace DebugMarker
	{
		bool active = false;

		PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag = VK_NULL_HANDLE;
		PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = VK_NULL_HANDLE;
		PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin = VK_NULL_HANDLE;
		PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd = VK_NULL_HANDLE;
		PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert = VK_NULL_HANDLE;

		void setup(vk::Device device)
		{
			//pfnDebugMarkerSetObjectTag = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT"));
			pfnDebugMarkerSetObjectTag = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(device.getProcAddr("vkDebugMarkerSetObjectTagEXT"));

			//pfnDebugMarkerSetObjectName = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT"));
			pfnDebugMarkerSetObjectName = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(device.getProcAddr("vkDebugMarkerSetObjectNameEXT"));

			//pfnCmdDebugMarkerBegin = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT"));
			pfnCmdDebugMarkerBegin = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(device.getProcAddr("vkCmdDebugMarkerBeginEXT"));

			//pfnCmdDebugMarkerEnd = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT"));
			pfnCmdDebugMarkerEnd = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(device.getProcAddr("vkCmdDebugMarkerEndEXT"));

			//pfnCmdDebugMarkerInsert = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT"));
			pfnCmdDebugMarkerInsert = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(device.getProcAddr("vkCmdDebugMarkerInsertEXT"));

			// Set flag if at least one function pointer is present
			active = (pfnDebugMarkerSetObjectName != VK_NULL_HANDLE);

		}

		void setObjectName(vk::Device device, uint64_t object, vk::DebugReportObjectTypeEXT objectType, const char *name)
		{
			// Check for valid function pointer (may not be present if not running in a debugging application)
			if (pfnDebugMarkerSetObjectName)
			{
				vk::DebugMarkerObjectNameInfoEXT nameInfo;
				nameInfo.objectType = objectType;
				nameInfo.object = object;
				nameInfo.pObjectName = name;
				//pfnDebugMarkerSetObjectName(device, &nameInfo);
				device.debugMarkerSetObjectNameEXT(&nameInfo);
			}
		}

		void setObjectTag(vk::Device device, uint64_t object, vk::DebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag)
		{
			// Check for valid function pointer (may not be present if not running in a debugging application)
			if (pfnDebugMarkerSetObjectTag)
			{
				vk::DebugMarkerObjectTagInfoEXT tagInfo;
				tagInfo.objectType = objectType;
				tagInfo.object = object;
				tagInfo.tagName = name;
				tagInfo.tagSize = tagSize;
				tagInfo.pTag = tag;
				//pfnDebugMarkerSetObjectTag(device, &tagInfo);
				device.debugMarkerSetObjectTagEXT(&tagInfo);
			}
		}

		void beginRegion(vk::CommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color)
		{
			// Check for valid function pointer (may not be present if not running in a debugging application)
			if (pfnCmdDebugMarkerBegin)
			{
				vk::DebugMarkerMarkerInfoEXT markerInfo;
				memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
				markerInfo.pMarkerName = pMarkerName;
				//pfnCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
				cmdbuffer.debugMarkerBeginEXT(&markerInfo);
			}
		}

		void insert(vk::CommandBuffer cmdbuffer, std::string markerName, glm::vec4 color)
		{
			// Check for valid function pointer (may not be present if not running in a debugging application)
			if (pfnCmdDebugMarkerInsert)
			{
				vk::DebugMarkerMarkerInfoEXT markerInfo;
				memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
				markerInfo.pMarkerName = markerName.c_str();
				//pfnCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
				cmdbuffer.debugMarkerInsertEXT(&markerInfo);
			}
		}

		void endRegion(vk::CommandBuffer cmdBuffer)
		{
			// Check for valid function (may not be present if not runnin in a debugging application)
			if (pfnCmdDebugMarkerEnd)
			{
				//pfnCmdDebugMarkerEnd(cmdBuffer);
				cmdBuffer.debugMarkerEndEXT();
			}
		}

		void setCommandBufferName(vk::Device device, vk::CommandBuffer cmdBuffer, const char * name)
		{
			setObjectName(device, (uint64_t)cmdBuffer, vk::DebugReportObjectTypeEXT::eCommandBuffer, name);

		}

		void setQueueName(vk::Device device, vk::Queue queue, const char * name)
		{
			setObjectName(device, (uint64_t)queue, vk::DebugReportObjectTypeEXT::eQueue, name);
		}

		void setImageName(vk::Device device, vk::Image image, const char * name)
		{
			setObjectName(device, (uint64_t)image, vk::DebugReportObjectTypeEXT::eImage, name);
		}

		void setSamplerName(vk::Device device, vk::Sampler sampler, const char * name)
		{
			setObjectName(device, (uint64_t)sampler, vk::DebugReportObjectTypeEXT::eSampler, name);
		}

		void setBufferName(vk::Device device, vk::Buffer buffer, const char * name)
		{
			setObjectName(device, (uint64_t)buffer, vk::DebugReportObjectTypeEXT::eBuffer, name);
		}

		void setDeviceMemoryName(vk::Device device, vk::DeviceMemory memory, const char * name)
		{
			setObjectName(device, (uint64_t)memory, vk::DebugReportObjectTypeEXT::eDeviceMemory, name);
		}

		void setShaderModuleName(vk::Device device, vk::ShaderModule shaderModule, const char * name)
		{
			setObjectName(device, (uint64_t)shaderModule, vk::DebugReportObjectTypeEXT::eShaderModule, name);
		}

		void setPipelineName(vk::Device device, vk::Pipeline pipeline, const char * name)
		{
			setObjectName(device, (uint64_t)pipeline, vk::DebugReportObjectTypeEXT::ePipeline, name);
		}

		void setPipelineLayoutName(vk::Device device, vk::PipelineLayout pipelineLayout, const char * name)
		{
			setObjectName(device, (uint64_t)pipelineLayout, vk::DebugReportObjectTypeEXT::ePipelineLayout, name);
		}

		void setRenderPassName(vk::Device device, vk::RenderPass renderPass, const char * name)
		{
			setObjectName(device, (uint64_t)renderPass, vk::DebugReportObjectTypeEXT::eRenderPass, name);
		}

		void setFramebufferName(vk::Device device, vk::Framebuffer framebuffer, const char * name)
		{
			setObjectName(device, (uint64_t)framebuffer, vk::DebugReportObjectTypeEXT::eFramebuffer, name);
		}

		void setDescriptorSetLayoutName(vk::Device device, vk::DescriptorSetLayout descriptorSetLayout, const char * name)
		{
			setObjectName(device, (uint64_t)descriptorSetLayout, vk::DebugReportObjectTypeEXT::eDescriptorSetLayout, name);
		}

		void setDescriptorSetName(vk::Device device, vk::DescriptorSet descriptorSet, const char * name)
		{
			setObjectName(device, (uint64_t)descriptorSet, vk::DebugReportObjectTypeEXT::eDescriptorSet, name);
		}

		void setSemaphoreName(vk::Device device, vk::Semaphore semaphore, const char * name)
		{
			setObjectName(device, (uint64_t)semaphore, vk::DebugReportObjectTypeEXT::eSemaphore, name);
		}

		void setFenceName(vk::Device device, vk::Fence fence, const char * name)
		{
			setObjectName(device, (uint64_t)fence, vk::DebugReportObjectTypeEXT::eFence, name);
		}

		void setEventName(vk::Device device, vk::Event _event, const char * name)
		{
			setObjectName(device, (uint64_t)_event, vk::DebugReportObjectTypeEXT::eEvent, name);
		}
	};

}

