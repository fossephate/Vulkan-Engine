/*
* Vulkan buffer class
*
* Encapsulates a Vulkan buffer
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>
#include "vulkanTools.h"

namespace vkx
{
	/**
	* @brief Encapsulates access to a Vulkan buffer backed up by device memory
	* @note To be filled by an external source like the VulkanDevice
	*/
	class Buffer
	{

	public:
		vk::Buffer buffer;
		vk::Device device;
		vk::DeviceMemory memory;
		vk::DescriptorBufferInfo descriptor;
		vk::DeviceSize size = 0;
		vk::DeviceSize alignment = 0;
		void* mapped = nullptr;

		/** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
		vk::BufferUsageFlags usageFlags;
		/** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point) */
		vk::MemoryPropertyFlags memoryPropertyFlags;

		/**
		* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
		*
		* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
		* @param offset (Optional) Byte offset from beginning
		*
		* @return VkResult of the buffer mapping call
		*/
		//vk::Result map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0, vk::MemoryMapFlags flags = vk::MemoryMapFlags())
		void map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0, vk::MemoryMapFlags flags = vk::MemoryMapFlags());

		/**
		* Unmap a mapped memory range
		*
		* @note Does not return a result as vkUnmapMemory can't fail
		*/
		void unmap();

		/**
		* Attach the allocated memory block to the buffer
		*
		* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
		*
		* @return VkResult of the bindBufferMemory call
		*/
		vk::Result bind(vk::DeviceSize offset = 0);

		/**
		* Setup the default descriptor for this buffer
		*
		* @param size (Optional) Size of the memory range of the descriptor
		* @param offset (Optional) Byte offset from beginning
		*
		*/
		void setupDescriptor(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

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
		vk::Result flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

		/**
		* Invalidate a memory range of the buffer to make it visible to the host
		*
		* @note Only required for non-coherent memory
		*
		* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
		* @param offset (Optional) Byte offset from beginning
		*
		* @return VkResult of the invalidate call
		*/
		vk::Result invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

		/**
		* Release all Vulkan resources held by this buffer
		*/
		void destroy();

	};
}