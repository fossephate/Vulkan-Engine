
#include "vulkanDevice.h"




namespace vkx
{
	/**
	* Default constructor
	*
	* @param physicalDevice Phyiscal device that is to be used
	*/

	VulkanDevice::VulkanDevice(vk::PhysicalDevice physicalDevice)
	{
		assert(physicalDevice);
		this->physicalDevice = physicalDevice;

		// Store Properties features, limits and properties of the physical device for later use
		// Device properties also contain limits and sparse properties
		//vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		physicalDevice.getProperties(&properties);
		// Features should be checked by the examples before using them
		//vkGetPhysicalDeviceFeatures(physicalDevice, &features);
		physicalDevice.getFeatures(&features);
		// Memory properties are used regularly for creating all kinds of buffer
		//vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
		physicalDevice.getMemoryProperties(&memoryProperties);
		// Queue family properties, used for setting up requested queues upon device creation
		uint32_t queueFamilyCount;
		//vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		physicalDevice.getQueueFamilyProperties(&queueFamilyCount, nullptr);
		assert(queueFamilyCount > 0);
		queueFamilyProperties.resize(queueFamilyCount);
		//vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());
		physicalDevice.getQueueFamilyProperties(&queueFamilyCount, queueFamilyProperties.data());
	}

	/**
	* Get the index of a memory type that has all the requested property bits set
	*
	* @param typeBits Bitmask with bits set for each memory type supported by the resource to request for (from vk::MemoryRequirements)
	* @param properties Bitmask of properties for the memory type to request
	* @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
	*
	* @return Index of the requested memory type
	*
	* @throw Throws an exception if memTypeFound is null and no memory type could be found that supports the requested properties
	*/

	uint32_t VulkanDevice::getMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties, vk::Bool32 * memTypeFound)
	{
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
			if ((typeBits & 1) == 1) {
				if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					if (memTypeFound) {
						*memTypeFound = true;
					}
					return i;
				}
			}
			typeBits >>= 1;
		}

		#if defined(__ANDROID__)
		//todo : Exceptions are disabled by default on Android (need to add LOCAL_CPP_FEATURES += exceptions to Android.mk), so for now just return zero
		if (memTypeFound) {
			*memTypeFound = false;
		}
		return 0;
		#else
		if (memTypeFound) {
			*memTypeFound = false;
			return 0;
		} else {
			throw std::runtime_error("Could not find a matching memory type");
		}
		#endif
	}

	/**
	* Get the index of a queue family that supports the requested queue flags
	*
	* @param queueFlags Queue flags to find a queue family index for
	*
	* @return Index of the queue family index that matches the flags
	*
	* @throw Throws an exception if no queue family index could be found that supports the requested flags
	*/

	uint32_t VulkanDevice::getQueueFamiliyIndex(vk::QueueFlags queueFlags)
	{
		// If a compute queue is requested, try to find a separate compute queue family from graphics first
		if (queueFlags & vk::QueueFlagBits::eCompute) {
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
				if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlags())) {
					return i;
					break;
				}
			}
		}

		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
			if (queueFamilyProperties[i].queueFlags & queueFlags) {
				return i;
				break;
			}
		}

		#if defined(__ANDROID__)
		//todo : Exceptions are disabled by default on Android (need to add LOCAL_CPP_FEATURES += exceptions to Android.mk), so for now just return zero
		return 0;
		#else
		throw std::runtime_error("Could not find a matching queue family index");
		#endif
	}

	/**
	* Create the logical device based on the assigned physical device, also gets default queue family indices
	*
	* @param enabledFeatures Can be used to enable certain features upon device creation
	* @param useSwapChain Set to false for headless rendering to omit the swapchain device extensions
	* @param requestedQueueTypes Bit flags specifying the queue types to be requested from the device
	*
	* @return vk::Result of the device creation call
	*/

	vk::Result VulkanDevice::createLogicalDevice(vk::PhysicalDeviceFeatures enabledFeatures, bool useSwapChain, vk::QueueFlags requestedQueueTypes)
	{
		// Desired queues need to be requested upon logical device creation
		// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
		// requests different queue types

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

		// Get queue family indices for graphics and compute
		// Note that the indices may overlap depending on the implementation

		// Graphics queue
		if (requestedQueueTypes & vk::QueueFlagBits::eGraphics) {
			queueFamilyIndices.graphics = getQueueFamiliyIndex(vk::QueueFlagBits::eGraphics);
			float queuePriority(0.0f);
			vk::DeviceQueueCreateInfo queueInfo;
			//queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueInfo);
		} else
		{
			queueFamilyIndices.graphics = VK_NULL_HANDLE;
		}

		// Compute queue
		if (requestedQueueTypes & vk::QueueFlagBits::eCompute)
		{
			queueFamilyIndices.compute = getQueueFamiliyIndex(vk::QueueFlagBits::eCompute);
			if (queueFamilyIndices.compute != queueFamilyIndices.graphics)
			{
				// If compute family index differs, we need an additional queue create info for the compute queue
				float queuePriority(0.0f);
				vk::DeviceQueueCreateInfo queueInfo;
				//queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
			// Else we use the same queue
		} else {
			queueFamilyIndices.compute = VK_NULL_HANDLE;
		}

		// Create the logical device representation
		std::vector<const char*> deviceExtensions;
		if (useSwapChain)
		{
			// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		vk::DeviceCreateInfo deviceCreateInfo = {};
		//deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

		// Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
		if (vkx::checkDeviceExtensionPresent(physicalDevice, VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
			deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
			enableDebugMarkers = true;
		}

		if (deviceExtensions.size() > 0) {
			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		//vk::Result result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
		vk::Result result = physicalDevice.createDevice(&deviceCreateInfo, nullptr, &logicalDevice);

		if (result == vk::Result::eSuccess) {
			// Create a default command pool for graphics command buffers
			commandPool = createCommandPool(queueFamilyIndices.graphics);
		}

		return result;
	}

	/**
	* Create a buffer on the device
	*
	* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
	* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
	* @param size Size of the buffer in byes
	* @param buffer Pointer to the buffer handle acquired by the function
	* @param memory Pointer to the memory handle acquired by the function
	* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
	*
	* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
	*/

	vk::Result VulkanDevice::createBuffer(vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryPropertyFlags, vk::DeviceSize size, vk::Buffer * buffer, vk::DeviceMemory * memory, void * data)
	{
		// Create the buffer handle
		vk::BufferCreateInfo bufferCreateInfo = vkx::bufferCreateInfo(usageFlags, size);
		bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

		//VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer));
		logicalDevice.createBuffer(&bufferCreateInfo, nullptr, buffer);

		// Create the memory backing up the buffer handle
		vk::MemoryRequirements memReqs;
		vk::MemoryAllocateInfo memAlloc;

		//vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
		logicalDevice.getBufferMemoryRequirements(*buffer, &memReqs);

		memAlloc.allocationSize = memReqs.size;
		// Find a memory type index that fits the properties of the buffer
		memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);

		//VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));
		logicalDevice.allocateMemory(&memAlloc, nullptr, memory);

		// If a pointer to the buffer data has been passed, map the buffer and copy over the data
		if (data != nullptr)
		{
			void *mapped;
			//VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
			mapped = logicalDevice.mapMemory(*memory, 0, size, vk::MemoryMapFlags());//what?
			memcpy(mapped, data, size);
			vkUnmapMemory(logicalDevice, *memory);
		}

		// Attach the memory to the buffer object
		//VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));
		logicalDevice.bindBufferMemory(*buffer, *memory, 0);

		return vk::Result::eSuccess;
	}

	/**
	* Create a buffer on the device
	*
	* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
	* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
	* @param buffer Pointer to a vk::Vulkan buffer object
	* @param size Size of the buffer in byes
	* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
	*
	* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
	*/

	vk::Result VulkanDevice::createBuffer(vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryPropertyFlags, vkx::Buffer * buffer, vk::DeviceSize size, void * data)
	{
		buffer->device = logicalDevice;

		// Create the buffer handle
		vk::BufferCreateInfo bufferCreateInfo = vkx::bufferCreateInfo(usageFlags, size);
		//VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer));
		logicalDevice.createBuffer(&bufferCreateInfo, nullptr, &buffer->buffer);

		// Create the memory backing up the buffer handle
		vk::MemoryRequirements memReqs;
		vk::MemoryAllocateInfo memAlloc;
		//vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer, &memReqs);
		logicalDevice.getBufferMemoryRequirements(buffer->buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		// Find a memory type index that fits the properties of the buffer
		memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
		//VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer->memory));
		logicalDevice.allocateMemory(&memAlloc, nullptr, &buffer->memory);

		buffer->alignment = memReqs.alignment;
		buffer->size = memAlloc.allocationSize;
		buffer->usageFlags = usageFlags;
		buffer->memoryPropertyFlags = memoryPropertyFlags;

		// If a pointer to the buffer data has been passed, map the buffer and copy over the data
		if (data != nullptr)
		{
			//VK_CHECK_RESULT(buffer->map());
			buffer->map();
			memcpy(buffer->mapped, data, size);
			buffer->unmap();
		}

		// Initialize a default descriptor that covers the whole buffer size
		buffer->setupDescriptor();

		// Attach the memory to the buffer object
		return buffer->bind();
	}

	/**
	* Copy buffer data from src to dst using vk::CmdCopyBuffer
	*
	* @param src Pointer to the source buffer to copy from
	* @param dst Pointer to the destination buffer to copy tp
	* @param queue Pointer
	* @param copyRegion (Optional) Pointer to a copy region, if NULL, the whole buffer is copied
	*
	* @note Source and destionation pointers must have the approriate transfer usage flags set (TRANSFER_SRC / TRANSFER_DST)
	*/

	void VulkanDevice::copyBuffer(vkx::Buffer * src, vkx::Buffer * dst, vk::Queue queue, vk::BufferCopy * copyRegion)
	{
		assert(dst->size <= src->size);
		assert(src->buffer && src->buffer);
		vk::CommandBuffer copyCmd = createCommandBuffer(vk::CommandBufferLevel::ePrimary, true);
		vk::BufferCopy bufferCopy{};
		if (copyRegion == nullptr) {
			bufferCopy.size = src->size;
		} else {
			bufferCopy = *copyRegion;
		}

		//vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);
		copyCmd.copyBuffer(src->buffer, dst->buffer, 1, &bufferCopy);

		flushCommandBuffer(copyCmd, queue);
	}

	/**
	* Create a command pool for allocation command buffers from
	*
	* @param queueFamilyIndex Family index of the queue to create the command pool for
	* @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
	*
	* @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
	*
	* @return A handle to the created command buffer
	*/

	vk::CommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags createFlags)
	{
		vk::CommandPoolCreateInfo cmdPoolInfo;
		cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
		cmdPoolInfo.flags = createFlags;
		vk::CommandPool cmdPool;
		//VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));
		logicalDevice.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool);
		return cmdPool;
	}

	/**
	* Allocate a command buffer from the command pool
	*
	* @param level Level of the new command buffer (primary or secondary)
	* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
	*
	* @return A handle to the allocated command buffer
	*/

	vk::CommandBuffer VulkanDevice::createCommandBuffer(vk::CommandBufferLevel level, bool begin)
	{
		vk::CommandBufferAllocateInfo cmdBufAllocateInfo = vkx::commandBufferAllocateInfo(commandPool, level, 1);

		vk::CommandBuffer cmdBuffer;
		//VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
		logicalDevice.allocateCommandBuffers(&cmdBufAllocateInfo, &cmdBuffer);

		// If requested, also start recording for the new command buffer
		if (begin) {
			vk::CommandBufferBeginInfo cmdBufInfo;
			//VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
			cmdBuffer.begin(&cmdBufInfo);
		}

		return cmdBuffer;
	}

	/**
	* Finish command buffer recording and submit it to a queue
	*
	* @param commandBuffer Command buffer to flush
	* @param queue Queue to submit the command buffer to
	* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
	*
	* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
	* @note Uses a fence to ensure command buffer has finished executing
	*/

	void VulkanDevice::flushCommandBuffer(vk::CommandBuffer commandBuffer, vk::Queue queue, bool free)
	{
		if ((bool)commandBuffer == VK_NULL_HANDLE) {
			return;
		}

		//VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
		commandBuffer.end();

		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// Create fence to ensure that the command buffer has finished executing
		vk::FenceCreateInfo fenceInfo;
		vk::Fence fence;
		//VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
		logicalDevice.createFence(&fenceInfo, nullptr, &fence);

		// Submit to the queue
		//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		queue.submit(1, &submitInfo, fence);
		// Wait for the fence to signal that command buffer has finished executing
		//VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
		logicalDevice.waitForFences(1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

		//vkDestroyFence(logicalDevice, fence, nullptr);
		logicalDevice.destroyFence(fence, nullptr);

		if (free) {
			//vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
			logicalDevice.freeCommandBuffers(commandPool, 1, &commandBuffer);
		}
	}

	/**
	* Default destructor
	*
	* @note Frees the logical device
	*/

	VulkanDevice::~VulkanDevice()
	{
		if (commandPool) {
			//vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
			logicalDevice.destroyCommandPool(commandPool, nullptr);
		}
		if (logicalDevice) {
			//vkDestroyDevice(logicalDevice, nullptr);
			logicalDevice.destroy(nullptr);
		}
	}

}