/*
* Vulkan Example base class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "main/global.h"


#include "vulkanApp.h"

vk::Result vulkanApp::createInstance(bool enableValidation)
{
	this->enableValidation = enableValidation;

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = name.c_str();
	appInfo.pEngineName = name.c_str();
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 13);// VK_API_VERSION_1_0;

	std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

	// Enable surface extensions depending on os
	// no need for SDL2 specifics here
	/* WINDOWS */
	#if defined(_WIN32)
		enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	/* LINUX */
	#elif defined(__linux__)
		enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
	/* ANDROID */
	#elif defined(__ANDROID__)
		enabledExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
	#endif

	vk::InstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	
	// if any extensions are enabled
	if (enabledExtensions.size() > 0) {
		// if validation is enabled
		if (enableValidation) {
			enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
		instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	}
	if (enableValidation) {
		instanceCreateInfo.enabledLayerCount = vkDebug::validationLayerCount;
		instanceCreateInfo.ppEnabledLayerNames = vkDebug::validationLayerNames;
	}
	//return vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
	return vk::createInstance(&instanceCreateInfo, nullptr, &instance);
}

std::string vulkanApp::getWindowTitle()
{
	std::string device(deviceProperties.deviceName);
	std::string windowTitle;
	windowTitle = title + " - " + device;
	if (!enableTextOverlay) {
		windowTitle += " - " + std::to_string(frameCounter) + " fps";
	}
	return windowTitle;
}

const std::string vulkanApp::getAssetPath()
{
	#if defined(__ANDROID__)
		return "";
	#else
		return "./assets/";
	#endif
}

bool vulkanApp::checkCommandBuffers()
{
	for (auto& cmdBuffer : drawCmdBuffers) {
		if (cmdBuffer == VK_NULL_HANDLE) {
			return false;
		}
	}
	return true;
}

void vulkanApp::createCommandBuffers() {
	// Create one command buffer for each swap chain image and reuse for rendering
	drawCmdBuffers.resize(swapChain.imageCount);

	vk::CommandBufferAllocateInfo cmdBufAllocateInfo =
		vkx::commandBufferAllocateInfo(
			cmdPool,
			vk::CommandBufferLevel::ePrimary,
			static_cast<uint32_t>(drawCmdBuffers.size()));

	//VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));
	device.allocateCommandBuffers(&cmdBufAllocateInfo, drawCmdBuffers.data());
}

void vulkanApp::destroyCommandBuffers()
{
	vkFreeCommandBuffers(device, cmdPool, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
}

void vulkanApp::createSetupCommandBuffer()
{
	if (setupCmdBuffer != VK_NULL_HANDLE) {
		//vkFreeCommandBuffers(device, cmdPool, 1, &setupCmdBuffer);
		device.freeCommandBuffers(cmdPool, 1, &setupCmdBuffer);
		setupCmdBuffer = VK_NULL_HANDLE; // todo : check if still necessary
	}

	vk::CommandBufferAllocateInfo cmdBufAllocateInfo =
		vkx::commandBufferAllocateInfo(
			cmdPool,
			vk::CommandBufferLevel::ePrimary,
			1);

	//VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &setupCmdBuffer));
	device.allocateCommandBuffers(&cmdBufAllocateInfo, &setupCmdBuffer);

	vk::CommandBufferBeginInfo cmdBufInfo;

	//VK_CHECK_RESULT(vkBeginCommandBuffer(setupCmdBuffer, &cmdBufInfo));
	setupCmdBuffer.begin(&cmdBufInfo);
}

void vulkanApp::flushSetupCommandBuffer()
{
	if (setupCmdBuffer == VK_NULL_HANDLE) {
		return;
	}

	//VK_CHECK_RESULT(vkEndCommandBuffer(setupCmdBuffer));
	setupCmdBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &setupCmdBuffer;

	//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	queue.submit(1, &submitInfo, VK_NULL_HANDLE);
	//VK_CHECK_RESULT(vkQueueWaitIdle(queue));
	queue.waitIdle();

	//vkFreeCommandBuffers(device, cmdPool, 1, &setupCmdBuffer);
	device.freeCommandBuffers(cmdPool, 1, &setupCmdBuffer);
	
	setupCmdBuffer = VK_NULL_HANDLE; 
}











vk::CommandBuffer vulkanApp::createCommandBuffer(vk::CommandBufferLevel level, bool begin)
{
	vk::CommandBuffer cmdBuffer;

	vk::CommandBufferAllocateInfo cmdBufAllocateInfo =
		vkx::commandBufferAllocateInfo(
			cmdPool,
			level,
			1);

	//VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));
	device.allocateCommandBuffers(&cmdBufAllocateInfo, &cmdBuffer);

	// If requested, also start the new command buffer
	if (begin) {
		vk::CommandBufferBeginInfo cmdBufInfo;
		//VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
		cmdBuffer.begin(&cmdBufInfo);
	}

	return cmdBuffer;
}







void vulkanApp::flushCommandBuffer(vk::CommandBuffer commandBuffer, vk::Queue queue, bool free)
{
	if (commandBuffer == VK_NULL_HANDLE) {
		return;
	}
	
	//VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	queue.submit(1, &submitInfo, VK_NULL_HANDLE);
	//VK_CHECK_RESULT(vkQueueWaitIdle(queue));
	queue.waitIdle();

	if (free) {
		//vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
		device.freeCommandBuffers(cmdPool, 1, &commandBuffer);
	}
}




void vulkanApp::createPipelineCache()
{
	vk::PipelineCacheCreateInfo pipelineCacheCreateInfo;
	//VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
	device.createPipelineCache(&pipelineCacheCreateInfo, nullptr, &pipelineCache);
}




void vulkanApp::prepare()
{
	if (enableDebugMarkers) {
		vkDebug::DebugMarker::setup(device);
	}

	createCommandPool();
	createSetupCommandBuffer();
	setupSwapChain();
	createCommandBuffers();
	setupDepthStencil();
	setupRenderPass();
	createPipelineCache();
	setupFrameBuffer();
	flushSetupCommandBuffer();
	// Recreate setup command buffer for derived class
	createSetupCommandBuffer();
	// Create a simple texture loader class
	textureLoader = new vkTools::VulkanTextureLoader(vulkanDevice, queue, cmdPool);

	#if defined(__ANDROID__)
		textureLoader->assetManager = androidApp->activity->assetManager;
	#endif

	if (enableTextOverlay)
	{
		// Load the text rendering shaders
		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
		shaderStages.push_back(loadShader(getAssetPath() + "shaders/base/textoverlay.vert.spv", vk::ShaderStageFlagBits::eVertex));
		shaderStages.push_back(loadShader(getAssetPath() + "shaders/base/textoverlay.frag.spv", vk::ShaderStageFlagBits::eFragment));
		textOverlay = new vkx::VulkanTextOverlay(
			vulkanDevice,
			queue,
			frameBuffers,
			colorformat,
			depthFormat,
			&width,
			&height,
			shaderStages
			);
		updateTextOverlay();
	}
}




vk::PipelineShaderStageCreateInfo vulkanApp::loadShader(std::string fileName, vk::ShaderStageFlagBits stage)
{
	vk::PipelineShaderStageCreateInfo shaderStage;
	shaderStage.stage = stage;

	#if defined(__ANDROID__)
		shaderStage.module = vkx::loadShader(androidApp->activity->assetManager, fileName.c_str(), device, stage);
	#else
		shaderStage.module = vkx::loadShader(fileName.c_str(), device, stage);
	#endif

	shaderStage.pName = "main"; // todo : make param
	assert(shaderStage.module != NULL);
	shaderModules.push_back(shaderStage.module);
	return shaderStage;
}






vk::Bool32 vulkanApp::createBuffer(vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryPropertyFlags, vk::DeviceSize size, void * data, vk::Buffer * buffer, vk::DeviceMemory * memory)
{
	vk::MemoryRequirements memReqs;
	vk::MemoryAllocateInfo memAlloc;
	vk::BufferCreateInfo bufferCreateInfo = vkx::bufferCreateInfo(usageFlags, size);

	//VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer));
	device.createBuffer(&bufferCreateInfo, nullptr, buffer);

	//vkGetBufferMemoryRequirements(device, *buffer, &memReqs);
	device.getBufferMemoryRequirements(*buffer, &memReqs);

	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
	//VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, memory));
	device.allocateMemory(&memAlloc, nullptr, memory);

	if (data != nullptr) {
		void *mapped;
		//VK_CHECK_RESULT(vkMapMemory(device, *memory, 0, size, 0, &mapped));
		device.mapMemory(*memory, 0, size, 0, &mapped);

		memcpy(mapped, data, size);
		//vkUnmapMemory(device, *memory);
		device.unmapMemory(*memory);
	}
	//VK_CHECK_RESULT(vkBindBufferMemory(device, *buffer, *memory, 0));
	device.bindBufferMemory(*buffer, *memory, 0);

	return true;
}




vk::Bool32 vulkanApp::createBuffer(vk::BufferUsageFlags usage, vk::DeviceSize size, void * data, vk::Buffer *buffer, vk::DeviceMemory *memory)
{
	return createBuffer(usage, vk::MemoryPropertyFlagBits::eHostVisible, size, data, buffer, memory);
}




vk::Bool32 vulkanApp::createBuffer(vk::BufferUsageFlags usage, vk::DeviceSize size, void * data, vk::Buffer * buffer, vk::DeviceMemory * memory, vk::DescriptorBufferInfo * descriptor)
{
	vk::Bool32 res = createBuffer(usage, size, data, buffer, memory);
	if (res) {
		descriptor->offset = 0;
		descriptor->buffer = *buffer;
		descriptor->range = size;
		return true;
	} else {
		return false;
	}
}




vk::Bool32 vulkanApp::createBuffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryPropertyFlags, vk::DeviceSize size, void * data, vk::Buffer * buffer, vk::DeviceMemory * memory, vk::DescriptorBufferInfo * descriptor)
{
	vk::Bool32 res = createBuffer(usage, memoryPropertyFlags, size, data, buffer, memory);
	if (res) {
		descriptor->offset = 0;
		descriptor->buffer = *buffer;
		descriptor->range = size;
		return true;
	} else {
		return false;
	}
}




void vulkanApp::loadMesh(std::string filename, vkMeshLoader::MeshBuffer * meshBuffer, std::vector<vkMeshLoader::VertexLayout> vertexLayout, float scale)
{
	vkMeshLoader::MeshCreateInfo meshCreateInfo;
	meshCreateInfo.scale = glm::vec3(scale);
	meshCreateInfo.center = glm::vec3(0.0f);
	meshCreateInfo.uvscale = glm::vec2(1.0f);
	loadMesh(filename, meshBuffer, vertexLayout, &meshCreateInfo);
}



void vulkanApp::loadMesh(std::string filename, vkMeshLoader::MeshBuffer * meshBuffer, std::vector<vkMeshLoader::VertexLayout> vertexLayout, vkMeshLoader::MeshCreateInfo *meshCreateInfo)
{
	VulkanMeshLoader *mesh = new VulkanMeshLoader(vulkanDevice);

	#if defined(__ANDROID__)
		mesh->assetManager = androidApp->activity->assetManager;
	#endif

	mesh->LoadMesh(filename);
	assert(mesh->m_Entries.size() > 0);

	vk::CommandBuffer copyCmd = vulkanApp::createCommandBuffer(vk::CommandBufferLevel::ePrimary, false);

	mesh->createBuffers(
		meshBuffer,
		vertexLayout,
		meshCreateInfo,
		true,
		copyCmd,
		queue);

	//vkFreeCommandBuffers(device, cmdPool, 1, &copyCmd);
	device.freeCommandBuffers(cmdPool, 1, &copyCmd);

	meshBuffer->dim = mesh->dim.size;

	delete(mesh);
}












void vulkanApp::renderLoop()
{
	destWidth = width;
	destHeight = height;

	#if USE_SDL2
		//this->handleInput();
	#endif

	#if defined(_WIN32)
		MSG msg;
		while (TRUE)
		{
			auto tStart = std::chrono::high_resolution_clock::now();
			if (viewUpdated) {
				viewUpdated = false;
				viewChanged();
			}

			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if (msg.message == WM_QUIT) {
				break;
			}

			render();


			frameCounter++;
			auto tEnd = std::chrono::high_resolution_clock::now();
			auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
			frameTimer = (float)tDiff / 1000.0f;
			camera.update(frameTimer);
			if (camera.moving())
			{
				viewUpdated = true;
			}
			// Convert to clamped timer value
			if (!paused) {
				timer += timerSpeed * frameTimer;
				if (timer > 1.0) {
					timer -= 1.0f;
				}
			}
			fpsTimer += (float)tDiff;
			if (fpsTimer > 1000.0f) {
				if (!enableTextOverlay) {
					std::string windowTitle = getWindowTitle();
					SetWindowText(this->windowHandle, windowTitle.c_str());
				}
				lastFPS = frameCounter;
				updateTextOverlay();
				fpsTimer = 0.0f;
				frameCounter = 0;
			}
		}
	#elif defined(__linux__)
		xcb_flush(connection);
		while (!quit) {
			auto tStart = std::chrono::high_resolution_clock::now();
			if (viewUpdated) {
				viewUpdated = false;
				viewChanged();
			}
			xcb_generic_event_t *event;
			while ((event = xcb_poll_for_event(connection))) {
				handleEvent(event);
				free(event);
			}
			render();
			frameCounter++;
			auto tEnd = std::chrono::high_resolution_clock::now();
			auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
			frameTimer = tDiff / 1000.0f;
			camera.update(frameTimer);
			if (camera.moving()) {
				viewUpdated = true;
			}
			// Convert to clamped timer value
			if (!paused) {
				timer += timerSpeed * frameTimer;
				if (timer > 1.0) {
					timer -= 1.0f;
				}
			}
			fpsTimer += (float)tDiff;
			if (fpsTimer > 1000.0f) {
				if (!enableTextOverlay) {
					std::string windowTitle = getWindowTitle();
					xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
						window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
						windowTitle.size(), windowTitle.c_str());
				}
				lastFPS = frameCounter;
				updateTextOverlay();
				fpsTimer = 0.0f;
				frameCounter = 0;
			}
		}


	#elif defined(__ANDROID__)
		while (1) {
			int ident;
			int events;
			struct android_poll_source* source;
			bool destroy = false;

			focused = true;

			while ((ident = ALooper_pollAll(focused ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {
				if (source != NULL) {
					source->process(androidApp, source);
				}
				if (androidApp->destroyRequested != 0) {
					LOGD("Android app destroy requested");
					destroy = true;
					break;
				}
			}

			// App destruction requested
			// Exit loop, example will be destroyed in application main
			if (destroy) {
				break;
			}

			// Render frame
			if (prepared) {
				auto tStart = std::chrono::high_resolution_clock::now();
				render();
				frameCounter++;
				auto tEnd = std::chrono::high_resolution_clock::now();
				auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
				frameTimer = tDiff / 1000.0f;
				camera.update(frameTimer);
				// Convert to clamped timer value
				if (!paused) {
					timer += timerSpeed * frameTimer;
					if (timer > 1.0)
					{
						timer -= 1.0f;
					}
				}
				fpsTimer += (float)tDiff;
				if (fpsTimer > 1000.0f) {
					lastFPS = frameCounter;
					updateTextOverlay();
					fpsTimer = 0.0f;
					frameCounter = 0;
				}
				// Check gamepad state
				const float deadZone = 0.0015f;
				// todo : check if gamepad is present
				// todo : time based and relative axis positions
				bool updateView = false;
				if (camera.type != Camera::CameraType::firstperson) {
					// Rotate
					if (std::abs(gamePadState.axisLeft.x) > deadZone) {
						rotation.y += gamePadState.axisLeft.x * 0.5f * rotationSpeed;
						camera.rotate(glm::vec3(0.0f, gamePadState.axisLeft.x * 0.5f, 0.0f));
						updateView = true;
					}
					if (std::abs(gamePadState.axisLeft.y) > deadZone) {
						rotation.x -= gamePadState.axisLeft.y * 0.5f * rotationSpeed;
						camera.rotate(glm::vec3(gamePadState.axisLeft.y * 0.5f, 0.0f, 0.0f));
						updateView = true;
					}
					// Zoom
					if (std::abs(gamePadState.axisRight.y) > deadZone) {
						zoom -= gamePadState.axisRight.y * 0.01f * zoomSpeed;
						updateView = true;
					}
					if (updateView) {
						viewChanged();
					}
				} else {
					updateView = camera.updatePad(gamePadState.axisLeft, gamePadState.axisRight, frameTimer);
					if (updateView) {
						viewChanged();
					}
				}
			}
		}
	#endif

	// Flush device to make sure all resources can be freed 
	//vkDeviceWaitIdle(device);
	device.waitIdle();
}






























void vulkanApp::updateTextOverlay()
{
	if (!enableTextOverlay) {
		return;
	}
	textOverlay->beginTextUpdate();
	textOverlay->addText(title, 5.0f, 5.0f, vkx::VulkanTextOverlay::alignLeft);
	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << (frameTimer * 1000.0f) << "ms (" << lastFPS << " fps)";
	textOverlay->addText(ss.str(), 5.0f, 25.0f, vkx::VulkanTextOverlay::alignLeft);
	textOverlay->addText(deviceProperties.deviceName, 5.0f, 45.0f, vkx::VulkanTextOverlay::alignLeft);
	getOverlayText(textOverlay);
	textOverlay->endTextUpdate();
}

void vulkanApp::getOverlayText(vkx::VulkanTextOverlay * textOverlay)
{
	// Can be overriden in derived class
}

void vulkanApp::prepareFrame()
{
	// Acquire the next image from the swap chaing
	//VK_CHECK_RESULT(swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer));
	swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
}

void vulkanApp::submitFrame()
{
	bool submitTextOverlay = enableTextOverlay && textOverlay->visible;

	if (submitTextOverlay) {
		// Wait for color attachment output to finish before rendering the text overlay
		vk::PipelineStageFlags stageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		submitInfo.pWaitDstStageMask = &stageFlags;

		// Set semaphores
		// Wait for render complete semaphore
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &semaphores.renderComplete;
		// Signal ready with text overlay complete semaphpre
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphores.textOverlayComplete;

		// Submit current text overlay command buffer
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &textOverlay->cmdBuffers[currentBuffer];
		//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		queue.submit(1, &submitInfo, VK_NULL_HANDLE);

		// Reset stage mask
		submitInfo.pWaitDstStageMask = &submitPipelineStages;
		// Reset wait and signal semaphores for rendering next frame
		// Wait for swap chain presentation to finish
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &semaphores.presentComplete;
		// Signal ready with offscreen semaphore
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphores.renderComplete;
	}

	//VK_CHECK_RESULT(swapChain.queuePresent(queue, currentBuffer, submitTextOverlay ? semaphores.textOverlayComplete : semaphores.renderComplete));
	swapChain.queuePresent(queue, currentBuffer, submitTextOverlay ? semaphores.textOverlayComplete : semaphores.renderComplete);

	//VK_CHECK_RESULT(vkQueueWaitIdle(queue));
	queue.waitIdle();
}

vulkanApp::vulkanApp(bool enableValidation, PFN_GetEnabledFeatures enabledFeaturesFn)
{
	// Check for validation command line flag
	#if defined(_WIN32)
		for (int32_t i = 0; i < __argc; i++) {
			if (__argv[i] == std::string("-validation")) {
				enableValidation = true;
			}
			if (__argv[i] == std::string("-vsync")) {
				enableVSync = true;
			}
		}
	#elif defined(__linux__)
		initxcbConnection();
	#elif defined(__ANDROID__)
		// Vulkan library is loaded dynamically on Android
		bool libLoaded = loadVulkanLibrary();
		assert(libLoaded);
	#endif

	if (enabledFeaturesFn != nullptr) {
		this->enabledFeatures = enabledFeaturesFn();
	}

	#if defined(_WIN32)
		// Enable console if validation is active
		// Debug message callback will output to it
		if (enableValidation) {
			setupConsole("VulkanExample");
		}
	#endif

	#if !defined(__ANDROID__)
		// Android Vulkan initialization is handled in APP_CMD_INIT_WINDOW event
		initVulkan(enableValidation);
	#endif
}

vulkanApp::~vulkanApp()
{
	// Clean up Vulkan resources
	swapChain.cleanup();
	if (descriptorPool != VK_NULL_HANDLE) {
		//vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		device.destroyDescriptorPool(descriptorPool, nullptr);
	}
	if (setupCmdBuffer != VK_NULL_HANDLE) {
		//vkFreeCommandBuffers(device, cmdPool, 1, &setupCmdBuffer);
		device.freeCommandBuffers(cmdPool, 1, &setupCmdBuffer);
	}
	destroyCommandBuffers();
	//vkDestroyRenderPass(device, renderPass, nullptr);
	device.destroyRenderPass(renderPass, nullptr);

	for (uint32_t i = 0; i < frameBuffers.size(); i++) {
		//vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
		device.destroyFramebuffer(frameBuffers[i], nullptr);
	}

	for (auto& shaderModule : shaderModules) {
		//vkDestroyShaderModule(device, shaderModule, nullptr);
		device.destroyShaderModule(shaderModule, nullptr);
	}

	//vkDestroyImageView(device, depthStencil.view, nullptr);
	device.destroyImageView(depthStencil.view, nullptr);
	//vkDestroyImage(device, depthStencil.image, nullptr);
	device.destroyImage(depthStencil.image, nullptr);
	//vkFreeMemory(device, depthStencil.mem, nullptr);
	device.freeMemory(depthStencil.mem, nullptr);

	//vkDestroyPipelineCache(device, pipelineCache, nullptr);
	device.destroyPipelineCache(pipelineCache, nullptr);

	if (textureLoader) {
		delete textureLoader;
	}

	//vkDestroyCommandPool(device, cmdPool, nullptr);
	device.destroyCommandPool(cmdPool, nullptr);

	//vkDestroySemaphore(device, semaphores.presentComplete, nullptr);
	device.destroySemaphore(semaphores.presentComplete, nullptr);
	//vkDestroySemaphore(device, semaphores.renderComplete, nullptr);
	device.destroySemaphore(semaphores.renderComplete, nullptr);
	//vkDestroySemaphore(device, semaphores.textOverlayComplete, nullptr);
	device.destroySemaphore(semaphores.textOverlayComplete, nullptr);

	if (enableTextOverlay) {
		delete textOverlay;
	}

	delete vulkanDevice;

	if (enableValidation) {
		vkDebug::freeDebugCallback(instance);
	}

	//vkDestroyInstance(instance, nullptr);
	instance.destroy(nullptr);

	#if defined(__linux)
		#if defined(__ANDROID__)
			// todo : android cleanup (if required)
		#else
			xcb_destroy_window(connection, window);
			xcb_disconnect(connection);
		#endif
	#endif
}

void vulkanApp::initVulkan(bool enableValidation)
{
	vk::Result err;

	// Vulkan instance
	err = createInstance(enableValidation);
	if ((bool)err) {
		vkx::exitFatal("Could not create Vulkan instance : \n" + vkx::errorString(err), "Fatal error");
	}

	#if defined(__ANDROID__)
		loadVulkanFunctions(instance);
	#endif

	// If requested, we enable the default validation layers for debugging
	if (enableValidation) {
		// The report flags determine what type of messages for the layers will be displayed
		// For validating (debugging) an appplication the error and warning bits should suffice
		vk::DebugReportFlagsEXT debugReportFlags = vk::DebugReportFlagBitsEXT::eError; // | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning;
		// Additional flags include performance info, loader and layer debug messages, etc.
		vkDebug::setupDebugging(instance, debugReportFlags, VK_NULL_HANDLE);
	}

	// Physical device
	uint32_t gpuCount = 0;
	// Get number of available physical devices
	//VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
	instance.enumeratePhysicalDevices(&gpuCount, nullptr);

	assert(gpuCount > 0);
	// Enumerate devices
	std::vector<vk::PhysicalDevice> physicalDevices(gpuCount);
	//err = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
	err = instance.enumeratePhysicalDevices(&gpuCount, physicalDevices.data());
	if ((bool)err) {
		vkx::exitFatal("Could not enumerate phyiscal devices : \n" + vkx::errorString(err), "Fatal error");
	}

	// Note :
	// This example will always use the first physical device reported,
	// change the vector index if you have multiple Vulkan devices installed
	// and want to use another one
	physicalDevice = physicalDevices[0];

	// Vulkan device creation
	// This is handled by a separate class that gets a logical device representation
	// and encapsulates functions related to a device
	vulkanDevice = new vkx::VulkanDevice(physicalDevice);
	//VK_CHECK_RESULT(vulkanDevice->createLogicalDevice(enabledFeatures));
	vulkanDevice->createLogicalDevice(enabledFeatures);

	device = vulkanDevice->logicalDevice;

	// todo: remove
	// Store properties (including limits) and features of the phyiscal device
	// So examples can check against them and see if a feature is actually supported

	//vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	physicalDevice.getProperties(&deviceProperties);

	//vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	physicalDevice.getFeatures(&deviceFeatures);
	// Gather physical device memory properties
	//vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
	physicalDevice.getMemoryProperties(&deviceMemoryProperties);
	// Get a graphics queue from the device
	//vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0, &queue);
	device.getQueue(vulkanDevice->queueFamilyIndices.graphics, 0, &queue);

	// Find a suitable depth format
	//vk::Bool32 validDepthFormat = vkx::getSupportedDepthFormat(physicalDevice, &depthFormat);
	//assert(validDepthFormat);//important

	swapChain.connect(instance, physicalDevice, device);

	// Create synchronization objects
	vk::SemaphoreCreateInfo semaphoreCreateInfo;
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queu
	//VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete));
	device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphores.presentComplete);
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	//VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete));
	device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphores.renderComplete);
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands for the text overlay have been sumbitted and executed
	// Will be inserted after the render complete semaphore if the text overlay is enabled
	//VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.textOverlayComplete));
	device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphores.textOverlayComplete);

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	submitInfo = vk::SubmitInfo{};
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semaphores.renderComplete;
}


// if using SDL2
#if USE_SDL2

	/* CROSS PLATFORM */

	void vulkanApp::setupWindow()
	{

		SDL_Window *mySDLWindow;
		SDL_SysWMinfo windowInfo;
		std::string windowTitle = "test";

		SDL_Init(SDL_INIT_EVERYTHING);

		mySDLWindow = SDL_CreateWindow(
			windowTitle.c_str(),
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			800,// width
			600,// height
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
		);

		SDL_VERSION(&windowInfo.version); // initialize info structure with SDL version info

		if (SDL_GetWindowWMInfo(mySDLWindow, &windowInfo)) {
			std::cout << "SDL window initialization success." << std::endl;
		}
		else {
			std::cout << "ERROR!: SDL window init: Couldn't get window information: " << SDL_GetError() << std::endl;
		}

		/* WINDOWS */
		#if defined(_WIN32)
			// get hinstance from sdl window
			HINSTANCE SDLhinstance = GetModuleHandle(NULL);// not sure how this works? (gets current window?) might limit number of windows later
			this->windowInstance = SDLhinstance;

			// get window handle from sdl window
			HWND SDLhandle = windowInfo.info.win.window;
			this->windowHandle = SDLhandle;

		#elif defined(__linux__)
			// TODO
		#endif

		// not really used for init anymore, kept bc will prob be needed for input
		this->SDLWindow = mySDLWindow;
	}

	// handle input
	void vulkanApp::handleInput()
	{
		SDL_Event test_event;
		while (SDL_PollEvent(&test_event)) {
			if (test_event.type == SDL_QUIT) {
				SDL_DestroyWindow(this->SDLWindow);
				SDL_Quit();
			}
		}
	}
	
	/* WINDOWS */
	#if defined(_WIN32)
		// Win32 : Sets up a console window and redirects standard output to it
		void vulkanApp::setupConsole(std::string title)
		{
			AllocConsole();
			AttachConsole(GetCurrentProcessId());
			FILE *stream;
			freopen_s(&stream, "CONOUT$", "w+", stdout);
			SetConsoleTitle(TEXT(title.c_str()));
		}
	/* LINUX */
	#elif defined(__linux__)

	#endif
// if not, make an os specific window
/* WINDOWS */
#elif defined(_WIN32)
	// Win32 : Sets up a console window and redirects standard output to it
	void vulkanApp::setupConsole(std::string title)
	{
		AllocConsole();
		AttachConsole(GetCurrentProcessId());
		FILE *stream;
		freopen_s(&stream, "CONOUT$", "w+", stdout);
		SetConsoleTitle(TEXT(title.c_str()));
	}

	HWND vulkanApp::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)
	{
		this->windowInstance = hinstance;

		bool fullscreen = false;

		// Check command line arguments
		for (int32_t i = 0; i < __argc; i++) {
			if (__argv[i] == std::string("-fullscreen")) {
				fullscreen = true;
			}
		}

		WNDCLASSEX wndClass;

		wndClass.cbSize = sizeof(WNDCLASSEX);
		wndClass.style = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc = wndproc;
		wndClass.cbClsExtra = 0;
		wndClass.cbWndExtra = 0;
		wndClass.hInstance = hinstance;
		wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wndClass.lpszMenuName = NULL;
		wndClass.lpszClassName = name.c_str();
		wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

		if (!RegisterClassEx(&wndClass)) {
			std::cout << "Could not register window class!\n";
			fflush(stdout);
			exit(1);
		}

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		if (fullscreen) {
			DEVMODE dmScreenSettings;
			memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
			dmScreenSettings.dmSize = sizeof(dmScreenSettings);
			dmScreenSettings.dmPelsWidth = screenWidth;
			dmScreenSettings.dmPelsHeight = screenHeight;
			dmScreenSettings.dmBitsPerPel = 32;
			dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

			if ((width != screenWidth) && (height != screenHeight)) {
				if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
					if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
						fullscreen = FALSE;
					} else {
						return FALSE;
					}
				}
			}

		}

		DWORD dwExStyle;
		DWORD dwStyle;

		if (fullscreen) {
			dwExStyle = WS_EX_APPWINDOW;
			dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		} else {
			dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
			dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		}

		RECT windowRect;
		windowRect.left = 0L;
		windowRect.top = 0L;
		windowRect.right = fullscreen ? (long)screenWidth : (long)width;
		windowRect.bottom = fullscreen ? (long)screenHeight : (long)height;

		AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

		std::string windowTitle = getWindowTitle();
		windowHandle = CreateWindowEx(0,
			name.c_str(),
			windowTitle.c_str(),
			dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0,
			0,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			NULL,
			NULL,
			hinstance,
			NULL);

		if (!fullscreen) {
			// Center on screen
			uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
			uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
			SetWindowPos(windowHandle, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}

		if (!windowHandle) {
			printf("Could not create window!\n");
			fflush(stdout);
			return 0;
			exit(1);
		}

		ShowWindow(windowHandle, SW_SHOW);
		SetForegroundWindow(windowHandle);
		SetFocus(windowHandle);

		return windowHandle;
	}

	void vulkanApp::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CLOSE:
			prepared = false;
			DestroyWindow(hWnd);
			PostQuitMessage(0);
			break;
		case WM_PAINT:
			ValidateRect(windowHandle, NULL);
			break;
		case WM_KEYDOWN:
			switch (wParam)
			{
			case KEY_P:
				paused = !paused;
				break;
			case KEY_F1:
				if (enableTextOverlay)
				{
					textOverlay->visible = !textOverlay->visible;
				}
				break;
			case KEY_ESCAPE:
				PostQuitMessage(0);
				break;
			}

			if (camera.firstperson)
			{
				switch (wParam)
				{
				case KEY_W:
					camera.keys.up = true;
					break;
				case KEY_S:
					camera.keys.down = true;
					break;
				case KEY_A:
					camera.keys.left = true;
					break;
				case KEY_D:
					camera.keys.right = true;
					break;
				}
			}

			keyPressed((uint32_t)wParam);
			break;
		case WM_KEYUP:
			if (camera.firstperson)
			{
				switch (wParam)
				{
				case KEY_W:
					camera.keys.up = false;
					break;
				case KEY_S:
					camera.keys.down = false;
					break;
				case KEY_A:
					camera.keys.left = false;
					break;
				case KEY_D:
					camera.keys.right = false;
					break;
				}
			}
			break;
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
			mousePos.x = (float)LOWORD(lParam);
			mousePos.y = (float)HIWORD(lParam);
			break;
		case WM_MOUSEWHEEL:
		{
			short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			zoom += (float)wheelDelta * 0.005f * zoomSpeed;
			camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f * zoomSpeed));
			viewUpdated = true;
			break;
		}
		case WM_MOUSEMOVE:
			if (wParam & MK_RBUTTON)
			{
				int32_t posx = LOWORD(lParam);
				int32_t posy = HIWORD(lParam);
				zoom += (mousePos.y - (float)posy) * .005f * zoomSpeed;
				camera.translate(glm::vec3(-0.0f, 0.0f, (mousePos.y - (float)posy) * .005f * zoomSpeed));
				mousePos = glm::vec2((float)posx, (float)posy);
				viewUpdated = true;
			}
			if (wParam & MK_LBUTTON)
			{
				int32_t posx = LOWORD(lParam);
				int32_t posy = HIWORD(lParam);
				rotation.x += (mousePos.y - (float)posy) * 1.25f * rotationSpeed;
				rotation.y -= (mousePos.x - (float)posx) * 1.25f * rotationSpeed;

				glm::vec3 vrot((mousePos.y - (float)posy) * camera.rotationSpeed, -(mousePos.x - (float)posx) * camera.rotationSpeed, 0.0f);
				glm::quat qrot(vrot);
				camera.rotate(qrot);
				//camera.rotate(vrot);

				mousePos = glm::vec2((float)posx, (float)posy);
				viewUpdated = true;
			}
			if (wParam & MK_MBUTTON)
			{
				int32_t posx = LOWORD(lParam);
				int32_t posy = HIWORD(lParam);
				cameraPos.x -= (mousePos.x - (float)posx) * 0.01f;
				cameraPos.y -= (mousePos.y - (float)posy) * 0.01f;
				camera.translate(glm::vec3(-(mousePos.x - (float)posx) * 0.01f, -(mousePos.y - (float)posy) * 0.01f, 0.0f));
				viewUpdated = true;
				mousePos.x = (float)posx;
				mousePos.y = (float)posy;
			}
			break;
		case WM_SIZE:
			if ((prepared) && (wParam != SIZE_MINIMIZED))
			{
				destWidth = LOWORD(lParam);
				destHeight = HIWORD(lParam);
				if ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_MINIMIZED))
				{
					windowResize();
				}
			}
			break;
		case WM_EXITSIZEMOVE:
			if ((prepared) && ((destWidth != width) || (destHeight != height)))
			{
				windowResize();
			}
			break;
		}
	}

/* LINUX */
#elif defined(__linux__)
	// Set up a window using XCB and request event types
	xcb_window_t vulkanApp::setupWindow()
	{
		uint32_t value_mask, value_list[32];

		window = xcb_generate_id(connection);

		value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
		value_list[0] = screen->black_pixel;
		value_list[1] =
			XCB_EVENT_MASK_KEY_RELEASE |
			XCB_EVENT_MASK_KEY_PRESS |
			XCB_EVENT_MASK_EXPOSURE |
			XCB_EVENT_MASK_STRUCTURE_NOTIFY |
			XCB_EVENT_MASK_POINTER_MOTION |
			XCB_EVENT_MASK_BUTTON_PRESS |
			XCB_EVENT_MASK_BUTTON_RELEASE;

		xcb_create_window(connection,
			XCB_COPY_FROM_PARENT,
			window, screen->root,
			0, 0, width, height, 0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			screen->root_visual,
			value_mask, value_list);

		/* Magic code that will send notification when window is destroyed */
		xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
		xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie, 0);

		xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
		atom_wm_delete_window = xcb_intern_atom_reply(connection, cookie2, 0);

		xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
			window, (*reply).atom, 4, 32, 1,
			&(*atom_wm_delete_window).atom);

		std::string windowTitle = getWindowTitle();
		xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
			window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
			title.size(), windowTitle.c_str());

		free(reply);

		xcb_map_window(connection, window);

		return(window);
	}

	// Initialize XCB connection
	void vulkanApp::initxcbConnection()
	{
		const xcb_setup_t *setup;
		xcb_screen_iterator_t iter;
		int scr;

		connection = xcb_connect(NULL, &scr);
		if (connection == NULL) {
			printf("Could not find a compatible Vulkan ICD!\n");
			fflush(stdout);
			exit(1);
		}

		setup = xcb_get_setup(connection);
		iter = xcb_setup_roots_iterator(setup);
		while (scr-- > 0)
			xcb_screen_next(&iter);
		screen = iter.data;
	}

	void vulkanApp::handleEvent(const xcb_generic_event_t *event)
	{
		switch (event->response_type & 0x7f)
		{
		case XCB_CLIENT_MESSAGE:
			if ((*(xcb_client_message_event_t*)event).data.data32[0] ==
				(*atom_wm_delete_window).atom) {
				quit = true;
			}
			break;
		case XCB_MOTION_NOTIFY:
		{
			xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
			if (mouseButtons.left)
			{
				rotation.x += (mousePos.y - (float)motion->event_y) * 1.25f;
				rotation.y -= (mousePos.x - (float)motion->event_x) * 1.25f;
				camera.rotate(glm::vec3((mousePos.y - (float)motion->event_y) * camera.rotationSpeed, -(mousePos.x - (float)motion->event_x) * camera.rotationSpeed, 0.0f));
				viewUpdated = true;
			}
			if (mouseButtons.right)
			{
				zoom += (mousePos.y - (float)motion->event_y) * .005f;
				camera.translate(glm::vec3(-0.0f, 0.0f, (mousePos.y - (float)motion->event_y) * .005f * zoomSpeed));
				viewUpdated = true;
			}
			if (mouseButtons.middle)
			{
				cameraPos.x -= (mousePos.x - (float)motion->event_x) * 0.01f;
				cameraPos.y -= (mousePos.y - (float)motion->event_y) * 0.01f;
				camera.translate(glm::vec3(-(mousePos.x - (float)(float)motion->event_x) * 0.01f, -(mousePos.y - (float)motion->event_y) * 0.01f, 0.0f));
				viewUpdated = true;
				mousePos.x = (float)motion->event_x;
				mousePos.y = (float)motion->event_y;
			}
			mousePos = glm::vec2((float)motion->event_x, (float)motion->event_y);
		}
		break;
		case XCB_BUTTON_PRESS:
		{
			xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
			if (press->detail == XCB_BUTTON_INDEX_1)
				mouseButtons.left = true;
			if (press->detail == XCB_BUTTON_INDEX_2)
				mouseButtons.middle = true;
			if (press->detail == XCB_BUTTON_INDEX_3)
				mouseButtons.right = true;
		}
		break;
		case XCB_BUTTON_RELEASE:
		{
			xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
			if (press->detail == XCB_BUTTON_INDEX_1)
				mouseButtons.left = false;
			if (press->detail == XCB_BUTTON_INDEX_2)
				mouseButtons.middle = false;
			if (press->detail == XCB_BUTTON_INDEX_3)
				mouseButtons.right = false;
		}
		break;
		case XCB_KEY_PRESS:
		{
			const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
			switch (keyEvent->detail)
			{
				case KEY_W:
					camera.keys.up = true;
					break;
				case KEY_S:
					camera.keys.down = true;
					break;
				case KEY_A:
					camera.keys.left = true;
					break;
				case KEY_D:
					camera.keys.right = true;
					break;
				case KEY_P:
					paused = !paused;
					break;
				case KEY_F1:
					if (enableTextOverlay)
					{
						textOverlay->visible = !textOverlay->visible;
					}
					break;				
			}
		}
		break;	
		case XCB_KEY_RELEASE:
		{
			const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
			switch (keyEvent->detail)
			{
				case KEY_W:
					camera.keys.up = false;
					break;
				case KEY_S:
					camera.keys.down = false;
					break;
				case KEY_A:
					camera.keys.left = false;
					break;
				case KEY_D:
					camera.keys.right = false;
					break;			
				case KEY_ESCAPE:
					quit = true;
					break;
			}
			keyPressed(keyEvent->detail);
		}
		break;
		case XCB_DESTROY_NOTIFY:
			quit = true;
			break;
		case XCB_CONFIGURE_NOTIFY:
		{
			const xcb_configure_notify_event_t *cfgEvent = (const xcb_configure_notify_event_t *)event;
			if ((prepared) && ((cfgEvent->width != width) || (cfgEvent->height != height)))
			{
					destWidth = cfgEvent->width;
					destHeight = cfgEvent->height;
					if ((destWidth > 0) && (destHeight > 0))
					{
						windowResize();
					}
			}
		}
		break;
		default:
			break;
		}
	}

/* ANDROID */
#elif defined(__ANDROID__)
	int32_t vulkanApp::handleAppInput(struct android_app* app, AInputEvent* event)
	{
		vulkanApp* vulkanExample = reinterpret_cast<vulkanApp*>(app->userData);
		if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
		{
			if (AInputEvent_getSource(event) == AINPUT_SOURCE_JOYSTICK)
			{
				// Left thumbstick
				vulkanExample->gamePadState.axisLeft.x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0);
				vulkanExample->gamePadState.axisLeft.y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0);
				// Right thumbstick
				vulkanExample->gamePadState.axisRight.x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, 0);
				vulkanExample->gamePadState.axisRight.y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, 0);
			}
			else
			{
				// todo : touch input
			}
			return 1;
		}

		if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
		{
			int32_t keyCode = AKeyEvent_getKeyCode((const AInputEvent*)event);
			int32_t action = AKeyEvent_getAction((const AInputEvent*)event);
			int32_t button = 0;

			if (action == AKEY_EVENT_ACTION_UP)
				return 0;

			switch (keyCode)
			{
			case AKEYCODE_BUTTON_A:
				vulkanExample->keyPressed(GAMEPAD_BUTTON_A);
				break;
			case AKEYCODE_BUTTON_B:
				vulkanExample->keyPressed(GAMEPAD_BUTTON_B);
				break;
			case AKEYCODE_BUTTON_X:
				vulkanExample->keyPressed(GAMEPAD_BUTTON_X);
				break;
			case AKEYCODE_BUTTON_Y:
				vulkanExample->keyPressed(GAMEPAD_BUTTON_Y);
				break;
			case AKEYCODE_BUTTON_L1:
				vulkanExample->keyPressed(GAMEPAD_BUTTON_L1);
				break;
			case AKEYCODE_BUTTON_R1:
				vulkanExample->keyPressed(GAMEPAD_BUTTON_R1);
				break;
			case AKEYCODE_BUTTON_START:
				vulkanExample->paused = !vulkanExample->paused;
				break;
			};

			LOGD("Button %d pressed", keyCode);
		}

		return 0;
	}

	void vulkanApp::handleAppCommand(android_app * app, int32_t cmd)
	{
		assert(app->userData != NULL);
		vulkanApp* vulkanExample = reinterpret_cast<vulkanApp*>(app->userData);
		switch (cmd)
		{
		case APP_CMD_SAVE_STATE:
			LOGD("APP_CMD_SAVE_STATE");
			/*
			vulkanExample->app->savedState = malloc(sizeof(struct saved_state));
			*((struct saved_state*)vulkanExample->app->savedState) = vulkanExample->state;
			vulkanExample->app->savedStateSize = sizeof(struct saved_state);
			*/
			break;
		case APP_CMD_INIT_WINDOW:
			LOGD("APP_CMD_INIT_WINDOW");
			if (vulkanExample->androidApp->window != NULL)
			{
				vulkanExample->initVulkan(false);
				vulkanExample->initSwapchain();
				vulkanExample->prepare();
				assert(vulkanExample->prepared);
			}
			else
			{
				LOGE("No window assigned!");
			}
			break;
		case APP_CMD_LOST_FOCUS:
			LOGD("APP_CMD_LOST_FOCUS");
			vulkanExample->focused = false;
			break;
		case APP_CMD_GAINED_FOCUS:
			LOGD("APP_CMD_GAINED_FOCUS");
			vulkanExample->focused = true;
			break;
		}
	}
#endif

void vulkanApp::viewChanged()
{
	// Can be overrdiden in derived class
}

void vulkanApp::keyPressed(uint32_t keyCode)
{
	// Can be overriden in derived class
}

void vulkanApp::buildCommandBuffers()
{
	// Can be overriden in derived class
}

void vulkanApp::createCommandPool()
{
	vk::CommandPoolCreateInfo cmdPoolInfo;
	cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
	cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	//VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
	device.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool);
}

void vulkanApp::setupDepthStencil()
{

	vk::ImageCreateInfo image;
	//image.flags = 0;
	image.pNext = NULL;
	image.imageType = vk::ImageType::e2D;
	image.format = depthFormat;
	image.format = vk::Format::eR8G8B8A8Unorm;

	image.extent = vk::Extent3D{ width, height, 1 };
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = vk::SampleCountFlagBits::e1;
	image.tiling = vk::ImageTiling::eOptimal;
	image.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	

	vk::MemoryAllocateInfo mem_alloc;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	vk::ImageViewCreateInfo depthStencilView;
	depthStencilView.pNext = NULL;
	depthStencilView.viewType = vk::ImageViewType::e2D;
	depthStencilView.format = depthFormat;
	depthStencilView.format = vk::Format::eR8G8B8A8Unorm;
	//depthStencilView.flags = 0;
	depthStencilView.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	vk::MemoryRequirements memReqs;

	//VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &depthStencil.image));
	device.createImage(&image, nullptr, &depthStencil.image);

	//vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
	device.getImageMemoryRequirements(depthStencil.image, &memReqs);

	mem_alloc.allocationSize = memReqs.size;
	mem_alloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	//VK_CHECK_RESULT(vkAllocateMemory(device, &mem_alloc, nullptr, &depthStencil.mem));
	device.allocateMemory(&mem_alloc, nullptr, &depthStencil.mem);
	//VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0));
	device.bindImageMemory(depthStencil.image, depthStencil.mem, 0);

	depthStencilView.image = depthStencil.image;
	//VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &depthStencil.view));
	device.createImageView(&depthStencilView, nullptr, &depthStencil.view);
}

void vulkanApp::setupFrameBuffer()
{
	vk::ImageView attachments[2];

	// Depth/Stencil attachment is the same for all frame buffers
	attachments[1] = depthStencil.view;

	vk::FramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = renderPass;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	frameBuffers.resize(swapChain.imageCount);
	for (uint32_t i = 0; i < frameBuffers.size(); i++)
	{
		attachments[0] = swapChain.buffers[i].view;
		//VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
		device.createFramebuffer(&frameBufferCreateInfo, nullptr, &frameBuffers[i]);
	}
}

void vulkanApp::setupRenderPass()
{
	std::array<vk::AttachmentDescription, 2> attachments = {};
	// Color attachment
	attachments[0].format = colorformat;
	attachments[0].samples = vk::SampleCountFlagBits::e1;
	attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
	attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
	attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[0].initialLayout = vk::ImageLayout::eUndefined;
	attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;
	// Depth attachment
	attachments[1].format = depthFormat;
	attachments[1].samples = vk::SampleCountFlagBits::e1;
	attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
	attachments[1].storeOp = vk::AttachmentStoreOp::eStore;
	attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[1].initialLayout = vk::ImageLayout::eUndefined;
	attachments[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference colorReference;
	colorReference.attachment = 0;
	colorReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference depthReference;
	depthReference.attachment = 1;
	depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::SubpassDescription subpassDescription;
	subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;
	subpassDescription.pResolveAttachments = nullptr;

	// Subpass dependencies for layout transitions
	std::array<vk::SubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
	dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
	dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	//VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
	device.createRenderPass(&renderPassInfo, nullptr, &renderPass);
}

void vulkanApp::windowResize()
{
	if (!prepared) {
		return;
	}
	prepared = false;

	// Recreate swap chain
	width = destWidth;
	height = destHeight;
	createSetupCommandBuffer();
	setupSwapChain();

	// Recreate the frame buffers

	//vkDestroyImageView(device, depthStencil.view, nullptr);
	device.destroyImageView(depthStencil.view, nullptr);
	//vkDestroyImage(device, depthStencil.image, nullptr);
	device.destroyImage(depthStencil.image, nullptr);
	//vkFreeMemory(device, depthStencil.mem, nullptr);
	device.freeMemory(depthStencil.mem, nullptr);

	setupDepthStencil();
	
	for (uint32_t i = 0; i < frameBuffers.size(); i++) {
		//vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
		device.destroyFramebuffer(frameBuffers[i], nullptr);
	}
	setupFrameBuffer();

	flushSetupCommandBuffer();

	// Command buffers need to be recreated as they may store
	// references to the recreated frame buffer
	destroyCommandBuffers();
	createCommandBuffers();
	buildCommandBuffers();

	//vkQueueWaitIdle(queue);
	queue.waitIdle();
	//vkDeviceWaitIdle(device);
	device.waitIdle();

	if (enableTextOverlay)
	{
		textOverlay->reallocateCommandBuffers();
		updateTextOverlay();
	}

	camera.updateAspectRatio((float)width / (float)height);

	// Notify derived class
	windowResized();
	viewChanged();

	prepared = true;
}

void vulkanApp::windowResized()
{
	// Can be overriden in derived class
}

void vulkanApp::initSwapchain()
{
	//// if using SDL2
	//#if USE_SDL2
	//	/* WINDOWS*/
	//	#if defined(_WIN32)
	//		swapChain.initSurface(this->windowInstance, this->SDLWindow);
	//	/* LINUX */
	//	#elif defined(__linux__)
	//	
	//	/* ANDROID */
	//	#elif defined(__ANDROID__)
	//	
	//	#endif
	//#endif

	/* OS specific surface creation */
	#if defined(_WIN32)
		swapChain.initSurface(this->windowInstance, this->windowHandle);
	#elif defined(__linux__)
		swapChain.initSurface(connection, window);
	#elif defined(__ANDROID__)	
		swapChain.initSurface(androidApp->window);
	#endif
}

void vulkanApp::setupSwapChain()
{
	swapChain.create(&width, &height, enableVSync);
}
