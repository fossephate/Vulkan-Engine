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

#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

#include "common.h"

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

// Macro to check and display Vulkan return results
#define VK_CHECK_RESULT(f)																				\
{																										\
	vk::Result res = (f);																				\
	if (res != vk::Result::eSuccess) {																	\
		std::cout << "Fatal : vk::Result is \"" << vkx::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl;\
		assert(res == vk::Result::eSuccess);															\
	}																									\
}																										\

namespace vkx {

	// Version information for Vulkan is stored in a single 32 bit integer
	// with individual bits representing the major, minor and patch versions.
	// The maximum possible major and minor version is 512 (look out nVidia)
	// while the maximum possible patch version is 2048
	struct Version {
		Version() : major(0), minor(0), patch(0) {}
		Version(uint32_t version) : Version() { *this = version; }

		Version& operator =(uint32_t version) {
			memcpy(this, &version, sizeof(uint32_t));
			return *this;
		}

		operator uint32_t() const {
			uint32_t result;
			memcpy(&result, this, sizeof(uint32_t));
		}

		std::string toString() const {
			std::stringstream buffer;
			buffer << major << "." << minor << "." << patch;
			return buffer.str();
		}

		const uint32_t patch : 12;
		const uint32_t minor : 10;
		const uint32_t major : 10;

	};


	// Check if extension is globally available
	vk::Bool32 checkGlobalExtensionPresent(const char* extensionName);
	// Check if extension is present on the given device
	vk::Bool32 checkDeviceExtensionPresent(vk::PhysicalDevice physicalDevice, const char* extensionName);
	// Selected a suitable supported depth format starting with 32 bit down to 16 bit
	// Returns false if none of the depth formats in the list is supported by the device
	vk::Format getSupportedDepthFormat(vk::PhysicalDevice physicalDevice);
	vk::AccessFlags accessFlagsForLayout(vk::ImageLayout layout);

	//// Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
	//void setImageLayout(
	//	vk::CommandBuffer cmdbuffer,
	//	vk::Image image,
	//	vk::ImageAspectFlags aspectMask,
	//	vk::ImageLayout oldImageLayout,
	//	vk::ImageLayout newImageLayout,
	//	vk::ImageSubresourceRange subresourceRange);
	//// Uses a fixed sub resource layout with first mip level and layer
	//void setImageLayout(
	//	vk::CommandBuffer cmdbuffer,
	//	vk::Image image,
	//	vk::ImageAspectFlags aspectMask,
	//	vk::ImageLayout oldImageLayout,
	//	vk::ImageLayout newImageLayout);




	// Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
	void setImageLayout(
		vk::CommandBuffer cmdbuffer,
		vk::Image image,
		vk::ImageAspectFlags aspectMask,
		vk::ImageLayout oldImageLayout,
		vk::ImageLayout newImageLayout,
		vk::ImageSubresourceRange subresourceRange,
		vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
		vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eAllCommands);

	// Uses a fixed sub resource layout with first mip level and layer
	void setImageLayout(
		vk::CommandBuffer cmdbuffer,
		vk::Image image,
		vk::ImageAspectFlags aspectMask,
		vk::ImageLayout oldImageLayout,
		vk::ImageLayout newImageLayout,
		vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
		vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eAllCommands);



	/** @brief Inser an image memory barrier into the command buffer */
	void insertImageMemoryBarrier(
		vk::CommandBuffer cmdbuffer,
		vk::Image image,
		vk::AccessFlags srcAccessMask,
		vk::AccessFlags dstAccessMask,
		vk::ImageLayout oldImageLayout,
		vk::ImageLayout newImageLayout,
		vk::PipelineStageFlags srcStageMask,
		vk::PipelineStageFlags dstStageMask,
		vk::ImageSubresourceRange subresourceRange);

	// Load a text file (e.g. GLGL shader) into a std::string
	std::string readTextFile(const std::string& filename);

	// Load a binary file into a buffer (e.g. SPIR-V)
	std::vector<uint8_t> readBinaryFile(const std::string& filename);

	// Load a SPIR-V shader
	#if defined(__ANDROID__)
	vk::ShaderModule loadShader(AAssetManager* assetManager, const char *fileName, vk::Device device, vk::ShaderStageFlagBits stage);
	#else
	vk::ShaderModule loadShader(const std::string& filename, vk::Device device, vk::ShaderStageFlagBits stage);
	#endif

	// Load a GLSL shader
	// Note : Only for testing purposes, support for directly feeding GLSL shaders into Vulkan
	// may be dropped at some point    
	vk::ShaderModule loadShaderGLSL(const std::string& filename, vk::Device device, vk::ShaderStageFlagBits stage);




	// A wrapper class for an allocation, either an Image or Buffer.  Not intended to be used directly
	// but only as a base class providing common functionality for the classes below.
	//
	// Provides easy to use mechanisms for mapping, unmapping and copying host data to the device memory
	struct AllocatedResult {
		vk::Device device;
		vk::DeviceMemory memory;
		vk::DeviceSize size{ 0 };
		vk::DeviceSize alignment{ 0 };
		vk::DeviceSize allocSize{ 0 };
		void* mapped{ nullptr };

		template <typename T = void>
		//inline T* map(size_t offset = 0, size_t size = VK_WHOLE_SIZE) {
		//	mapped = device.mapMemory(memory, offset, size, vk::MemoryMapFlags());
		//	return (T*)mapped;
		//}

		// changed to unmap before mapping// 4/8/17
		inline T* map(size_t offset = 0, size_t size = VK_WHOLE_SIZE) {
			if(mapped) {
				unmap();
			}
			mapped = device.mapMemory(memory, offset, size, vk::MemoryMapFlags());
			return (T*)mapped;
		}

		//inline void unmap() {
		//	device.unmapMemory(memory);
		//	mapped = nullptr;
		//}

		// changed to avoid crash if unmapping already unmapped memory// 4/8/17
		inline void unmap() {
			if (mapped) {
				device.unmapMemory(memory);
				mapped = nullptr;
			}
		}

		inline void copy(size_t size, const void* data, size_t offset = 0) const {
			memcpy((uint8_t*)mapped + offset, data, size);
		}

		template<typename T>
		inline void copy(const T& data, size_t offset = 0) const {
			copy(sizeof(T), &data, offset);
		}

		template<typename T>
		inline void copy(const std::vector<T>& data, size_t offset = 0) const {
			copy(sizeof(T) * data.size(), data.data(), offset);
		}

		virtual void destroy() {
			if (mapped) {
				unmap();
			}
			if (memory) {
				device.freeMemory(memory);
				memory = vk::DeviceMemory();
			}
		}
	};

	// Encaspulates an image, the memory for that image, a view of the image,
	// as well as a sampler and the image format.
	//
	// The sampler is not populated by the allocation code, but is provided
	// for convenience and easy cleanup if it is populated.
	struct CreateImageResult : public AllocatedResult {
	private:
		using Parent = AllocatedResult;
	public:
		vk::Extent3D extent;// added
		vk::Image image;
		vk::ImageView view;
		vk::Sampler sampler;
		vk::Format format{ vk::Format::eUndefined };

		void destroy() override {
			Parent::destroy();
			if (mapped) {
				unmap();
			}
			if (sampler) {
				device.destroySampler(sampler);
				sampler = vk::Sampler();
			}
			if (view) {
				device.destroyImageView(view);
				view = vk::ImageView();
			}
			if (image) {
				device.destroyImage(image);
				image = vk::Image();
			}
			Parent::destroy();
		}
	};

	//struct CreateBufferResult : public AllocatedResult {
	//private:
	//	using Parent = AllocatedResult;
	//public:
	//	vk::Buffer buffer;
	//	vk::DescriptorBufferInfo descriptor;

	//	void destroy() override {
	//		if (mapped) {
	//			unmap();
	//		}
	//		if (buffer) {
	//			device.destroyBuffer(buffer);
	//			buffer = vk::Buffer();
	//		}
	//		Parent::destroy();
	//	}
	//};












	// combined test buffer and create buffer result
	struct CreateBufferResult : public AllocatedResult {
		private:
		using Parent = AllocatedResult;
		public:
		vk::Buffer buffer;
		vk::DescriptorBufferInfo descriptor;

		/** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
		vk::BufferUsageFlags usageFlags;
		/** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point) */
		vk::MemoryPropertyFlags memoryPropertyFlags;


		/**
		* Attach the allocated memory block to the buffer
		*
		* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
		*
		* @return VkResult of the bindBufferMemory call
		*/
		void bind(vk::DeviceSize offset = 0) {
			device.bindBufferMemory(buffer, memory, offset);
		}

		/**
		* Setup the default descriptor for this buffer
		*
		* @param size (Optional) Size of the memory range of the descriptor
		* @param offset (Optional) Byte offset from beginning
		*
		*/
		void setupDescriptor(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
			descriptor.offset = offset;
			descriptor.buffer = buffer;
			descriptor.range = size;
		}

		///**
		//* Copies the specified data to the mapped buffer
		//*
		//* @param data Pointer to the data to copy
		//* @param size Size of the data to copy in machine units
		//*
		//*/
		//void copyTo(void* data, vk::DeviceSize size) {
		//	assert(mapped);
		//	memcpy(mapped, data, size);
		//}

		/**
		* Flush a memory range of the buffer to make it visible to the device
		*
		* @note Only required for non-coherent memory
		*
		* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
		* @param offset (Optional) Byte offset from beginning
		*
		* @return VkResult of the flush call
		*/
		vk::Result flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
			vk::MappedMemoryRange mappedRange;
			mappedRange.memory = memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			return device.flushMappedMemoryRanges(1, &mappedRange);
		}

		///**
		//* Invalidate a memory range of the buffer to make it visible to the host
		//*
		//* @note Only required for non-coherent memory
		//*
		//* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
		//* @param offset (Optional) Byte offset from beginning
		//*
		//* @return VkResult of the invalidate call
		//*/
		//vk::Result invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
		//	vk::MappedMemoryRange mappedRange;
		//	mappedRange.memory = memory;
		//	mappedRange.offset = offset;
		//	mappedRange.size = size;
		//	device.invalidateMappedMemoryRanges(1, &mappedRange);
		//}

		void destroy() override {
			if (mapped) {
				unmap();
			}
			if (buffer) {
				device.destroyBuffer(buffer);
				buffer = vk::Buffer();
			}
			Parent::destroy();
		}

	};







	///**
	//* @brief Encapsulates access to a Vulkan buffer backed up by device memory
	//* @note To be filled by an external source like the VulkanDevice
	//*/
	//struct TestBuffer {
	//	vk::Buffer buffer;
	//	vk::Device device;
	//	vk::DeviceMemory memory;
	//	vk::DescriptorBufferInfo descriptor;
	//	vk::DeviceSize size = 0;
	//	vk::DeviceSize alignment = 0;
	//	void* mapped = nullptr;

	//	/** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
	//	vk::BufferUsageFlags usageFlags;
	//	/** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point) */
	//	vk::MemoryPropertyFlags memoryPropertyFlags;

	//	/**
	//	* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
	//	*
	//	* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
	//	* @param offset (Optional) Byte offset from beginning
	//	*
	//	* @return VkResult of the buffer mapping call
	//	*/
	//	void map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
	//		//return vkMapMemory(device, memory, offset, size, 0, &mapped);
	//		mapped = device.mapMemory(memory, offset, size, {});
	//	}

	//	/**
	//	* Unmap a mapped memory range
	//	*
	//	* @note Does not return a result as vkUnmapMemory can't fail
	//	*/
	//	void unmap() {
	//		if (mapped) {
	//			//vkUnmapMemory(device, memory);
	//			device.unmapMemory(memory);
	//			mapped = nullptr;
	//		}
	//	}

	//	/**
	//	* Attach the allocated memory block to the buffer
	//	*
	//	* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
	//	*
	//	* @return VkResult of the bindBufferMemory call
	//	*/
	//	void bind(vk::DeviceSize offset = 0) {
	//		//return vkBindBufferMemory(device, buffer, memory, offset);
	//		device.bindBufferMemory(buffer, memory, offset);
	//	}

	//	/**
	//	* Setup the default descriptor for this buffer
	//	*
	//	* @param size (Optional) Size of the memory range of the descriptor
	//	* @param offset (Optional) Byte offset from beginning
	//	*
	//	*/
	//	void setupDescriptor(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
	//		descriptor.offset = offset;
	//		descriptor.buffer = buffer;
	//		descriptor.range = size;
	//	}

	//	/**
	//	* Copies the specified data to the mapped buffer
	//	*
	//	* @param data Pointer to the data to copy
	//	* @param size Size of the data to copy in machine units
	//	*
	//	*/
	//	void copyTo(void* data, vk::DeviceSize size) {
	//		assert(mapped);
	//		memcpy(mapped, data, size);
	//	}

	//	/**
	//	* Flush a memory range of the buffer to make it visible to the device
	//	*
	//	* @note Only required for non-coherent memory
	//	*
	//	* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
	//	* @param offset (Optional) Byte offset from beginning
	//	*
	//	* @return VkResult of the flush call
	//	*/
	//	vk::Result flush(VkDeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
	//		vk::MappedMemoryRange mappedRange;
	//		mappedRange.memory = memory;
	//		mappedRange.offset = offset;
	//		mappedRange.size = size;
	//		return device.flushMappedMemoryRanges(1, &mappedRange);
	//	}

	//	/**
	//	* Invalidate a memory range of the buffer to make it visible to the host
	//	*
	//	* @note Only required for non-coherent memory
	//	*
	//	* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
	//	* @param offset (Optional) Byte offset from beginning
	//	*
	//	* @return VkResult of the invalidate call
	//	*/
	//	vk::Result invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
	//		vk::MappedMemoryRange mappedRange;
	//		mappedRange.memory = memory;
	//		mappedRange.offset = offset;
	//		mappedRange.size = size;
	//		device.invalidateMappedMemoryRanges(1, &mappedRange);
	//	}

	//	/**
	//	* Release all Vulkan resources held by this buffer
	//	*/
	//	void destroy() {
	//		if (buffer) {
	//			device.destroyBuffer(buffer, nullptr);
	//		}
	//		if (memory) {
	//			device.freeMemory(memory, nullptr);
	//		}
	//	}

	//};




















	// Contains all vulkan objects
	// required for a uniform data object
	using UniformData = vkx::CreateBufferResult;

	//////////////////////////////////////////////////////////////////////////////
	//
	// Helper functions to create commonly used types while taking 
	// only a subset of the total possible number of structure members
	// (leaving the remaining at reasonable defaults)
	//

	// Contains often used vulkan object initializers
	// Save lot of VK_STRUCTURE_TYPE assignments
	// Some initializers are parameterized for convenience
	vk::ClearColorValue clearColor(const glm::vec4& v);

	vk::CommandBufferAllocateInfo commandBufferAllocateInfo(vk::CommandPool commandPool, vk::CommandBufferLevel level, uint32_t bufferCount);

	vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlags flags);

	vk::Viewport viewport(
		float width,
		float height,
		float minDepth = 0,
		float maxDepth = 1);

	vk::Viewport viewport(
		const glm::uvec2& size,
		float minDepth = 0,
		float maxDepth = 1);

	vk::Viewport viewport(
		const vk::Extent2D& size,
		float minDepth = 0,
		float maxDepth = 1);

	vk::Rect2D rect2D(
		uint32_t width,
		uint32_t height,
		int32_t offsetX = 0,
		int32_t offsetY = 0);

	vk::Rect2D rect2D(
		const glm::uvec2& size,
		const glm::ivec2& offset = glm::ivec2(0));

	vk::Rect2D rect2D(
		const vk::Extent2D& size,
		const vk::Offset2D& offset = vk::Offset2D());

	vk::BufferCreateInfo bufferCreateInfo(
		vk::BufferUsageFlags usage,
		vk::DeviceSize size);

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(
		uint32_t poolSizeCount,
		vk::DescriptorPoolSize* pPoolSizes,
		uint32_t maxSets);

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(
		const std::vector<vk::DescriptorPoolSize>& poolSizes,
		uint32_t maxSets);

	vk::DescriptorPoolSize descriptorPoolSize(
		vk::DescriptorType type,
		uint32_t descriptorCount);

	vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding(
		vk::DescriptorType type,
		vk::ShaderStageFlags stageFlags,
		uint32_t binding);

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
		const vk::DescriptorSetLayoutBinding* pBindings,
		uint32_t bindingCount);

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
		const std::vector<vk::DescriptorSetLayoutBinding> &bindings);

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
		const vk::DescriptorSetLayout* pSetLayouts,
		uint32_t setLayoutCount);

	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(
		vk::DescriptorPool descriptorPool,
		const vk::DescriptorSetLayout* pSetLayouts,
		uint32_t descriptorSetCount);

	vk::DescriptorImageInfo descriptorImageInfo(
		vk::Sampler sampler,
		vk::ImageView imageView,
		vk::ImageLayout imageLayout);

	vk::WriteDescriptorSet writeDescriptorSet(
		vk::DescriptorSet dstSet,
		vk::DescriptorType type,
		uint32_t binding,
		vk::DescriptorBufferInfo* bufferInfo);

	vk::WriteDescriptorSet writeDescriptorSet(
		vk::DescriptorSet dstSet,
		vk::DescriptorType type,
		uint32_t binding,
		vk::DescriptorImageInfo* imageInfo);

	vk::VertexInputBindingDescription vertexInputBindingDescription(
		uint32_t binding,
		uint32_t stride,
		vk::VertexInputRate inputRate);

	vk::VertexInputAttributeDescription vertexInputAttributeDescription(
		uint32_t binding,
		uint32_t location,
		vk::Format format,
		uint32_t offset);

	vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
		vk::PrimitiveTopology topology,
		vk::PipelineInputAssemblyStateCreateFlags flags = vk::PipelineInputAssemblyStateCreateFlags(),
		vk::Bool32 primitiveRestartEnable = VK_FALSE);

	vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
		vk::PolygonMode polygonMode,
		vk::CullModeFlags cullMode,
		vk::FrontFace frontFace,
		vk::PipelineRasterizationStateCreateFlags flags = vk::PipelineRasterizationStateCreateFlags());

	vk::ColorComponentFlags fullColorWriteMask();

	vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
		vk::ColorComponentFlags colorWriteMask = fullColorWriteMask(),
		vk::Bool32 blendEnable = VK_FALSE);

	vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
		uint32_t attachmentCount,
		const vk::PipelineColorBlendAttachmentState* pAttachments);

	vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
		vk::Bool32 depthTestEnable,
		vk::Bool32 depthWriteEnable,
		vk::CompareOp depthCompareOp);

	vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
		uint32_t viewportCount,
		uint32_t scissorCount,
		vk::PipelineViewportStateCreateFlags flags = vk::PipelineViewportStateCreateFlags());

	vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
		vk::SampleCountFlagBits rasterizationSamples,
		vk::PipelineMultisampleStateCreateFlags flags = vk::PipelineMultisampleStateCreateFlags());

	vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
		const vk::DynamicState *pDynamicStates,
		uint32_t dynamicStateCount,
		vk::PipelineDynamicStateCreateFlags flags = vk::PipelineDynamicStateCreateFlags());

	vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
		const std::vector<vk::DynamicState> &pDynamicStates,
		vk::PipelineDynamicStateCreateFlags flags = vk::PipelineDynamicStateCreateFlags());

	vk::PipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(
		uint32_t patchControlPoints);

	vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
		vk::PipelineLayout layout,
		vk::RenderPass renderPass,
		vk::PipelineCreateFlags flags = vk::PipelineCreateFlags());

	vk::ComputePipelineCreateInfo computePipelineCreateInfo(
		vk::PipelineLayout layout,
		vk::PipelineCreateFlags flags = vk::PipelineCreateFlags());

	vk::PushConstantRange pushConstantRange(
		vk::ShaderStageFlags stageFlags,
		uint32_t size,
		uint32_t offset);

	const std::string& getAssetPath();
}
