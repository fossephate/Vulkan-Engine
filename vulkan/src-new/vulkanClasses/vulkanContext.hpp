#pragma once

#include "common.hpp"
#include "vulkanDebug.h"
#include "vulkanTools.h"
#include "vulkanShaders.h"

namespace vkx {
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

        static std::set<std::string> getAvailableLayers() {
            std::set<std::string> result;
            auto layers = vk::enumerateInstanceLayerProperties();
            for (auto layer : layers) {
                result.insert(layer.layerName);
            }
            return result;
        }

        void createContext(bool enableValidation = false) {

            this->enableValidation = enableValidation;
            {
                // Vulkan instance
                vk::ApplicationInfo appInfo;
                appInfo.pApplicationName = "VulkanExamples";
                appInfo.pEngineName = "VulkanExamples";
                appInfo.apiVersion = VK_API_VERSION_1_0;

                std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
                // Enable surface extensions depending on os
#if defined(_WIN32)
                enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__ANDROID__)
                enabledExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(__linux__)
                enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
                vk::InstanceCreateInfo instanceCreateInfo;
                instanceCreateInfo.pApplicationInfo = &appInfo;
                if (enabledExtensions.size() > 0) {
                    if (enableValidation) {
                        enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
                    }
                    instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
                    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
                }
                instance = vk::createInstance(instanceCreateInfo);
            }

#if defined(__ANDROID__)
            loadVulkanFunctions(instance);
#endif

            // Physical device
            physicalDevices = instance.enumeratePhysicalDevices();
            // Note :
            // This example will always use the first physical device reported,
            // change the vector index if you have multiple Vulkan devices installed
            // and want to use another one
            physicalDevice = physicalDevices[0];
            struct Version {
                uint32_t patch : 12;
                uint32_t minor : 10;
                uint32_t major : 10;
            } _version;
            // Store properties (including limits) and features of the phyiscal device
            // So examples can check against them and see if a feature is actually supported
            deviceProperties = physicalDevice.getProperties();
            memcpy(&_version, &deviceProperties.apiVersion, sizeof(uint32_t));
            deviceFeatures = physicalDevice.getFeatures();
            // Gather physical device memory properties
            deviceMemoryProperties = physicalDevice.getMemoryProperties();

            // Vulkan device
            {
                // Find a queue that supports graphics operations
                uint32_t graphicsQueueIndex = findQueue(vk::QueueFlagBits::eGraphics);
                std::array<float, 1> queuePriorities = { 0.0f };
                vk::DeviceQueueCreateInfo queueCreateInfo;
                queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = queuePriorities.data();
                std::vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
                vk::DeviceCreateInfo deviceCreateInfo;
                deviceCreateInfo.queueCreateInfoCount = 1;
                deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
                deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
                // enable the debug marker extension if it is present (likely meaning a debugging tool is present)
                if (vkx::checkDeviceExtensionPresent(physicalDevice, VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
                    enabledExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
                    enableDebugMarkers = true;
                }
                if (enabledExtensions.size() > 0) {
                    deviceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
                    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
                }
                if (enableValidation) {
                    deviceCreateInfo.enabledLayerCount = (uint32_t)debug::validationLayerNames.size();
                    deviceCreateInfo.ppEnabledLayerNames = debug::validationLayerNames.data();
                }
                device = physicalDevice.createDevice(deviceCreateInfo);
            }

            if (enableValidation) {
                debug::setupDebugging(instance, vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning);
            }

            if (enableDebugMarkers) {
                debug::marker::setup(device);
            }
            pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());
            // Find a queue that supports graphics operations
            graphicsQueueIndex = findQueue(vk::QueueFlagBits::eGraphics);
            // Get the graphics queue
            queue = device.getQueue(graphicsQueueIndex, 0);

        }

        void destroyContext() {
            queue.waitIdle();
            device.waitIdle();
            for (const auto& trash : dumpster) {
                trash();
            }

            while (!recycler.empty()) {
                recycle();
            }

            destroyCommandPool();
            device.destroyPipelineCache(pipelineCache);
            device.destroy();
            if (enableValidation) {
                debug::freeDebugCallback(instance);
            }
            instance.destroy();
        }

        uint32_t findQueue(const vk::QueueFlags& flags, const vk::SurfaceKHR& presentSurface = vk::SurfaceKHR()) const {
            std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
            size_t queueCount = queueProps.size();
            for (uint32_t i = 0; i < queueCount; i++) {
                if (queueProps[i].queueFlags & flags) {
                    if (presentSurface && !physicalDevice.getSurfaceSupportKHR(i, presentSurface)) {
                        continue;
                    }
                    return i;
                }
            }
            throw std::runtime_error("No queue matches the flags " + vk::to_string(flags));
        }

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
        void trash(T value, std::function<void(const T& t)> destructor) {
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
        void trash(std::vector<T>& values, std::function<void(const T& t)> destructor) {
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
        void trash(std::vector<T>& values, std::function<void(const std::vector<T>& t)> destructor) {
            if (values.empty()) {
                return;
            }
            std::vector<T> trashedValues;
            trashedValues.swap(values);
            dumpster.push_back([trashedValues, destructor] {
                destructor(trashedValues);
            });
        }

        //
        // Convenience functions for trashing specific types.  These functions know what kind of function 
        // call to make for destroying a given Vulkan object.
        //

        void trashPipeline(vk::Pipeline& pipeline) {
            std::function<void(const vk::Pipeline& t)> destructor =
                [this](const vk::Pipeline& pipeline) { device.destroyPipeline(pipeline); };
            trash(pipeline, destructor);
        }

        void trashCommandBuffer(vk::CommandBuffer& cmdBuffer) {
            std::function<void(const vk::CommandBuffer& t)> destructor =
                [this](const vk::CommandBuffer& cmdBuffer) {
                device.freeCommandBuffers(getCommandPool(), cmdBuffer);
            };
            trash(cmdBuffer, destructor);
        }

        void trashCommandBuffers(std::vector<vk::CommandBuffer>& cmdBuffers) {
            std::function<void(const std::vector<vk::CommandBuffer>& t)> destructor =
                [this](const std::vector<vk::CommandBuffer>& cmdBuffers) {
                device.freeCommandBuffers(getCommandPool(), cmdBuffers);
            };
            trash(cmdBuffers, destructor);
        }

        // Should be called from time to time by the application to migrate zombie resources
        // to the recycler along with a fence that will be signalled when the objects are 
        // safe to delete.
        void emptyDumpster(vk::Fence fence) {
            VoidLambdaList newDumpster;
            newDumpster.swap(dumpster);
            recycler.push(FencedLambda{ fence, [fence, newDumpster, this] {
                for (const auto & f : newDumpster) { f(); }
            } });
        }

        // Check the recycler fences for signalled status.  Any that are signalled will have their corresponding
        // lambdas executed, freeing up the associated resources
        void recycle() {
            while (!recycler.empty() && vk::Result::eSuccess == device.getFenceStatus(recycler.front().first)) {
                vk::Fence fence = recycler.front().first;
                VoidLambda lambda = recycler.front().second;
                recycler.pop();

                lambda();

                if (recycler.empty() || fence != recycler.front().first) {
                    device.destroyFence(fence);
                }
            }
        }

#ifdef WIN32
        static __declspec(thread) VkCommandPool s_cmdPool;
#else
        static thread_local vk::CommandPool s_cmdPool;
#endif

        const vk::CommandPool getCommandPool() const {
            if (!s_cmdPool) {
                vk::CommandPoolCreateInfo cmdPoolInfo;
                cmdPoolInfo.queueFamilyIndex = graphicsQueueIndex;
                cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
                s_cmdPool = device.createCommandPool(cmdPoolInfo);
            }
            return s_cmdPool;
        }

        void destroyCommandPool() {
            if (s_cmdPool) {
                device.destroyCommandPool(s_cmdPool);
                s_cmdPool = vk::CommandPool();
            }
        }

        vk::CommandBuffer createCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary, bool begin = false) const {
            vk::CommandBuffer cmdBuffer;
            vk::CommandBufferAllocateInfo cmdBufAllocateInfo;
            cmdBufAllocateInfo.commandPool = getCommandPool();
            cmdBufAllocateInfo.level = level;
            cmdBufAllocateInfo.commandBufferCount = 1;

            cmdBuffer = device.allocateCommandBuffers(cmdBufAllocateInfo)[0];

            // If requested, also start the new command buffer
            if (begin) {
                cmdBuffer.begin(vk::CommandBufferBeginInfo());
            }

            return cmdBuffer;
        }

        void flushCommandBuffer(vk::CommandBuffer& commandBuffer, bool free = false) const {
            if (!commandBuffer) {
                return;
            }

            commandBuffer.end();

            vk::SubmitInfo submitInfo;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            queue.submit(submitInfo, vk::Fence());
            queue.waitIdle();
            device.waitIdle();
            if (free) {
                device.freeCommandBuffers(getCommandPool(), commandBuffer);
                commandBuffer = vk::CommandBuffer();
            }
        }

        // Create a short lived command buffer which is immediately executed and released
        template <typename F>
        void withPrimaryCommandBuffer(F f) const {
            vk::CommandBuffer commandBuffer = createCommandBuffer(vk::CommandBufferLevel::ePrimary, true);
            f(commandBuffer);
            flushCommandBuffer(commandBuffer, true);
        }

        CreateImageResult createImage(const vk::ImageCreateInfo& imageCreateInfo, const vk::MemoryPropertyFlags& memoryPropertyFlags) const {
            CreateImageResult result;
            result.device = device;
            result.image = device.createImage(imageCreateInfo);
            result.format = imageCreateInfo.format;
            vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(result.image);
            vk::MemoryAllocateInfo memAllocInfo;
            memAllocInfo.allocationSize = result.allocSize = memReqs.size;
            memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
            result.memory = device.allocateMemory(memAllocInfo);
            device.bindImageMemory(result.image, result.memory, 0);
            return result;
        }

        using MipData = ::std::pair<vk::Extent3D, vk::DeviceSize>;

        CreateImageResult stageToDeviceImage(vk::ImageCreateInfo imageCreateInfo, const vk::MemoryPropertyFlags& memoryPropertyFlags, vk::DeviceSize size, const void* data, const std::vector<MipData>& mipData = {}) const {
            CreateBufferResult staging = createBuffer(vk::BufferUsageFlagBits::eTransferSrc, size, data);
            imageCreateInfo.usage = imageCreateInfo.usage | vk::ImageUsageFlagBits::eTransferDst;
            CreateImageResult result = createImage(imageCreateInfo, memoryPropertyFlags);

            withPrimaryCommandBuffer([&](const vk::CommandBuffer& copyCmd) {
                vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, imageCreateInfo.mipLevels, 0, 1);
                // Prepare for transfer
                setImageLayout(copyCmd, result.image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, range);

                // Prepare for transfer
                std::vector<vk::BufferImageCopy> bufferCopyRegions;
                {
                    vk::BufferImageCopy bufferCopyRegion;
                    bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                    bufferCopyRegion.imageSubresource.layerCount = 1;
                    if (!mipData.empty()) {
                        for (uint32_t i = 0; i < imageCreateInfo.mipLevels; i++) {
                            bufferCopyRegion.imageSubresource.mipLevel = i;
                            bufferCopyRegion.imageExtent = mipData[i].first;
                            bufferCopyRegions.push_back(bufferCopyRegion);
                            bufferCopyRegion.bufferOffset += mipData[i].second;
                        }
                    } else {
                        bufferCopyRegion.imageExtent = imageCreateInfo.extent;
                        bufferCopyRegions.push_back(bufferCopyRegion);
                    }
                }
                copyCmd.copyBufferToImage(staging.buffer, result.image, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);
                // Prepare for shader read
                setImageLayout(copyCmd, result.image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, range);
            });
            staging.destroy();
            return result;
        }

        CreateImageResult stageToDeviceImage(const vk::ImageCreateInfo& imageCreateInfo, const vk::MemoryPropertyFlags& memoryPropertyFlags, const gli::texture2D& tex2D) const {
            std::vector<MipData> mips;
            for (size_t i = 0; i < imageCreateInfo.mipLevels; ++i) {
                const auto& mip = tex2D[i];
                const auto dims = mip.dimensions();
                mips.push_back({ vk::Extent3D{ (uint32_t)dims.x, (uint32_t)dims.y, 1}, (uint32_t)mip.size() });
            }
            return stageToDeviceImage(imageCreateInfo, memoryPropertyFlags, (vk::DeviceSize)tex2D.size(), tex2D.data(), mips);
        }

        CreateBufferResult createBuffer(const vk::BufferUsageFlags& usageFlags, const vk::MemoryPropertyFlags& memoryPropertyFlags, vk::DeviceSize size, const void * data = nullptr) const {
            CreateBufferResult result;
            result.device = device;
            result.size = size;
            result.descriptor.range = size;
            result.descriptor.offset = 0;

            vk::BufferCreateInfo bufferCreateInfo;
            bufferCreateInfo.usage = usageFlags;
            bufferCreateInfo.size = size;

            result.descriptor.buffer = result.buffer = device.createBuffer(bufferCreateInfo);

            vk::MemoryRequirements memReqs = device.getBufferMemoryRequirements(result.buffer);
            vk::MemoryAllocateInfo memAlloc;
            result.allocSize = memAlloc.allocationSize = memReqs.size;
            memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
            result.memory = device.allocateMemory(memAlloc);
            if (data != nullptr) {
                copyToMemory(result.memory, data, size);
            }
            device.bindBufferMemory(result.buffer, result.memory, 0);
            return result;
        }

        CreateBufferResult createBuffer(const vk::BufferUsageFlags& usage, vk::DeviceSize size, const void * data = nullptr) const {
            return createBuffer(usage, vk::MemoryPropertyFlagBits::eHostVisible, size, data);
        }

        template <typename T>
        CreateBufferResult createBuffer(const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memoryPropertyFlags, const T& data) const {
            return createBuffer(usage, memoryPropertyFlags, sizeof(T), &data);
        }

        template <typename T>
        CreateBufferResult createBuffer(const vk::BufferUsageFlags& usage, const T& data) const {
            return createBuffer(usage, vk::MemoryPropertyFlagBits::eHostVisible, data);
        }

        template <typename T>
        CreateBufferResult createBuffer(const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memoryPropertyFlags, const std::vector<T>& data) const {
            return createBuffer(usage, memoryPropertyFlags, data.size() * sizeof(T), (void*)data.data());
        }

        template <typename T>
        CreateBufferResult createBuffer(const vk::BufferUsageFlags& usage, const std::vector<T>& data) const {
            return createBuffer(usage, vk::MemoryPropertyFlagBits::eHostVisible, data);
        }

        template <typename T>
        CreateBufferResult createUniformBuffer(const T& data, size_t count = 3) const {
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

        void copyToMemory(const vk::DeviceMemory & memory, const void* data, vk::DeviceSize size, vk::DeviceSize offset = 0) const {
            void *mapped = device.mapMemory(memory, offset, size, vk::MemoryMapFlags());
            memcpy(mapped, data, size);
            device.unmapMemory(memory);
        }

        template<typename T>
        void copyToMemory(const vk::DeviceMemory &memory, const T& data, size_t offset = 0) const {
            copyToMemory(memory, &data, sizeof(T), offset);
        }

        template<typename T>
        void copyToMemory(const vk::DeviceMemory &memory, const std::vector<T>& data, size_t offset = 0) const {
            copyToMemory(memory, data.data(), data.size() * sizeof(T), offset);
        }

        CreateBufferResult stageToDeviceBuffer(const vk::BufferUsageFlags& usage, size_t size, const void* data) const {
            CreateBufferResult staging = createBuffer(vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible, size, data);
            CreateBufferResult result = createBuffer(usage | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, size);
            withPrimaryCommandBuffer([&](vk::CommandBuffer copyCmd) {
                copyCmd.copyBuffer(staging.buffer, result.buffer, vk::BufferCopy(0, 0, size));
            });
            device.freeMemory(staging.memory);
            device.destroyBuffer(staging.buffer);
            return result;
        }

        template <typename T>
        CreateBufferResult stageToDeviceBuffer(const vk::BufferUsageFlags& usage, const std::vector<T>& data) const {
            return stageToDeviceBuffer(usage, sizeof(T)* data.size(), data.data());
        }

        template <typename T>
        CreateBufferResult stageToDeviceBuffer(const vk::BufferUsageFlags& usage, const T& data) const {
            return stageToDeviceBuffer(usage, sizeof(T), (void*)&data);
        }


        vk::Bool32 getMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties, uint32_t * typeIndex) const {
            for (uint32_t i = 0; i < 32; i++) {
                if ((typeBits & 1) == 1) {
                    if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                        *typeIndex = i;
                        return true;
                    }
                }
                typeBits >>= 1;
            }
            return false;
        }

        uint32_t getMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties) const {
            uint32_t result = 0;
            if (!getMemoryType(typeBits, properties, &result)) {
                // todo : throw error
            }
            return result;
        }

        // Load a SPIR-V shader
        inline vk::PipelineShaderStageCreateInfo loadShader(const std::string& fileName, vk::ShaderStageFlagBits stage) {
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

        inline vk::PipelineShaderStageCreateInfo loadGlslShader(const std::string& fileName, vk::ShaderStageFlagBits stage) const {
            auto source = readTextFile(fileName.c_str());
            vk::PipelineShaderStageCreateInfo shaderStage;
            shaderStage.stage = stage;
            shaderStage.module = shader::glslToShaderModule(device, stage, source);
            shaderStage.pName = "main";
            shaderModules.push_back(shaderStage.module);
            return shaderStage;
        }

        void submit(
            const vk::ArrayProxy<const vk::CommandBuffer>& commandBuffers,
            const vk::ArrayProxy<const vk::Semaphore>& wait = {},
            const vk::ArrayProxy<const vk::PipelineStageFlags>& waitStages = {},
            const vk::ArrayProxy<const vk::Semaphore>& signals = {},
            const vk::Fence& fence = vk::Fence()
            ) {
            vk::SubmitInfo info;
            info.commandBufferCount = commandBuffers.size();
            info.pCommandBuffers = commandBuffers.data();

            if (signals.size()) {
                info.signalSemaphoreCount = signals.size();
                info.pSignalSemaphores = signals.data();
            }

            assert(waitStages.size() == wait.size());

            if (wait.size()) {
                info.waitSemaphoreCount = wait.size();
                info.pWaitSemaphores = wait.data();
                info.pWaitDstStageMask = waitStages.data();
            }
            info.pWaitDstStageMask = waitStages.data();

            info.signalSemaphoreCount = signals.size();
            queue.submit(info, fence);
        }

        using SemaphoreStagePair = std::pair<const vk::Semaphore, const vk::PipelineStageFlags>;

        void submit(
            const vk::ArrayProxy<const vk::CommandBuffer>& commandBuffers,
            const vk::ArrayProxy<const SemaphoreStagePair>& wait = {},
            const vk::ArrayProxy<const vk::Semaphore>& signals = {},
            const vk::Fence& fence = vk::Fence()) {
            std::vector<vk::Semaphore> waitSemaphores;
            std::vector<vk::PipelineStageFlags> waitStages;
            for (size_t i = 0; i < wait.size(); ++i) {
                const auto& pair = wait.data()[i];
                waitSemaphores.push_back(pair.first);
                waitStages.push_back(pair.second);
            }
            submit(commandBuffers, waitSemaphores, waitStages, signals, fence);
        }
    };

    // Template specialization for texture objects
    template <>
    inline CreateBufferResult Context::createBuffer(const vk::BufferUsageFlags& usage, const gli::textureCube& texture) const {
        return createBuffer(usage, (vk::DeviceSize)texture.size(), texture.data());
    }

    template <>
    inline CreateBufferResult Context::createBuffer(const vk::BufferUsageFlags& usage, const gli::texture2D& texture) const {
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

}
