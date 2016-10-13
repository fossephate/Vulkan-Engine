#pragma once

#include "vulkanContext.h"
#include "vulkanFramebuffer.h"

namespace vkx {

    struct OffscreenRenderer {
        const vkx::Context& context;
        const vk::Device& device;
        const vk::Queue& queue;
        vk::CommandPool cmdPool;
        vk::RenderPass renderPass;
        glm::uvec2 framebufferSize;
        std::vector<vk::Format> colorFormats{ { vk::Format::eB8G8R8A8Unorm } };
        // This value is chosen as an invalid default that signals that the code should pick a specific depth buffer
        // Alternative, you can set this to undefined to explicitly declare you want no depth buffer.
        vk::Format depthFormat = vk::Format::eR8Uscaled;
        struct {
            vk::Semaphore renderStart;
            vk::Semaphore renderComplete;
        } semaphores;
        vkx::Framebuffer framebuffer;
        // Typically the offscreen render results will either be used for a blit operation or a shader read operation
        // so the final color layout is usually either TransferSrcOptimal or ShaderReadOptimal
        vk::ImageLayout colorFinalLayout{ vk::ImageLayout::eTransferSrcOptimal };
        vk::ImageLayout depthFinalLayout{ vk::ImageLayout::eUndefined };
        vk::SubmitInfo submitInfo;
        vk::ImageUsageFlags attachmentUsage{ vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eInputAttachment };
        vk::DescriptorPool descriptorPool;

        OffscreenRenderer(const vkx::Context& context) : context(context), device(context.device), queue(context.queue) {}

		void destroy();

		void prepare();

		void prepareFramebuffer();

		void prepareSampler();

		void prepareRenderPass();
    };
}
