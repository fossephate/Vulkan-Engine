/*
* Assorted commonly used Vulkan helper functions
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

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
#include <stdexcept>

#if defined(_WIN32)
	#include <windows.h>
	#include <fcntl.h>
	#include <io.h>
#endif

#if defined(__ANDROID__)
	#include "vulkanAndroid.h"
	#include <android/asset_manager.h>
#endif

#include <vulkan/vulkan.hpp>

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

// Macro to check and display Vulkan return results
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << vkTools::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}																										\

// Macro to check and display Vulkan return results
#define VK_CHECK_RESULT2(f)																				\
{																										\
	vk::Result res = (f);																					\
	if (res != vk::Result::eSuccess)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << vkTools::errorString2(res) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
		assert(res == vk::Result::eSuccess);																		\
	}																									\
}																										\

namespace vkTools
{
	//// Check if extension is globally available
	//VkBool32 checkGlobalExtensionPresent(const char* extensionName);

	//// Check if extension is present on the given device
	//VkBool32 checkDeviceExtensionPresent(VkPhysicalDevice physicalDevice, const char* extensionName);
	//// Return string representation of a vulkan error string
	//std::string errorString(VkResult errorCode);

	//// Selected a suitable supported depth format starting with 32 bit down to 16 bit
	//// Returns false if none of the depth formats in the list is supported by the device
	//VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat);

	//// Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
	//void setImageLayout(
	//	VkCommandBuffer cmdbuffer,
	//	VkImage image,
	//	VkImageAspectFlags aspectMask,
	//	VkImageLayout oldImageLayout,
	//	VkImageLayout newImageLayout,
	//	VkImageSubresourceRange subresourceRange);
	//// Uses a fixed sub resource layout with first mip level and layer
	//void setImageLayout(
	//	VkCommandBuffer cmdbuffer, 
	//	VkImage image, 
	//	VkImageAspectFlags aspectMask, 
	//	VkImageLayout oldImageLayout, 
	//	VkImageLayout newImageLayout);




	// C++ WRAPPER

	// Check if extension is globally available
	vk::Bool32 checkGlobalExtensionPresent2(const char* extensionName);

	// Check if extension is present on the given device
	vk::Bool32 checkDeviceExtensionPresent2(vk::PhysicalDevice physicalDevice, const char* extensionName);
	// Return string representation of a vulkan error string
	std::string errorString2(vk::Result errorCode);

	// Selected a suitable supported depth format starting with 32 bit down to 16 bit
	// Returns false if none of the depth formats in the list is supported by the device
	vk::Bool32 getSupportedDepthFormat(vk::PhysicalDevice physicalDevice, vk::Format *depthFormat);

	vk::AccessFlags accessFlagsForLayout(vk::ImageLayout layout);

	// Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
	void setImageLayout2(
		vk::CommandBuffer cmdbuffer,
		vk::Image image,
		vk::ImageAspectFlags aspectMask,
		vk::ImageLayout oldImageLayout,
		vk::ImageLayout newImageLayout,
		vk::ImageSubresourceRange subresourceRange);
	// Uses a fixed sub resource layout with first mip level and layer
	void setImageLayout2(
		vk::CommandBuffer cmdbuffer,
		vk::Image image,
		vk::ImageAspectFlags aspectMask,
		vk::ImageLayout oldImageLayout,
		vk::ImageLayout newImageLayout);

	// END C++ WRAPPER




	// Display error message and exit on fatal error
	void exitFatal(std::string message, std::string caption);
	// Load a text file (e.g. GLGL shader) into a std::string
	std::string readTextFile(const char *fileName);
	// Load a binary file into a buffer (e.g. SPIR-V)
	std::vector<uint8_t> readBinaryFile(const std::string& filename);

	// Load a SPIR-V shader
	#if defined(__ANDROID__)
		VkShaderModule loadShader(AAssetManager* assetManager, const char *fileName, VkDevice device, VkShaderStageFlagBits stage);
		vk::ShaderModule loadShader2(AAssetManager* assetManager, const char *fileName, vk::Device device, vk::ShaderStageFlagBits stage);
	#else
		//VkShaderModule loadShader(const char *fileName, VkDevice device, VkShaderStageFlagBits stage);
		vk::ShaderModule loadShader2(const char *fileName, vk::Device device, vk::ShaderStageFlagBits stage);
	#endif

	// Load a GLSL shader
	// Note : Only for testing psdl2rposes, support for directly feeding GLSL shaders into Vulkan
	// may be dropped at some point	
	//VkShaderModule loadShaderGLSL(const char *fileName, VkDevice device, VkShaderStageFlagBits stage);
	vk::ShaderModule loadShaderGLSL2(const char *fileName, vk::Device device, vk::ShaderStageFlagBits stage);

	// Returns a pre-present image memory barrier
	// Transforms the image's layout from color attachment to present khr
	//VkImageMemoryBarrier prePresentBarrier(VkImage presentImage);
	vk::ImageMemoryBarrier prePresentBarrier2(vk::Image presentImage);// replaces ^

	// Returns a post-present image memory barrier
	// Transforms the image's layout back from present khr to color attachment
	//VkImageMemoryBarrier postPresentBarrier(VkImage presentImage);
	vk::ImageMemoryBarrier postPresentBarrier2(vk::Image presentImage);// replaces ^

	// Contains all vulkan objects
	// required for a uniform data object
	//struct UniformData 
	//{
	//	VkBuffer buffer;
	//	VkDeviceMemory memory;
	//	VkDescriptorBufferInfo descriptor;
	//	uint32_t allocSize;
	//	void* mapped = nullptr;
	//};

	struct UniformData2
	{
		vk::Buffer buffer;
		vk::DeviceMemory memory;
		vk::DescriptorBufferInfo descriptor;
		uint32_t allocSize;
		void* mapped = nullptr;
	};


	// Destroy (and free) Vulkan resources used by a uniform data structure
	//void destroyUniformData(VkDevice device, vkTools::UniformData *uniformData);
	void destroyUniformData2(vk::Device device, vkTools::UniformData2 *uniformData);























	// Contains often used vulkan object initializers
	// Save lot of VK_STRUCTURE_TYPE assignments
	// Some initializers are parameterized for convenience
	namespace initializers
	{
		//VkMemoryAllocateInfo memoryAllocateInfo();

		//VkCommandBufferAllocateInfo commandBufferAllocateInfo(
		//	VkCommandPool commandPool,
		//	VkCommandBufferLevel level,
		//	uint32_t bufferCount);

		//VkCommandPoolCreateInfo commandPoolCreateInfo();
		//VkCommandBufferBeginInfo commandBufferBeginInfo();
		//VkCommandBufferInheritanceInfo commandBufferInheritanceInfo();

		//VkRenderPassBeginInfo renderPassBeginInfo();
		//VkRenderPassCreateInfo renderPassCreateInfo();

		//VkImageMemoryBarrier imageMemoryBarrier();
		//VkBufferMemoryBarrier bufferMemoryBarrier();
		//VkMemoryBarrier memoryBarrier();

		//VkImageCreateInfo imageCreateInfo();
		//VkSamplerCreateInfo samplerCreateInfo();
		//VkImageViewCreateInfo imageViewCreateInfo();

		//VkFramebufferCreateInfo framebufferCreateInfo();

		//VkSemaphoreCreateInfo semaphoreCreateInfo();
		//VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = VK_FLAGS_NONE);
		//VkEventCreateInfo eventCreateInfo();

		//VkSubmitInfo submitInfo();

		//VkViewport viewport(
		//	float width, 
		//	float height, 
		//	float minDepth, 
		//	float maxDepth);

		//VkRect2D rect2D(
		//	int32_t width,
		//	int32_t height,
		//	int32_t offsetX,
		//	int32_t offsetY);

		//VkBufferCreateInfo bufferCreateInfo();

		//VkBufferCreateInfo bufferCreateInfo(
		//	VkBufferUsageFlags usage, 
		//	VkDeviceSize size);

		//VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
		//	uint32_t poolSizeCount,
		//	VkDescriptorPoolSize* pPoolSizes,
		//	uint32_t maxSets);

		//VkDescriptorPoolSize descriptorPoolSize(
		//	VkDescriptorType type,
		//	uint32_t descriptorCount);

		//VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
		//	VkDescriptorType type, 
		//	VkShaderStageFlags stageFlags, 
		//	uint32_t binding,
		//	uint32_t count = 1);

		//VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
		//	const VkDescriptorSetLayoutBinding* pBindings,
		//	uint32_t bindingCount);

		//VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
		//	const VkDescriptorSetLayout* pSetLayouts,
		//	uint32_t setLayoutCount	);

		//VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
		//	VkDescriptorPool descriptorPool,
		//	const VkDescriptorSetLayout* pSetLayouts,
		//	uint32_t descriptorSetCount);

		//VkDescriptorImageInfo descriptorImageInfo(
		//	VkSampler sampler,
		//	VkImageView imageView,
		//	VkImageLayout imageLayout);

		//VkWriteDescriptorSet writeDescriptorSet(
		//	VkDescriptorSet dstSet, 
		//	VkDescriptorType type, 
		//	uint32_t binding, 
		//	VkDescriptorBufferInfo* bufferInfo);

		//VkWriteDescriptorSet writeDescriptorSet(
		//	VkDescriptorSet dstSet, 
		//	VkDescriptorType type, 
		//	uint32_t binding, 
		//	VkDescriptorImageInfo* imageInfo);

		//VkVertexInputBindingDescription vertexInputBindingDescription(
		//	uint32_t binding, 
		//	uint32_t stride, 
		//	VkVertexInputRate inputRate);

		//VkVertexInputAttributeDescription vertexInputAttributeDescription(
		//	uint32_t binding,
		//	uint32_t location,
		//	VkFormat format,
		//	uint32_t offset);

		//VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo();

		//VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
		//	VkPrimitiveTopology topology,
		//	VkPipelineInputAssemblyStateCreateFlags flags,
		//	VkBool32 primitiveRestartEnable);

		//VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
		//	VkPolygonMode polygonMode,
		//	VkCullModeFlags cullMode,
		//	VkFrontFace frontFace,
		//	VkPipelineRasterizationStateCreateFlags flags);

		//VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
		//	VkColorComponentFlags colorWriteMask,
		//	VkBool32 blendEnable);

		//VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
		//	uint32_t attachmentCount,
		//	const VkPipelineColorBlendAttachmentState* pAttachments);

		//VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
		//	VkBool32 depthTestEnable,
		//	VkBool32 depthWriteEnable,
		//	VkCompareOp depthCompareOp);

		//VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
		//	uint32_t viewportCount,
		//	uint32_t scissorCount,
		//	VkPipelineViewportStateCreateFlags flags);

		//VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
		//	VkSampleCountFlagBits rasterizationSamples,
		//	VkPipelineMultisampleStateCreateFlags flags);

		//VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
		//	const VkDynamicState *pDynamicStates,
		//	uint32_t dynamicStateCount,
		//	VkPipelineDynamicStateCreateFlags flags);

		//VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(
		//	uint32_t patchControlPoints);

		//VkGraphicsPipelineCreateInfo pipelineCreateInfo(
		//	VkPipelineLayout layout,
		//	VkRenderPass renderPass,
		//	VkPipelineCreateFlags flags);

		//VkComputePipelineCreateInfo computePipelineCreateInfo(
		//	VkPipelineLayout layout,
		//	VkPipelineCreateFlags flags);

		//VkPushConstantRange pushConstantRange(
		//	VkShaderStageFlags stageFlags,
		//	uint32_t size,
		//	uint32_t offset);
























		vk::MemoryAllocateInfo memoryAllocateInfo2();

		vk::CommandBufferAllocateInfo commandBufferAllocateInfo2(
			vk::CommandPool commandPool,
			vk::CommandBufferLevel level,
			uint32_t bufferCount);


		vk::CommandPoolCreateInfo commandPoolCreateInfo2();
		vk::CommandBufferBeginInfo commandBufferBeginInfo2();
		vk::CommandBufferInheritanceInfo commandBufferInheritanceInfo2();

		vk::RenderPassBeginInfo renderPassBeginInfo2();
		vk::RenderPassCreateInfo renderPassCreateInfo2();

		vk::ImageMemoryBarrier imageMemoryBarrier2();
		vk::BufferMemoryBarrier bufferMemoryBarrier2();
		VkMemoryBarrier memoryBarrier2();


		vk::ImageCreateInfo imageCreateInfo2();
		vk::SamplerCreateInfo samplerCreateInfo2();
		vk::ImageViewCreateInfo imageViewCreateInfo2();


		vk::FramebufferCreateInfo framebufferCreateInfo2();

		vk::SemaphoreCreateInfo semaphoreCreateInfo2();
		vk::FenceCreateInfo fenceCreateInfo2(vk::FenceCreateFlags flags/* = VK_FLAGS_NONE*/);
		vk::EventCreateInfo eventCreateInfo2();


		vk::SubmitInfo submitInfo2();

		vk::Viewport viewport2(
			float width,
			float height,
			float minDepth,
			float maxDepth);

		vk::Rect2D rect2D2(
			int32_t width,
			int32_t height,
			int32_t offsetX,
			int32_t offsetY);

		vk::BufferCreateInfo bufferCreateInfo2();

		vk::BufferCreateInfo bufferCreateInfo2(
			vk::BufferUsageFlags usage,
			vk::DeviceSize size);

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo2(
			uint32_t poolSizeCount,
			vk::DescriptorPoolSize* pPoolSizes,
			uint32_t maxSets);

		vk::DescriptorPoolSize descriptorPoolSize2(
			vk::DescriptorType type,
			uint32_t descriptorCount);

		vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding2(
			vk::DescriptorType type,
			vk::ShaderStageFlags stageFlags,
			uint32_t binding,
			uint32_t count = 1);

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo2(
			const vk::DescriptorSetLayoutBinding* pBindings,
			uint32_t bindingCount);

		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo2(
			const vk::DescriptorSetLayout* pSetLayouts,
			uint32_t setLayoutCount);

		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo2(
			vk::DescriptorPool descriptorPool,
			const vk::DescriptorSetLayout* pSetLayouts,
			uint32_t descriptorSetCount);

		vk::DescriptorImageInfo descriptorImageInfo2(
			vk::Sampler sampler,
			vk::ImageView imageView,
			vk::ImageLayout imageLayout);

		vk::WriteDescriptorSet writeDescriptorSet2(
			vk::DescriptorSet dstSet,
			vk::DescriptorType type,
			uint32_t binding,
			vk::DescriptorBufferInfo* bufferInfo);

		vk::WriteDescriptorSet writeDescriptorSet2(
			vk::DescriptorSet dstSet,
			vk::DescriptorType type,
			uint32_t binding,
			vk::DescriptorImageInfo* imageInfo);

		vk::VertexInputBindingDescription vertexInputBindingDescription2(
			uint32_t binding,
			uint32_t stride,
			vk::VertexInputRate inputRate);

		vk::VertexInputAttributeDescription vertexInputAttributeDescription2(
			uint32_t binding,
			uint32_t location,
			vk::Format format,
			uint32_t offset);

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo2();

		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo2(
			vk::PrimitiveTopology topology,
			vk::PipelineInputAssemblyStateCreateFlags flags,
			vk::Bool32 primitiveRestartEnable);

		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo2(
			vk::PolygonMode polygonMode,
			vk::CullModeFlags cullMode,
			vk::FrontFace frontFace,
			vk::PipelineRasterizationStateCreateFlags flags);

		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState2(
			vk::ColorComponentFlags colorWriteMask,
			vk::Bool32 blendEnable);

		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo2(
			uint32_t attachmentCount,
			const vk::PipelineColorBlendAttachmentState* pAttachments);

		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo2(
			vk::Bool32 depthTestEnable,
			vk::Bool32 depthWriteEnable,
			vk::CompareOp depthCompareOp);

		vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo2(
			uint32_t viewportCount,
			uint32_t scissorCount,
			vk::PipelineViewportStateCreateFlags flags);

		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo2(
			vk::SampleCountFlagBits rasterizationSamples,
			vk::PipelineMultisampleStateCreateFlags flags);

		vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo2(
			const vk::DynamicState *pDynamicStates,
			uint32_t dynamicStateCount,
			vk::PipelineDynamicStateCreateFlags flags);

		vk::PipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo2(
			uint32_t patchControlPoints);

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo2(
			vk::PipelineLayout layout,
			vk::RenderPass renderPass,
			vk::PipelineCreateFlags flags);

		vk::ComputePipelineCreateInfo computePipelineCreateInfo2(
			vk::PipelineLayout layout,
			vk::PipelineCreateFlags flags);

		vk::PushConstantRange pushConstantRange2(
			vk::ShaderStageFlags stageFlags,
			uint32_t size,
			uint32_t offset);
































	}

}
