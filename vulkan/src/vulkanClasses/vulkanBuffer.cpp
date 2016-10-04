





#include "vulkanBuffer.h"




namespace vkx
{












	/**
	* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
	*
	* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the buffer mapping call
	*/
	//vk::Result map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0, vk::MemoryMapFlags flags = vk::MemoryMapFlags())

	void Buffer::map(vk::DeviceSize size, vk::DeviceSize offset, vk::MemoryMapFlags flags)
	{
		//return vkMapMemory(device, memory, offset, size, 0, &mapped);
		mapped = device.mapMemory(memory, offset, size, flags);
	}

	/**
	* Attach the allocated memory block to the buffer
	*
	* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
	*
	* @return VkResult of the bindBufferMemory call
	*/

	vk::Result Buffer::bind(vk::DeviceSize offset)
	{
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

	void Buffer::setupDescriptor(vk::DeviceSize size, vk::DeviceSize offset)
	{
		descriptor.offset = offset;
		descriptor.buffer = buffer;
		descriptor.range = size;
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

	vk::Result Buffer::flush(vk::DeviceSize size, vk::DeviceSize offset)
	{
		vk::MappedMemoryRange mappedRange;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
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

	vk::Result Buffer::invalidate(vk::DeviceSize size, vk::DeviceSize offset)
	{
		vk::MappedMemoryRange mappedRange;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return device.invalidateMappedMemoryRanges(1, &mappedRange);
	}

	/**
	* Release all Vulkan resources held by this buffer
	*/

	void Buffer::destroy()
	{
		if (buffer) {
			//vkDestroyBuffer(device, buffer, nullptr);
			device.destroyBuffer(buffer, nullptr);
		}
		if (memory) {
			//vkFreeMemory(device, memory, nullptr);
			device.freeMemory(memory, nullptr);
		}
	}

	/**
	* Unmap a mapped memory range
	*
	* @note Does not return a result as vkUnmapMemory can't fail
	*/

	void Buffer::unmap()
	{
		if (mapped) {
			//vkUnmapMemory(device, memory);
			device.unmapMemory(memory);
			mapped = nullptr;
		}
	}

}