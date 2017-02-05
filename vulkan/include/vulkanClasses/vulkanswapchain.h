/*
* Class wrapping access to the swap chain
* 
* A swap chain is a collection of framebuffers used for rendering and presentation to the windowing system
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdlib.h>
#include <string>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>

// global
#include "main/global.h"

// load SDL2 if using it
#if USE_SDL2
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_syswm.h>
#endif



#if defined(_WIN32)
	#include <windows.h>
	#include <fcntl.h>
	#include <io.h>
#endif


#include <vulkan/vulkan.hpp>
#include "vulkanTools.h"
#include "vulkanContext.h"



#if defined(__ANDROID__)
	#include "vulkanAndroid.h"
#endif

// Macro to get a procedure address based on a vulkan instance
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                        \
{                                                                       \
	fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetInstanceProcAddr(inst, "vk"#entrypoint)); \
	if (fp##entrypoint == NULL)                                         \
	{																    \
		exit(1);                                                        \
	}                                                                   \
}

// Macro to get a procedure address based on a vulkan device
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
	fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetDeviceProcAddr(dev, "vk"#entrypoint));   \
	if (fp##entrypoint == NULL)                                         \
	{																    \
		exit(1);                                                        \
	}                                                                   \
}



namespace vkx
{

	/*typedef struct _SwapChainBuffers {
		vk::Image image;
		vk::ImageView view;
	} SwapChainBuffer;*/

	struct SwapChainImage {//better
		vk::Image image;
		vk::ImageView view;
		vk::Fence fence;
	};

	class VulkanSwapChain {
	private:

		const vkx::Context &context;

		vk::SurfaceKHR surface;
		vk::SwapchainKHR swapChain;
		vk::PresentInfoKHR presentInfo;

		// Function pointers
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
		PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
		PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
		PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
		PFN_vkQueuePresentKHR fpQueuePresentKHR;

	public:
		std::vector<SwapChainImage> scimages;

		std::vector<vk::Image> images;
		

		vk::Format colorFormat;
		vk::ColorSpaceKHR colorSpace;

		uint32_t imageCount{ 0 };
		uint32_t currentImage{ 0 };


		// Index of the deteced graphics and presenting device queue
		uint32_t queueNodeIndex = UINT32_MAX;



		//void setContext(vkx::Context ctx);

		VulkanSwapChain(const vkx::Context &context);

		// Creates an os specific surface
		/**
		* Create the surface object, an abstraction for the native platform window
		*
		* @pre Windows
		* @param platformHandle HINSTANCE of the window to create the surface for
		* @param platformWindow HWND of the window to create the surface for
		*
		* @pre Android
		* @param window A native platform window
		*
		* @pre Linux (XCB)
		* @param connection xcb connection to the X Server
		* @param window The xcb window to create the surface for
		* @note Targets other than XCB ar not yet supported
		*/
		// define params for this function based on os and settings
		#if defined(_WIN32)
			void createSurface(SDL_Window * SDLWindow, SDL_SysWMinfo windowInfo);
		#elif defined(__linux__)
			void createSurface(xcb_connection_t* connection, xcb_window_t window);
		#elif defined(__ANDROID__)
			void vkx::VulkanSwapChain::initSurface(ANativeWindow * window)
		#endif

		//#if defined(__linux__)
		//	void initSurface(xcb_connection_t* connection, xcb_window_t window);
		//#elif defined(__ANDROID__)
		//	ANativeWindow* window
		//#endif

		/**
		* Create the swapchain and get it's images with given width and height
		*
		* @param width Pointer to the width of the swapchain (may be adjusted to fit the requirements of the swapchain)
		* @param height Pointer to the height of the swapchain (may be adjusted to fit the requirements of the swapchain)
		* @param vsync (Optional) Can be used to force vsync'd rendering (by using VK_PRESENT_MODE_FIFO_KHR as presentation mode)
		*/
		void create(vk::Extent2D size, bool vsync);



		std::vector<vk::Framebuffer> createFramebuffers(vk::FramebufferCreateInfo framebufferCreateInfo);

		/**
		* Acquires the next image in the swap chain
		*
		* @param presentCompleteSemaphore (Optional) Semaphore that is signaled when the image is ready for use
		* @param imageIndex Pointer to the image index that will be increased if the next image could be acquired
		*
		* @note The function will always wait until the next image has been acquired by setting timeout to UINT64_MAX
		*
		* @return vk::Result of the image acquisition
		*/
		vk::Result acquireNextImage(vk::Semaphore presentCompleteSemaphore, uint32_t *imageIndex);


		uint32_t acquireNextImage(vk::Semaphore presentCompleteSemaphore);



		void clearSubmitFence(uint32_t index);

		vk::Fence getSubmitFence(bool destroy = false);


		/**
		* Queue an image for presentation
		*
		* @param queue Presentation queue for presenting the image
		* @param imageIndex Index of the swapchain image to queue for presentation
		* @param waitSemaphore (Optional) Semaphore that is waited on before the image is presented (only used if != VK_NULL_HANDLE)
		*
		* @return vk::Result of the queue presentation
		*/
		vk::Result queuePresent(vk::Queue queue, uint32_t imageIndex, vk::Semaphore waitSemaphore);

		vk::Result queuePresent(vk::Queue queue, vk::Semaphore waitSemaphore);


		/**
		* Destroy and free Vulkan resources used for the swapchain
		*/
		void cleanup();

	};
}