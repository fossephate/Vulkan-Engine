#pragma once

#include "common.h"
#include "vulkanDebug.h"
#include "vulkanTools.h"
#include "vulkanShaders.h"












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





















    class Context {
    public:
        static std::list<std::string> requestedLayers;
        // Set to true when example is created with enabled validation layers
        bool enableValidation = false;
        // Set to true when the debug marker extension is detected
        bool enableDebugMarkers = false;
        // fps timer (one second interval)
        float fpsTimer = 0.0f;
        // Create application wide Vulkan instance

		static std::set<std::string> getAvailableLayers();




		std::set<std::string> requiredExtensions;

		template<typename Container>
		void requireExtensions(const Container& requestedExtensions) {
			requiredExtensions.insert(requestedExtensions.begin(), requestedExtensions.end());

		}

		void requireExtension(const std::string& requestedExtension) {
			requiredExtensions.insert(requestedExtension);
		}




		void createContext(bool enableValidation = false);

		void destroyContext();

		uint32_t findQueue(const vk::QueueFlags& flags, const vk::SurfaceKHR& presentSurface = vk::SurfaceKHR()) const;

        // Vulkan instance, stores all per-application states
        vk::Instance instance;
        std::vector<vk::PhysicalDevice> physicalDevices;
        // Physical device (GPU) that Vulkan will ise
        vk::PhysicalDevice physicalDevice;
        // Stores physical device properties (for e.g. checking device limits)
        vk::PhysicalDeviceProperties deviceProperties;
        // Stores phyiscal device features (for e.g. checking if a feature is available)
        vk::PhysicalDeviceFeatures deviceFeatures;
        // Stores all available memory (type) properties for the physical device
        vk::PhysicalDeviceMemoryProperties deviceMemoryProperties;
        // Logical device, application's view of the physical device (GPU)
        vk::Device device;
        // vk::Pipeline cache object
        vk::PipelineCache pipelineCache;
        // List of shader modules created (stored for cleanup)
        mutable std::vector<vk::ShaderModule> shaderModules;

        vk::Queue queue;
        // Find a queue that supports graphics operations
        uint32_t graphicsQueueIndex;

        ///////////////////////////////////////////////////////////////////////
        //
        // Object destruction support
        //
        // It's often critical to avoid destroying an object that may be in use by the GPU.  In order to service this need
        // the context class contains structures for objects that are pending deletion.  
        // 
        // The first container is the dumpster, and it just contains a set of lambda objects that when executed, destroy 
        // resources (presumably... in theory the lambda can do anything you want, but the purpose is to contain GPU object 
        // destruction calls).
        //
        // When the application makes use of a function that uses a fence, it can provide that fence to the context as a marker 
        // for destroying all the pending objects.  Anything in the dumpster is migrated to the recycler.
        // 
        // Finally, an application can call the recycle function at regular intervals (perhaps once per frame, perhaps less often)
        // in order to check the fences and execute the associated destructors for any that are signalled.
        using VoidLambda = std::function<void()>;
        using VoidLambdaList = std::list<VoidLambda>;
        using FencedLambda = std::pair<vk::Fence, VoidLambda>;
        using FencedLambdaQueue = std::queue<FencedLambda>;

        // A collection of items queued for destruction.  Once a fence has been created
        // for a queued submit, these items can be moved to the recycler for actual destruction
        // by calling the rec
        VoidLambdaList dumpster;
        FencedLambdaQueue recycler;

        template<typename T>
		void trash(T value, std::function<void(const T& t)> destructor);

        template<typename T>
		void trash(std::vector<T>& values, std::function<void(const T& t)> destructor);

        template<typename T>
		void trash(std::vector<T>& values, std::function<void(const std::vector<T>& t)> destructor);

        //
        // Convenience functions for trashing specific types.  These functions know what kind of function 
        // call to make for destroying a given Vulkan object.
        //

		void trashPipeline(vk::Pipeline& pipeline);

		void trashCommandBuffer(vk::CommandBuffer& cmdBuffer);

		void trashCommandBuffers(std::vector<vk::CommandBuffer>& cmdBuffers);

        // Should be called from time to time by the application to migrate zombie resources
        // to the recycler along with a fence that will be signalled when the objects are 
        // safe to delete.
		void emptyDumpster(vk::Fence fence);

        // Check the recycler fences for signalled status.  Any that are signalled will have their corresponding
        // lambdas executed, freeing up the associated resources
		void recycle();

#ifdef WIN32
        static __declspec(thread) VkCommandPool s_cmdPool;
#else
        static thread_local vk::CommandPool s_cmdPool;
#endif

		const vk::CommandPool getCommandPool() const;

		void destroyCommandPool();

		vk::CommandBuffer createCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary, bool begin = false) const;

		void flushCommandBuffer(vk::CommandBuffer& commandBuffer, bool free = false) const;

		void flushCommandBuffer(vk::CommandBuffer commandBuffer, vk::Queue myQueue, bool free);

        // Create a short lived command buffer which is immediately executed and released
        template <typename F>
		void withPrimaryCommandBuffer(F f) const;

		

		CreateImageResult createImage(const vk::ImageCreateInfo& imageCreateInfo, const vk::MemoryPropertyFlags& memoryPropertyFlags) const;

        using MipData = ::std::pair<vk::Extent3D, vk::DeviceSize>;

		CreateImageResult stageToDeviceImage(vk::ImageCreateInfo imageCreateInfo, const vk::MemoryPropertyFlags& memoryPropertyFlags, vk::DeviceSize size, const void* data, const std::vector<MipData>& mipData = {}) const;

		CreateImageResult stageToDeviceImage(const vk::ImageCreateInfo& imageCreateInfo, const vk::MemoryPropertyFlags& memoryPropertyFlags, const gli::texture2d& tex2D) const;

		CreateBufferResult createBuffer(const vk::BufferUsageFlags& usageFlags, const vk::MemoryPropertyFlags& memoryPropertyFlags, vk::DeviceSize size, const void * data = nullptr) const;

        CreateBufferResult createBuffer(const vk::BufferUsageFlags& usage, vk::DeviceSize size, const void * data = nullptr) const {
            return createBuffer(usage, vk::MemoryPropertyFlagBits::eHostVisible, size, data);
        }

        template <typename T>
		CreateBufferResult createBuffer(const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memoryPropertyFlags, const T& data) const;

        template <typename T>
		CreateBufferResult createBuffer(const vk::BufferUsageFlags& usage, const T& data) const;

        template <typename T>
		CreateBufferResult createBuffer(const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memoryPropertyFlags, const std::vector<T>& data) const;

        template <typename T>
		CreateBufferResult createBuffer(const vk::BufferUsageFlags& usage, const std::vector<T>& data) const;

        template <typename T>
		CreateBufferResult createUniformBuffer(const T& data, size_t count = 3) const;

		template<typename T>
		CreateBufferResult createDynamicUniformBuffer(const std::vector<T>& data) const;

		template<typename T>
		CreateBufferResult createDynamicUniformBufferManual(const T& data, size_t count = 3) const;

		//template<typename T>
		//CreateBufferResult createDynamicUniformBuffer(const T & data, size_t elementSize, size_t count = 3) const;

		//template<typename T>
		//CreateBufferResult createDynamicUniformBuffer(const T & data, size_t count = 3) const;

		void copyToMemory(const vk::DeviceMemory & memory, const void* data, vk::DeviceSize size, vk::DeviceSize offset = 0) const;

        template<typename T>
		void copyToMemory(const vk::DeviceMemory &memory, const T& data, size_t offset = 0) const;

        template<typename T>
		void copyToMemory(const vk::DeviceMemory &memory, const std::vector<T>& data, size_t offset = 0) const;

		CreateBufferResult stageToDeviceBuffer(const vk::BufferUsageFlags& usage, size_t size, const void* data) const;

        template <typename T>
		CreateBufferResult stageToDeviceBuffer(const vk::BufferUsageFlags& usage, const std::vector<T>& data) const;

        template <typename T>
		CreateBufferResult stageToDeviceBuffer(const vk::BufferUsageFlags& usage, const T& data) const;


		vk::Bool32 getMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties, uint32_t * typeIndex) const;

		uint32_t getMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties) const;

        // Load a SPIR-V shader
		// Load a SPIR-V shader
		// actually inline
		inline vk::PipelineShaderStageCreateInfo loadShader(const std::string& fileName, vk::ShaderStageFlagBits stage) const {
			vk::PipelineShaderStageCreateInfo shaderStage;
			shaderStage.stage = stage;
			#if defined(__ANDROID__)
			shaderStage.module = loadShader(androidApp->activity->assetManager, fileName.c_str(), device, stage);
			#else
			shaderStage.module = vkx::loadShader(fileName.c_str(), device, stage);
			#endif
			shaderStage.pName = "main"; // todo : make param
			assert(shaderStage.module);
			shaderModules.push_back(shaderStage.module);
			return shaderStage;
		}

		inline vk::PipelineShaderStageCreateInfo loadGlslShader(const std::string& fileName, vk::ShaderStageFlagBits stage) const;

        void submit(
            const vk::ArrayProxy<const vk::CommandBuffer>& commandBuffers,
            const vk::ArrayProxy<const vk::Semaphore>& wait = {},
            const vk::ArrayProxy<const vk::PipelineStageFlags>& waitStages = {},
            const vk::ArrayProxy<const vk::Semaphore>& signals = {},
            const vk::Fence& fence = vk::Fence()
		);

        using SemaphoreStagePair = std::pair<const vk::Semaphore, const vk::PipelineStageFlags>;

        void submit(
            const vk::ArrayProxy<const vk::CommandBuffer>& commandBuffers,
            const vk::ArrayProxy<const SemaphoreStagePair>& wait = {},
            const vk::ArrayProxy<const vk::Semaphore>& signals = {},
			const vk::Fence& fence = vk::Fence());

		/*vk::Result*/void createBuffer(vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryPropertyFlags, vk::DeviceSize size, vk::Buffer * buffer, vk::DeviceMemory * memory, void * data = nullptr);

		// todo: remove this:
			/*vk::Result*/void createBuffer(vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryPropertyFlags, vkx::TestBuffer * buffer, vk::DeviceSize size, void * data = nullptr);
    };

    // Template specialization for texture objects
    template <>
    inline CreateBufferResult Context::createBuffer(const vk::BufferUsageFlags& usage, const gli::texture_cube& texture) const {
        return createBuffer(usage, (vk::DeviceSize)texture.size(), texture.data());
    }

    template <>
    inline CreateBufferResult Context::createBuffer(const vk::BufferUsageFlags& usage, const gli::texture2d& texture) const {
        return createBuffer(usage, (vk::DeviceSize)texture.size(), texture.data());
    }

    template <>
    inline CreateBufferResult Context::createBuffer(const vk::BufferUsageFlags& usage, const gli::texture& texture) const {
        return createBuffer(usage, (vk::DeviceSize)texture.size(), texture.data());
    }

    template <>
    inline CreateBufferResult Context::createBuffer(const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memoryPropertyFlags, const gli::texture& texture) const {
        return createBuffer(usage, memoryPropertyFlags, (vk::DeviceSize)texture.size(), texture.data());
    }

    template <>
    inline void Context::copyToMemory(const vk::DeviceMemory &memory, const gli::texture& texture, size_t offset) const {
        copyToMemory(memory, texture.data(), vk::DeviceSize(texture.size()), offset);
    }

	template<typename T>
	void Context::trash(T value, std::function<void(const T&t)> destructor) {
		if (!value) {
			return;
		}
		T trashedValue;
		std::swap(trashedValue, value);
		dumpster.push_back([trashedValue, destructor] {
			destructor(trashedValue);
		});
	}

	template<typename T>
	void Context::trash(std::vector<T>& values, std::function<void(const T&t)> destructor) {
		if (values.empty()) {
			return;
		}
		std::vector<T> trashedValues;
		trashedValues.swap(values);
		for (const T& value : trashedValues) {
			trash(value, destructor);
		}
	}

	template<typename T>
	void Context::trash(std::vector<T>& values, std::function<void(const std::vector<T>&t)> destructor) {
		if (values.empty()) {
			return;
		}
		std::vector<T> trashedValues;
		trashedValues.swap(values);
		dumpster.push_back([trashedValues, destructor] {
			destructor(trashedValues);
		});
	}

	// Create a short lived command buffer which is immediately executed and released

	template<typename F>
	void Context::withPrimaryCommandBuffer(F f) const {
		vk::CommandBuffer commandBuffer = createCommandBuffer(vk::CommandBufferLevel::ePrimary, true);
		f(commandBuffer);
		flushCommandBuffer(commandBuffer, true);
	}

	template<typename T>
	inline CreateBufferResult Context::createBuffer(const vk::BufferUsageFlags & usage, const vk::MemoryPropertyFlags & memoryPropertyFlags, const T & data) const {
		return createBuffer(usage, memoryPropertyFlags, sizeof(T), &data);
	}

	template<typename T>
	inline CreateBufferResult Context::createBuffer(const vk::BufferUsageFlags & usage, const T & data) const {
		return createBuffer(usage, vk::MemoryPropertyFlagBits::eHostVisible, data);
	}

	template<typename T>
	inline CreateBufferResult Context::createBuffer(const vk::BufferUsageFlags & usage, const vk::MemoryPropertyFlags & memoryPropertyFlags, const std::vector<T>& data) const {
		return createBuffer(usage, memoryPropertyFlags, data.size() * sizeof(T), (void*)data.data());
	}

	template<typename T>
	inline CreateBufferResult Context::createBuffer(const vk::BufferUsageFlags & usage, const std::vector<T>& data) const {
		return createBuffer(usage, vk::MemoryPropertyFlagBits::eHostVisible, data);
	}

	template<typename T>
	inline CreateBufferResult Context::createUniformBuffer(const T & data, size_t count) const {
		auto alignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
		auto extra = sizeof(T) % alignment;
		auto alignedSize = sizeof(T) + (alignment - extra);
		auto allocatedSize = count * alignedSize;
		CreateBufferResult result = createBuffer(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, allocatedSize);
		result.alignment = alignedSize;
		result.descriptor.range = result.alignment;
		result.map();
		result.copy(data);
		return result;
	}

	template<typename T>
	inline CreateBufferResult Context::createDynamicUniformBuffer(const std::vector<T>& data) const {

		auto alignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
		auto extra = sizeof(T) % alignment;
		auto alignedSize = sizeof(T) + (alignment - extra);
		auto allocatedSize = /*count*/data.size() * alignedSize;



		CreateBufferResult result = createBuffer(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, allocatedSize);
		
		result.alignment = alignedSize;
		result.descriptor.range = result.alignment;
		result.map();
		//result.copy(data);
		result.copy((void*)data.data());
		return result;
	}

	template<typename T>
	inline CreateBufferResult Context::createDynamicUniformBufferManual(const T & data, size_t count) const {

		//auto alignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
		//auto extra = sizeof(T) % alignment;
		//auto alignedSize = sizeof(T) + (alignment - extra);
		//auto allocatedSize = /*count*/data.size() * alignedSize;

		auto alignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
		auto extra = sizeof(T) % alignment;
		auto alignedSize = sizeof(T) + (alignment - extra);
		auto allocatedSize = count * alignedSize;



		CreateBufferResult result = createBuffer(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, allocatedSize);

		result.alignment = alignedSize;
		result.descriptor.range = result.alignment;
		result.map();
		//result.copy(data);
		//result.copy((void*)data.data());
		return result;
	}

	//// todo:
	//template<typename T>
	//inline CreateBufferResult Context::createDynamicUniformBufferManualFlush(const T & data, size_t count) const {

	//	//auto alignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
	//	//auto extra = sizeof(T) % alignment;
	//	//auto alignedSize = sizeof(T) + (alignment - extra);
	//	//auto allocatedSize = /*count*/data.size() * alignedSize;

	//	auto alignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
	//	auto extra = sizeof(T) % alignment;
	//	auto alignedSize = sizeof(T) + (alignment - extra);
	//	auto allocatedSize = count * alignedSize;

	//	//https://github.com/SaschaWillems/Vulkan/tree/master/dynamicuniformbuffer
	//	//(Updates will be flushed manually, the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag will isn't used)

	//	CreateBufferResult result = createBuffer(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eHostVisible /* | vk::MemoryPropertyFlagBits::eHostCoherent*/, allocatedSize);

	//	result.alignment = alignedSize;
	//	result.descriptor.range = result.alignment;
	//	result.map();
	//	//result.copy(data);
	//	//result.copy((void*)data.data());
	//	return result;
	//}

	template<typename T>
	inline void Context::copyToMemory(const vk::DeviceMemory & memory, const T & data, size_t offset) const {
		copyToMemory(memory, &data, sizeof(T), offset);
	}

	template<typename T>
	inline void Context::copyToMemory(const vk::DeviceMemory & memory, const std::vector<T>& data, size_t offset) const {
		copyToMemory(memory, data.data(), data.size() * sizeof(T), offset);
	}

	template<typename T>
	inline CreateBufferResult Context::stageToDeviceBuffer(const vk::BufferUsageFlags & usage, const std::vector<T>& data) const {
		return stageToDeviceBuffer(usage, sizeof(T)* data.size(), data.data());
	}

	template<typename T>
	inline CreateBufferResult Context::stageToDeviceBuffer(const vk::BufferUsageFlags & usage, const T & data) const {
		return stageToDeviceBuffer(usage, sizeof(T), (void*)&data);
	}

}
