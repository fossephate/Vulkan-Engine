/*
* Class wrapping access to the swap chain
*
* A swap chain is a collection of framebuffers used for rendering
* The swap chain images can then presented to the windowing system
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include "common.hpp"
#include "vulkanTools.h"

namespace vkx {
    struct SwapChainImage {
        vk::Image image;
        vk::ImageView view;
        vk::Fence fence;
    };

    class SwapChain {
    private:
        const vkx::Context& context;
        vk::SurfaceKHR surface;
        vk::SwapchainKHR swapChain;
        vk::PresentInfoKHR presentInfo;

    public:
        std::vector<SwapChainImage> images;
        vk::Format colorFormat;
        vk::ColorSpaceKHR colorSpace;
        uint32_t imageCount{ 0 };
        uint32_t currentImage{ 0 };
        // Index of the deteced graphics and presenting device queue
        uint32_t queueNodeIndex = UINT32_MAX;

        SwapChain(const vkx::Context& context) : context(context) {
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapChain;
            presentInfo.pImageIndices = &currentImage;
        }

        void createSurface(
#ifdef __ANDROID__
            ANativeWindow* window
#else
            GLFWwindow* window
#endif
            ) {
            // Create surface depending on OS
#ifdef __ANDROID__
            vk::AndroidSurfaceCreateInfoKHR surfaceCreateInfo;
            surfaceCreateInfo.window = window;
            surface = instance.createAndroidSurfaceKHR(surfaceCreateInfo);
#else
#ifdef WIN32
            // Create surface depending on OS
            vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo;
            surfaceCreateInfo.hinstance = GetModuleHandle(NULL);
            surfaceCreateInfo.hwnd = glfwGetWin32Window(window);
            surface = context.instance.createWin32SurfaceKHR(surfaceCreateInfo);
#else
            //vk::XcbSurfaceCreateInfoKHR surfaceCreateInfo;
            //surfaceCreateInfo.connection = connection;
            //surfaceCreateInfo.window = window;
            //surface = context.instance.createXcbSurfaceKHR(surfaceCreateInfo);

            VkSurfaceKHR vk_surf;
            vk::Result result = static_cast<vk::Result>(glfwCreateWindowSurface(context.instance, window, NULL, &vk_surf));
            if (result != vk::Result::eSuccess) {
                std::cerr << "Window surface creation failed: " << vk::to_string(result);
                throw std::error_code(result);
            }
            surface = vk_surf;
#endif
#endif

            // Get list of supported surface formats
            std::vector<vk::SurfaceFormatKHR> surfaceFormats = context.physicalDevice.getSurfaceFormatsKHR(surface);
            auto formatCount = surfaceFormats.size();


            // If the surface format list only includes one entry with  vk::Format::eUndefined,
            // there is no preferered format, so we assume  vk::Format::eB8G8R8A8Unorm
            if ((formatCount == 1) && (surfaceFormats[0].format == vk::Format::eUndefined)) {
                colorFormat = vk::Format::eB8G8R8A8Unorm;
            } else {
                // Always select the first available color format
                // If you need a specific format (e.g. SRGB) you'd need to
                // iterate over the list of available surface format and
                // check for it's presence
                colorFormat = surfaceFormats[0].format;
            }
            colorSpace = surfaceFormats[0].colorSpace;

            // Find a queue for both present and graphics
            queueNodeIndex = context.findQueue(vk::QueueFlagBits::eGraphics, surface);
        }

        // Creates an os specific surface
        // Tries to find a graphics and a present queue
        void create(
            vk::Extent2D& size, bool vsync = false
            ) {
            assert(surface);
            assert(queueNodeIndex != UINT32_MAX);
            vk::SwapchainKHR oldSwapchain = swapChain;
            currentImage = 0;

            // Get physical device surface properties and formats
            vk::SurfaceCapabilitiesKHR surfCaps = context.physicalDevice.getSurfaceCapabilitiesKHR(surface);
            // Get available present modes
            std::vector<vk::PresentModeKHR> presentModes = context.physicalDevice.getSurfacePresentModesKHR(surface);
            auto presentModeCount = presentModes.size();

            vk::Extent2D swapchainExtent;
            // width and height are either both -1, or both not -1.
            if (surfCaps.currentExtent.width == -1) {
                // If the surface size is undefined, the size is set to
                // the size of the images requested.
                swapchainExtent = size;
            } else {
                // If the surface size is defined, the swap chain size must match
                swapchainExtent = surfCaps.currentExtent;
                size = surfCaps.currentExtent;
            }

            // Prefer mailbox mode if present, it's the lowest latency non-tearing present  mode
            vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

            if (!vsync) {
                for (size_t i = 0; i < presentModeCount; i++) {
                    if (presentModes[i] == vk::PresentModeKHR::eMailbox) {
                        swapchainPresentMode = vk::PresentModeKHR::eMailbox;
                        break;
                    }
                    if ((swapchainPresentMode != vk::PresentModeKHR::eMailbox) && (presentModes[i] == vk::PresentModeKHR::eImmediate)) {
                        swapchainPresentMode = vk::PresentModeKHR::eImmediate;
                    }
                }
            }

            // Determine the number of images
            uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
            if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount)) {
                desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
            }

            vk::SurfaceTransformFlagBitsKHR preTransform;
            if (surfCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
                preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
            } else {
                preTransform = surfCaps.currentTransform;
            }

            //auto imageFormat = context.physicalDevice.getImageFormatProperties(colorFormat, vk::ImageType::e2D, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment, vk::ImageCreateFlags());
            vk::SwapchainCreateInfoKHR swapchainCI;
            swapchainCI.surface = surface;
            swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
            swapchainCI.imageFormat = colorFormat;
            swapchainCI.imageColorSpace = colorSpace;
            swapchainCI.imageExtent = vk::Extent2D{ swapchainExtent.width, swapchainExtent.height };
            swapchainCI.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
            swapchainCI.preTransform = preTransform;
            swapchainCI.imageArrayLayers = 1;
            swapchainCI.imageSharingMode = vk::SharingMode::eExclusive;
            swapchainCI.queueFamilyIndexCount = 0;
            swapchainCI.pQueueFamilyIndices = NULL;
            swapchainCI.presentMode = swapchainPresentMode;
            swapchainCI.oldSwapchain = oldSwapchain;
            swapchainCI.clipped = true;
            swapchainCI.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

            swapChain = context.device.createSwapchainKHR(swapchainCI);


            // If an existing sawp chain is re-created, destroy the old swap chain
            // This also cleans up all the presentable images
            if (oldSwapchain) {
                for (uint32_t i = 0; i < imageCount; i++) {
                    context.device.destroyImageView(images[i].view);
                }
                context.device.destroySwapchainKHR(oldSwapchain);
            }

            vk::ImageViewCreateInfo colorAttachmentView;
            colorAttachmentView.format = colorFormat;
            colorAttachmentView.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            colorAttachmentView.subresourceRange.levelCount = 1;
            colorAttachmentView.subresourceRange.layerCount = 1;
            colorAttachmentView.viewType = vk::ImageViewType::e2D;

            // Get the swap chain images
            auto swapChainImages = context.device.getSwapchainImagesKHR(swapChain);
            imageCount = (uint32_t)swapChainImages.size();

            // Get the swap chain buffers containing the image and imageview
            images.resize(imageCount);
            for (uint32_t i = 0; i < imageCount; i++) {
                images[i].image = swapChainImages[i];
                colorAttachmentView.image = swapChainImages[i];
                images[i].view = context.device.createImageView(colorAttachmentView);
                images[i].fence = vk::Fence();
            }
        }

        std::vector<vk::Framebuffer> createFramebuffers(vk::FramebufferCreateInfo framebufferCreateInfo) {
            // Verify that the first attachment is null
            assert(framebufferCreateInfo.pAttachments[0] == vk::ImageView());


            std::vector<vk::ImageView> attachments;
            attachments.resize(framebufferCreateInfo.attachmentCount);
            for (size_t i = 0; i < framebufferCreateInfo.attachmentCount; ++i) {
                attachments[i] = framebufferCreateInfo.pAttachments[i];
            }
            framebufferCreateInfo.pAttachments = attachments.data();

            std::vector<vk::Framebuffer> framebuffers;
            framebuffers.resize(imageCount);
            for (uint32_t i = 0; i < imageCount; i++) {
                attachments[0] = images[i].view;
                framebuffers[i] = context.device.createFramebuffer(framebufferCreateInfo);
            }
            return framebuffers;
        }

        // Acquires the next image in the swap chain
        uint32_t acquireNextImage(vk::Semaphore presentCompleteSemaphore) {
            auto resultValue = context.device.acquireNextImageKHR(swapChain, UINT64_MAX, presentCompleteSemaphore, vk::Fence());
            vk::Result result = resultValue.result;
            if (result != vk::Result::eSuccess) {
                // TODO handle eSuboptimalKHR
                std::cerr << "Invalid acquire result: " << vk::to_string(result);
                throw std::error_code(result);
            }

            currentImage = resultValue.value;
            return currentImage;
        }

        void clearSubmitFence(uint32_t index) {
            images[index].fence = vk::Fence();
        }

        vk::Fence getSubmitFence(bool destroy = false) {
            auto& image = images[currentImage];
            while (image.fence) {
                vk::Result fenceRes = context.device.waitForFences(image.fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
                if (fenceRes == vk::Result::eSuccess) {
                    if (destroy) {
                        context.device.destroyFence(image.fence);
                    }
                    image.fence = vk::Fence();
                }
            }

            image.fence = context.device.createFence(vk::FenceCreateFlags());
            return image.fence;
        }

        // Present the current image to the queue
        vk::Result queuePresent(vk::Semaphore waitSemaphore) {
            presentInfo.waitSemaphoreCount = waitSemaphore ? 1 : 0;
            presentInfo.pWaitSemaphores = &waitSemaphore;
            return context.queue.presentKHR(presentInfo);
        }

        // Free all Vulkan resources used by the swap chain
        void cleanup() {
            for (uint32_t i = 0; i < imageCount; i++) {
                context.device.destroyImageView(images[i].view);
            }
            context.device.destroySwapchainKHR(swapChain);
            context.instance.destroySurfaceKHR(surface);
        }
    };
}

