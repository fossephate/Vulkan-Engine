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

#include "vulkan/vulkan.hpp"
#include "vulkanTools.h"

namespace vkx {
	/**
	* @brief Encapsulates access to a Vulkan buffer backed up by device memory
	* @note To be filled by an external source like the VulkanDevice
	*/
	struct TestBuffer {
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
		/*vk::Result*/void map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
			//return vkMapMemory(device, memory, offset, size, 0, &mapped);
			mapped = device.mapMemory(memory, offset, size, {});
		}

		/**
		* Unmap a mapped memory range
		*
		* @note Does not return a result as vkUnmapMemory can't fail
		*/
		void unmap() {
			if (mapped) {
				//vkUnmapMemory(device, memory);
				device.unmapMemory(memory);
				mapped = nullptr;
			}
		}

		/**
		* Attach the allocated memory block to the buffer
		*
		* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
		*
		* @return VkResult of the bindBufferMemory call
		*/
		/*vk::Result*/void bind(vk::DeviceSize offset = 0) {
			//return vkBindBufferMemory(device, buffer, memory, offset);
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

		/**
		* Copies the specified data to the mapped buffer
		*
		* @param data Pointer to the data to copy
		* @param size Size of the data to copy in machine units
		*
		*/
		void copyTo(void* data, vk::DeviceSize size) {
			assert(mapped);
			memcpy(mapped, data, size);
		}

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
		vk::Result flush(VkDeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
			vk::MappedMemoryRange mappedRange;
			mappedRange.memory = memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			//return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
			return device.flushMappedMemoryRanges(1, &mappedRange);
		}

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
		vk::Result invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
			vk::MappedMemoryRange mappedRange;
			mappedRange.memory = memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			//return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
			device.invalidateMappedMemoryRanges(1, &mappedRange);
		}

		/**
		* Release all Vulkan resources held by this buffer
		*/
		void destroy() {
			if (buffer) {
				//vkDestroyBuffer(device, buffer, nullptr);
				device.destroyBuffer(buffer, nullptr);
			}
			if (memory) {
				//vkFreeMemory(device, memory, nullptr);
				device.freeMemory(memory, nullptr);
			}
		}

	};
}