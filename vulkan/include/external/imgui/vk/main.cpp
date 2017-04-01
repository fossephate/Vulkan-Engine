// ImGui - standalone example application for Glfw + Vulkan, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui/imgui.h>

#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
//#define GLFW_INCLUDE_NONE
//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>

#include "vulkan/vulkan.hpp"

#include "imgui_impl_glfw_vulkan.h"

#define IMGUI_MAX_POSSIBLE_BACK_BUFFERS 16

static vk::AllocationCallbacks*   g_Allocator = nullptr;
static vk::Instance               g_Instance = nullptr;
static vk::SurfaceKHR             g_Surface = nullptr;
static vk::PhysicalDevice         g_Gpu = nullptr;
static vk::Device                 g_Device = nullptr;
static vk::SwapchainKHR           g_Swapchain = nullptr;
static vk::RenderPass             g_RenderPass = nullptr;
static uint32_t                 g_QueueFamily = 0;
static vk::Queue                  g_Queue = nullptr;

static vk::Format                 g_ImageFormat = vk::Format::eB8G8R8A8Unorm;
static vk::Format                 g_ViewFormat = vk::Format::eB8G8R8A8Unorm;
static vk::ColorSpaceKHR          g_ColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

static vk::ImageSubresourceRange  g_ImageRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

static vk::PipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static vk::DescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static int                      fb_width, fb_height;
static uint32_t                 g_BackBufferIndex = 0;
static uint32_t                 g_BackBufferCount = 0;
static vk::Image                  g_BackBuffer[IMGUI_MAX_POSSIBLE_BACK_BUFFERS] = {};
static vk::ImageView              g_BackBufferView[IMGUI_MAX_POSSIBLE_BACK_BUFFERS] = {};
static vk::Framebuffer            g_Framebuffer[IMGUI_MAX_POSSIBLE_BACK_BUFFERS] = {};

static uint32_t                 g_FrameIndex = 0;
static vk::CommandPool            g_CommandPool[IMGUI_VK_QUEUED_FRAMES];
static vk::CommandBuffer          g_CommandBuffer[IMGUI_VK_QUEUED_FRAMES];
static vk::Fence                  g_Fence[IMGUI_VK_QUEUED_FRAMES];
static vk::Semaphore              g_Semaphore[IMGUI_VK_QUEUED_FRAMES];

static vk::ClearValue             g_ClearValue = {};

static void check_vk_result(vk::Result err)
{
	if (err == 0) {
		return;
	}
    printf("vk::Result %d\n", err);
	if (err < 0) {
		abort();
	}
}

static void resize_vulkan(GLFWwindow* /*window*/, int w, int h)
{
    vk::Result err;
    vk::SwapchainKHR old_swapchain = g_Swapchain;
    err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);

    // Destroy old Framebuffer:
    for (uint32_t i = 0; i < g_BackBufferCount; i++)
		if (g_BackBufferView[i]) {
			//vkDestroyImageView(g_Device, g_BackBufferView[i], g_Allocator);
			g_Device.destroyImageView(g_BackBufferView[i], g_Allocator);
		}
    for (uint32_t i = 0; i < g_BackBufferCount; i++)
		if (g_Framebuffer[i]) {
			//vkDestroyFramebuffer(g_Device, g_Framebuffer[i], g_Allocator);
			g_Device.destroyFramebuffer(g_Framebuffer[i], g_Allocator);
		}
	if (g_RenderPass) {
		//vkDestroyRenderPass(g_Device, g_RenderPass, g_Allocator);
		g_Device.destroyRenderPass(g_RenderPass, g_Allocator);
	}

    // Create Swapchain:
    {
        vk::SwapchainCreateInfoKHR info = {};
        info.surface = g_Surface;
        info.imageFormat = g_ImageFormat;
        info.imageColorSpace = g_ColorSpace;
        info.imageArrayLayers = 1;
        info.imageUsage |= vk::ImageUsageFlagBits::eColorAttachment;
        info.imageSharingMode = vk::SharingMode::eExclusive;
        info.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        info.presentMode = vk::PresentModeKHR::eFifo;
        info.clipped = VK_TRUE;
        info.oldSwapchain = old_swapchain;
        vk::SurfaceCapabilitiesKHR cap;
        err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_Gpu, g_Surface, &cap);
        check_vk_result(err);
        if (cap.maxImageCount > 0)
            info.minImageCount = (cap.minImageCount + 2 < cap.maxImageCount) ? (cap.minImageCount + 2) : cap.maxImageCount;
        else
            info.minImageCount = cap.minImageCount + 2;
        if (cap.currentExtent.width == 0xffffffff)
        {
            fb_width = w;
            fb_height = h;
            info.imageExtent.width = fb_width;
            info.imageExtent.height = fb_height;
        }
        else
        {
            fb_width = cap.currentExtent.width;
            fb_height = cap.currentExtent.height;
            info.imageExtent.width = fb_width;
            info.imageExtent.height = fb_height;
        }
        err = vkCreateSwapchainKHR(g_Device, &info, g_Allocator, &g_Swapchain);
        check_vk_result(err);
        err = vkGetSwapchainImagesKHR(g_Device, g_Swapchain, &g_BackBufferCount, NULL);
        check_vk_result(err);
        err = vkGetSwapchainImagesKHR(g_Device, g_Swapchain, &g_BackBufferCount, g_BackBuffer);
        check_vk_result(err);
    }
    if (old_swapchain)
        vkDestroySwapchainKHR(g_Device, old_swapchain, g_Allocator);

    // Create the Render Pass:
    {
        vk::AttachmentDescription attachment = {};
        attachment.format = g_ViewFormat;
        attachment.samples = vk::SampleCountFlagBits::e1;
        attachment.loadOp = vk::AttachmentLoadOp::eClear;
        attachment.storeOp = vk::AttachmentStoreOp::eStore;
        attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachment.initialLayout = vk::ImageLayout::eUndefined;
        attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference color_attachment;
        color_attachment.attachment = 0;
        color_attachment.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;
        vk::RenderPassCreateInfo info = {};
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        //err = vkCreateRenderPass(g_Device, &info, g_Allocator, &g_RenderPass);
		g_RenderPass = g_Device.createRenderPass(info, g_Allocator);
		//check_vk_result(err);
    }

    // Create The Image Views
    {
        vk::ImageViewCreateInfo info;
        info.viewType = vk::ImageViewType::e2D;
        info.format = g_ViewFormat;
        info.components.r = vk::ComponentSwizzle::eR;
        info.components.g = vk::ComponentSwizzle::eG;
        info.components.b = vk::ComponentSwizzle::eB;
        info.components.a = vk::ComponentSwizzle::eA;
        info.subresourceRange = g_ImageRange;
        for (uint32_t i = 0; i < g_BackBufferCount; i++)
        {
            info.image = g_BackBuffer[i];
            //err = vkCreateImageView(g_Device, &info, g_Allocator, &g_BackBufferView[i]);
			g_BackBufferView[i] = g_Device.createImageView(info, g_Allocator);
            //check_vk_result(err);
        }
    }

    // Create Framebuffer:
    {
        vk::ImageView attachment[1];
        vk::FramebufferCreateInfo info = {};
        info.renderPass = g_RenderPass;
        info.attachmentCount = 1;
        info.pAttachments = attachment;
        info.width = fb_width;
        info.height = fb_height;
        info.layers = 1;
        for (uint32_t i = 0; i < g_BackBufferCount; i++)
        {
            attachment[0] = g_BackBufferView[i];
            //err = vkCreateFramebuffer(g_Device, &info, g_Allocator, &g_Framebuffer[i]);
			g_Framebuffer[i] = g_Device.createFramebuffer(info, g_Allocator);
            check_vk_result(err);
        }
    }
}

static void setup_vulkan(GLFWwindow* window)
{
    vk::Result err;

    // Create Vulkan Instance
    {
        uint32_t glfw_extensions_count;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
        vk::InstanceCreateInfo create_info = {};
        create_info.enabledExtensionCount = glfw_extensions_count;
        create_info.ppEnabledExtensionNames = glfw_extensions;
        err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(err);
    }

    // Create Window Surface
    {
        err = glfwCreateWindowSurface(g_Instance, window, g_Allocator, &g_Surface);
        check_vk_result(err);
    }

    // Get GPU (WARNING here we assume the first gpu is one we can use)
    {
        uint32_t count = 1;
        err = vkEnumeratePhysicalDevices(g_Instance, &count, &g_Gpu);
        check_vk_result(err);
    }

    // Get queue
    {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(g_Gpu, &count, NULL);
        vk::QueueFamilyProperties* queues = (vk::QueueFamilyProperties*)malloc(sizeof(vk::QueueFamilyProperties) * count);
        vkGetPhysicalDeviceQueueFamilyProperties(g_Gpu, &count, queues);
        for (uint32_t i = 0; i < count; i++)
        {
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                g_QueueFamily = i;
                break;
            }
        }
        free(queues);
    }

    // Check for WSI support
    {
        vk::Bool32 res;
        vkGetPhysicalDeviceSurfaceSupportKHR(g_Gpu, g_QueueFamily, g_Surface, &res);
        if (res != VK_TRUE)
        {
            fprintf(stderr, "Error no WSI support on physical device 0\n");
            exit(-1);
        }
    }

    // Get Surface Format
    {
        vk::Format image_view_format[][2] = {{VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM}, {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM}};
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(g_Gpu, g_Surface, &count, NULL);
        vk::SurfaceFormatKHR *formats = (vk::SurfaceFormatKHR*)malloc(sizeof(vk::SurfaceFormatKHR) * count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(g_Gpu, g_Surface, &count, formats);
        for (size_t i = 0; i < sizeof(image_view_format) / sizeof(image_view_format[0]); i++)
        {
            for (uint32_t j = 0; j < count; j++)
            {
                if (formats[j].format == image_view_format[i][0])
                {
                    g_ImageFormat = image_view_format[i][0];
                    g_ViewFormat = image_view_format[i][1];
                    g_ColorSpace = formats[j].colorSpace;
                }
            }
        }
        free(formats);
    }

    // Create Logical Device
    {
        int device_extension_count = 1;
        const char* device_extensions[] = {"VK_KHR_swapchain"};
        const uint32_t queue_index = 0;
        const uint32_t queue_count = 1;
        const float queue_priority[] = {1.0f};
        vk::DeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = queue_count;
        queue_info[0].pQueuePriorities = queue_priority;

        vk::DeviceCreateInfo create_info = {};
        create_info.queueCreateInfoCount = sizeof(queue_info)/sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = device_extension_count;
        create_info.ppEnabledExtensionNames = device_extensions;
        err = vkCreateDevice(g_Gpu, &create_info, g_Allocator, &g_Device);
        check_vk_result(err);
        vkGetDeviceQueue(g_Device, g_QueueFamily, queue_index, &g_Queue);
    }

    // Create Framebuffers
    {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        resize_vulkan(window, w, h);
        glfwSetFramebufferSizeCallback(window, resize_vulkan);
    }

    // Create Command Buffers
    for (int i = 0; i < IMGUI_VK_QUEUED_FRAMES; i++)
    {
        {
            vk::CommandPoolCreateInfo info = {};
            info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            info.queueFamilyIndex = g_QueueFamily;
            err = vkCreateCommandPool(g_Device, &info, g_Allocator, &g_CommandPool[i]);
            check_vk_result(err);
        }
        {
            vk::CommandBufferAllocateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            info.commandPool = g_CommandPool[i];
            info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            info.commandBufferCount = 1;
            err = vkAllocateCommandBuffers(g_Device, &info, &g_CommandBuffer[i]);
            check_vk_result(err);
        }
        {
            vk::FenceCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            err = vkCreateFence(g_Device, &info, g_Allocator, &g_Fence[i]);
            check_vk_result(err);
        }
        {
			vk::SemaphoreCreateInfo info;
            err = vkCreateSemaphore(g_Device, &info, g_Allocator, &g_Semaphore[i]);
            check_vk_result(err);
        }
    }

    // Create Descriptor Pool
    {
        vk::DescriptorPoolSize pool_size[11] =
        {
            { vk::DescriptorType::eSampler, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eSampledImage, 1000 },
            { vk::DescriptorType::eStorageImage, 1000 },
            { vk::DescriptorType::eUniformTexelBuffer, 1000 },
            { vk::DescriptorType::eStorageTexelBuffer, 1000 },
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eUniformBufferDynamic, 1000 },
            { vk::DescriptorType::eStorageBufferDynamic, 1000 },
            { vk::DescriptorType::eInputAttachment, 1000 }
        };
		vk::DescriptorPoolCreateInfo pool_info;
        pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        pool_info.maxSets = 1000 * 11;
        pool_info.poolSizeCount = 11;
        pool_info.pPoolSizes = pool_size;
        //err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
		g_DescriptorPool = g_Device.createDescriptorPool(pool_info, g_Allocator);
        //check_vk_result(err);
    }
}

static void cleanup_vulkan() {
    //vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);
	g_Device.destroyDescriptorPool(g_DescriptorPool, g_Allocator);

    for (int i = 0; i < IMGUI_VK_QUEUED_FRAMES; i++) {
        //vkDestroyFence(g_Device, g_Fence[i], g_Allocator);
		g_Device.destroyFence(g_Fence[i], g_Allocator);
        //vkFreeCommandBuffers(g_Device, g_CommandPool[i], 1, &g_CommandBuffer[i]);
		g_Device.freeCommandBuffers(g_CommandPool[i], 1, &g_CommandBuffer[i]);
        //vkDestroyCommandPool(g_Device, g_CommandPool[i], g_Allocator);
		g_Device.destroyCommandPool(g_CommandPool[i], g_Allocator);
        //vkDestroySemaphore(g_Device, g_Semaphore[i], g_Allocator);
		g_Device.destroySemaphore(g_Semaphore[i], g_Allocator);
    }
    for (uint32_t i = 0; i < g_BackBufferCount; i++)
    {
        //vkDestroyImageView(g_Device, g_BackBufferView[i], g_Allocator);
		g_Device.destroyImageView(g_BackBufferView[i], g_Allocator);
        //vkDestroyFramebuffer(g_Device, g_Framebuffer[i], g_Allocator);
		g_Device.destroyFramebuffer(g_Framebuffer[i], g_Allocator);
    }
    //vkDestroyRenderPass(g_Device, g_RenderPass, g_Allocator);
	g_Device.destroyRenderPass(g_RenderPass, g_Allocator);
    //vkDestroySwapchainKHR(g_Device, g_Swapchain, g_Allocator);
	g_Device.destroySwapchainKHR(g_Swapchain, g_Allocator);
    //vkDestroySurfaceKHR(g_Instance, g_Surface, g_Allocator);
	g_Instance.destroySurfaceKHR(g_Surface, g_Allocator);
    //vkDestroyDevice(g_Device, g_Allocator);
	g_Device.destroy(g_Allocator);
    //vkDestroyInstance(g_Instance, g_Allocator);
	g_Instance.destroy(g_Allocator);
}

static void frame_begin()
{
    vk::Result err;
    while (true) {
        //err = vkWaitForFences(g_Device, 1, &g_Fence[g_FrameIndex], VK_TRUE, 100);
		g_Device.waitForFences(1, &g_Fence[g_FrameIndex], VK_TRUE, 100);

		if (err == VK_SUCCESS) {
			break;
		}
		if (err == VK_TIMEOUT) {
			continue;
		}
        check_vk_result(err);
    }
    {
        err = vkAcquireNextImageKHR(g_Device, g_Swapchain, UINT64_MAX, g_Semaphore[g_FrameIndex], VK_NULL_HANDLE, &g_BackBufferIndex);
        check_vk_result(err);
    }
    {
        //err = vkResetCommandPool(g_Device, g_CommandPool[g_FrameIndex], 0);
		g_Device.resetCommandPool(g_CommandPool[g_FrameIndex], {});
        //check_vk_result(err);

		vk::CommandBufferBeginInfo info;
        info.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        //err = vkBeginCommandBuffer(g_CommandBuffer[g_FrameIndex], &info);
		g_CommandBuffer[g_FrameIndex].begin(info);
        //check_vk_result(err);
    }
    {
		vk::RenderPassBeginInfo info;
        info.renderPass = g_RenderPass;
        info.framebuffer = g_Framebuffer[g_BackBufferIndex];
        info.renderArea.extent.width = fb_width;
        info.renderArea.extent.height = fb_height;
        info.clearValueCount = 1;
        info.pClearValues = &g_ClearValue;
        //vkCmdBeginRenderPass(g_CommandBuffer[g_FrameIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
		g_CommandBuffer[g_FrameIndex].beginRenderPass(info, vk::SubpassContents::eInline);
    }
}

static void frame_end()
{
    vk::Result err;
    vkCmdEndRenderPass(g_CommandBuffer[g_FrameIndex]);
    {
        vk::ImageMemoryBarrier barrier;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = g_BackBuffer[g_BackBufferIndex];
        barrier.subresourceRange = g_ImageRange;
        vkCmdPipelineBarrier(g_CommandBuffer[g_FrameIndex], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
    }
    {
        vk::PipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        vk::SubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &g_Semaphore[g_FrameIndex];
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &g_CommandBuffer[g_FrameIndex];

        err = vkEndCommandBuffer(g_CommandBuffer[g_FrameIndex]);
        check_vk_result(err);
        err = vkResetFences(g_Device, 1, &g_Fence[g_FrameIndex]);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &info, g_Fence[g_FrameIndex]);
        check_vk_result(err);
    }
    {
        vk::Result res;
        vk::SwapchainKHR swapchains[1] = {g_Swapchain};
        uint32_t indices[1] = {g_BackBufferIndex};
		vk::PresentInfoKHR info;
        info.swapchainCount = 1;
        info.pSwapchains = swapchains;
        info.pImageIndices = indices;
        info.pResults = &res;
        err = vkQueuePresentKHR(g_Queue, &info);
        check_vk_result(err);
        check_vk_result(res);
    }
    g_FrameIndex = (g_FrameIndex) % IMGUI_VK_QUEUED_FRAMES;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

int main(int, char**)
{
    // Setup window
    //glfwSetErrorCallback(error_callback);
	//if (!glfwInit()) {
	//	return 1;
	//}

    //glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui Vulkan example", NULL, NULL);

    // Setup Vulkan
    //if (!glfwVulkanSupported()) {
    //    printf("GLFW: Vulkan Not Supported\n");
    //    return 1;
    //}
    //setup_vulkan(window);

    // Setup ImGui binding
    ImGui_ImplGlfwVulkan_Init_Data init_data = {};
    init_data.allocator = g_Allocator;
    init_data.gpu = g_Gpu;
    init_data.device = g_Device;
    init_data.render_pass = g_RenderPass;
    init_data.pipeline_cache = g_PipelineCache;
    init_data.descriptor_pool = g_DescriptorPool;
    init_data.check_vk_result = check_vk_result;
    ImGui_ImplGlfwVulkan_Init(window, true, &init_data);

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    // Upload Fonts
    {
        vk::Result err;
        //err = vkResetCommandPool(g_Device, g_CommandPool[g_FrameIndex], 0);
		g_Device.resetCommandPool(g_CommandPool[g_FrameIndex], {});
        //check_vk_result(err);
        vk::CommandBufferBeginInfo begin_info;
        begin_info.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        //err = vkBeginCommandBuffer(g_CommandBuffer[g_FrameIndex], &begin_info);
		g_CommandBuffer[g_FrameIndex].begin(begin_info);
        //check_vk_result(err);

        ImGui_ImplGlfwVulkan_CreateFontsTexture(g_CommandBuffer[g_FrameIndex]);

        vk::SubmitInfo end_info;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &g_CommandBuffer[g_FrameIndex];
        //err = vkEndCommandBuffer(g_CommandBuffer[g_FrameIndex]);
		g_CommandBuffer[g_FrameIndex].end();
        //check_vk_result(err);
        //err = vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE);
		g_Queue.submit(1, &end_info, VK_NULL_HANDLE);
        //check_vk_result(err);

        //err = vkDeviceWaitIdle(g_Device);
		g_Device.waitIdle();
        //check_vk_result(err);
        ImGui_ImplGlfwVulkan_InvalidateFontUploadObjects();
    }

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);

    // Main loop
    //while (!glfwWindowShouldClose(window)) {
	while(true) {
        //glfwPollEvents();
        ImGui_ImplGlfwVulkan_NewFrame();

        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        {
            static float f = 0.0f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window) {
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window) {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

        g_ClearValue.color.float32[0] = clear_color.x;
        g_ClearValue.color.float32[1] = clear_color.y;
        g_ClearValue.color.float32[2] = clear_color.z;
        g_ClearValue.color.float32[3] = clear_color.w;

        frame_begin();
        ImGui_ImplGlfwVulkan_Render(g_CommandBuffer[g_FrameIndex]);
        frame_end();
    }

    // Cleanup
    //vk::Result err = vkDeviceWaitIdle(g_Device);
	g_Device.waitIdle();
    //check_vk_result(err);
    ImGui_ImplGlfwVulkan_Shutdown();
    cleanup_vulkan();
    
	//glfwTerminate();

    return 0;
}
