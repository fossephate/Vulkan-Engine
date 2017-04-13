

#include "vulkanSwapchain.h"

// global
#include "main/global.h"


uint32_t findQueue(const vk::PhysicalDevice& physicalDevice, const vk::QueueFlags& flags, const vk::SurfaceKHR& presentSurface = vk::SurfaceKHR()) {
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


namespace vkx {


	VulkanSwapChain::VulkanSwapChain(const vkx::Context &context) : context(context) {
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &currentImage;
	}


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
	void vkx::VulkanSwapChain::createSurface(SDL_Window * SDLWindow, SDL_SysWMinfo windowInfo)
#elif defined(__linux__)
	void vkx::VulkanSwapChain::initSurface(xcb_connection_t * connection, xcb_window_t window)
#elif defined(__ANDROID__)
	void vkx::VulkanSwapChain::initSurface(ANativeWindow * window)
#endif
	{

		/* WINDOWS */
#if defined(_WIN32)

		HINSTANCE SDLhinstance = GetModuleHandle(NULL);// not sure how this works? (gets current window?) might limit number of windows later
		HWND SDLhandle = windowInfo.info.win.window;

		vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.hinstance = (HINSTANCE)SDLhinstance;
		surfaceCreateInfo.hwnd = (HWND)SDLhandle;

		surface = context.instance.createWin32SurfaceKHR(surfaceCreateInfo);


		/* LINUX */
#elif defined(__linux__)
		vk::XcbSurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.connection = connection;
		surfaceCreateInfo.window = window;
		err = vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
		/* ANDROID */
#elif defined(__ANDROID__)
		vk::AndroidSurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.window = window;
		err = vkCreateAndroidSurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
#endif

		// Get available queue family properties
		uint32_t queueCount;
		//vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
		context.physicalDevice.getQueueFamilyProperties(&queueCount, nullptr);
		assert(queueCount >= 1);

		std::vector<vk::QueueFamilyProperties> queueProps(queueCount);
		context.physicalDevice.getQueueFamilyProperties(&queueCount, queueProps.data());

		queueNodeIndex = findQueue(context.physicalDevice, vk::QueueFlagBits::eGraphics, surface);

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


	}







	/**
	* Create the swapchain and get it's images with given width and height
	*
	* @param width Pointer to the width of the swapchain (may be adjusted to fit the requirements of the swapchain)
	* @param height Pointer to the height of the swapchain (may be adjusted to fit the requirements of the swapchain)
	* @param vsync (Optional) Can be used to force vsync'd rendering (by using VK_PRESENT_MODE_FIFO_KHR as presentation mode)
	*/
	void vkx::VulkanSwapChain::create(vk::Extent2D size, bool vsync = false) {

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


		// Select a present mode for the swapchain

		// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
		// This mode waits for the vertical blank ("v-sync")
		vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

		// If v-sync is not requested, try to find a mailbox mode
		// It's the lowest latency non-tearing present mode available
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

		// Find the transformation of the surface
		vk::SurfaceTransformFlagBitsKHR preTransform;
		if (surfCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
			// We prefer a non-rotated transform
			preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;//lol @ rotate
		} else {
			preTransform = surfCaps.currentTransform;
		}

		vk::SwapchainCreateInfoKHR swapchainCI;
		swapchainCI.pNext = NULL;
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
		// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		swapchainCI.clipped = true;
		swapchainCI.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

		swapChain = context.device.createSwapchainKHR(swapchainCI);

		// If an existing swap chain is re-created, destroy the old swap chain
		// This also cleans up all the presentable images
		if (oldSwapchain) {
			for (uint32_t i = 0; i < imageCount; i++) {
				context.device.destroyImageView(scimages[i].view);
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
		scimages.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; i++) {
			scimages[i].image = swapChainImages[i];
			colorAttachmentView.image = swapChainImages[i];
			scimages[i].view = context.device.createImageView(colorAttachmentView);
			scimages[i].fence = vk::Fence();
		}
	}



	std::vector<vk::Framebuffer> vkx::VulkanSwapChain::createFramebuffers(vk::FramebufferCreateInfo framebufferCreateInfo) {
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
			attachments[0] = scimages[i].view;
			framebuffers[i] = context.device.createFramebuffer(framebufferCreateInfo);
		}
		return framebuffers;
	}



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
	vk::Result vkx::VulkanSwapChain::acquireNextImage(vk::Semaphore presentCompleteSemaphore, uint32_t *imageIndex) {
		// By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
		// With that we don't have to handle VK_NOT_READY
		//return fpAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentCompleteSemaphore, (vk::Fence)nullptr, imageIndex);
		return context.device.acquireNextImageKHR(swapChain, UINT64_MAX, presentCompleteSemaphore, (vk::Fence)nullptr, imageIndex);
	}



	uint32_t vkx::VulkanSwapChain::acquireNextImage(vk::Semaphore presentCompleteSemaphore) {
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


	void vkx::VulkanSwapChain::clearSubmitFence(uint32_t index) {
		scimages[index].fence = vk::Fence();
	}

	vk::Fence vkx::VulkanSwapChain::getSubmitFence(bool destroy) {
		auto &image = scimages[currentImage];
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

	/**
	* Queue an image for presentation
	*
	* @param queue Presentation queue for presenting the image
	* @param imageIndex Index of the swapchain image to queue for presentation
	* @param waitSemaphore (Optional) Semaphore that is waited on before the image is presented (only used if != VK_NULL_HANDLE)
	*
	* @return vk::Result of the queue presentation
	*/
	vk::Result vkx::VulkanSwapChain::queuePresent(vk::Queue queue, uint32_t imageIndex, vk::Semaphore waitSemaphore = nullptr) {
		vk::PresentInfoKHR presentInfo;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &imageIndex;
		// Check if a wait semaphore has been specified to wait for before presenting the image
		if (waitSemaphore) {
			presentInfo.pWaitSemaphores = &waitSemaphore;
			presentInfo.waitSemaphoreCount = 1;
		}

		return queue.presentKHR(&presentInfo);
	}



	// Present the current image to the queue
	vk::Result vkx::VulkanSwapChain::queuePresent(vk::Queue queue, vk::Semaphore waitSemaphore) {
		presentInfo.waitSemaphoreCount = waitSemaphore ? 1 : 0;
		presentInfo.pWaitSemaphores = &waitSemaphore;
		return queue.presentKHR(presentInfo);
	}




	/**
	* Destroy and free Vulkan resources used for the swapchain
	*/
	void vkx::VulkanSwapChain::cleanup() {
		for (uint32_t i = 0; i < imageCount; i++) {
			context.device.destroyImageView(scimages[i].view);
		}

		context.device.destroySwapchainKHR(swapChain);
		context.instance.destroySurfaceKHR(surface);
	}



}