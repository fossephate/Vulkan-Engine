/*
* Vulkan Example base class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "vulkanExampleBase.h"

using namespace vkx;

ExampleBase::ExampleBase(bool enableValidation) : swapChain(*this) {
    // Check for validation command line flag
#if defined(_WIN32)
    for (int32_t i = 0; i < __argc; i++) {
        if (__argv[i] == std::string("-validation")) {
            enableValidation = true;
        }
    }
#elif defined(__ANDROID__)
    // Vulkan library is loaded dynamically on Android
    bool libLoaded = loadVulkanLibrary();
    assert(libLoaded);
#endif

#if !defined(__ANDROID__)
    // Android Vulkan initialization is handled in APP_CMD_INIT_WINDOW event
    initVulkan(enableValidation);
#endif
}

ExampleBase::~ExampleBase() {
    // Clean up Vulkan resources
    swapChain.cleanup();
    if (descriptorPool) {
        device.destroyDescriptorPool(descriptorPool);
    }
    if (!primaryCmdBuffers.empty()) {
        device.freeCommandBuffers(cmdPool, primaryCmdBuffers);
        primaryCmdBuffers.clear();
    }
    if (!drawCmdBuffers.empty()) {
        device.freeCommandBuffers(cmdPool, drawCmdBuffers);
        drawCmdBuffers.clear();
    }
    if (!textCmdBuffers.empty()) {
        device.freeCommandBuffers(cmdPool, textCmdBuffers);
        textCmdBuffers.clear();
    }
    device.destroyRenderPass(renderPass);
    for (uint32_t i = 0; i < framebuffers.size(); i++) {
        device.destroyFramebuffer(framebuffers[i]);
    }

    for (auto& shaderModule : shaderModules) {
        device.destroyShaderModule(shaderModule);
    }
    depthStencil.destroy();

    if (textureLoader) {
        delete textureLoader;
    }

    if (enableTextOverlay) {
        delete textOverlay;
    }

    device.destroySemaphore(semaphores.acquireComplete);
    device.destroySemaphore(semaphores.renderComplete);

    destroyContext();

#if defined(__ANDROID__)
    // todo : android cleanup (if required)
#else
    glfwDestroyWindow(window);
    glfwTerminate();
#endif
}


void ExampleBase::run() {
#if defined(_WIN32)
    setupWindow();
#elif defined(__ANDROID__)
    // Attach vulkan example to global android application state
    state->userData = vulkanExample;
    state->onAppCmd = VulkanExample::handleAppCommand;
    state->onInputEvent = VulkanExample::handleAppInput;
    androidApp = state;
#elif defined(__linux__)
    setupWindow();
#endif
#if !defined(__ANDROID__)
    prepare();
#endif
    renderLoop();

    // Once we exit the render loop, wait for everything to become idle before proceeding to the descructor.
    queue.waitIdle();
    device.waitIdle();
}

void ExampleBase::initVulkan(bool enableValidation) {
    createContext(enableValidation);
    // Find a suitable depth format
    depthFormat = getSupportedDepthFormat(physicalDevice);

    // Create synchronization objects
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    // Create a semaphore used to synchronize image presentation
    // Ensures that the image is displayed before we start submitting new commands to the queu
    semaphores.acquireComplete = device.createSemaphore(semaphoreCreateInfo);
    // Create a semaphore used to synchronize command submission
    // Ensures that the image is not presented until all commands have been sumbitted and executed
    semaphores.renderComplete = device.createSemaphore(semaphoreCreateInfo);

    // Set up submit info structure
    // Semaphores will stay the same during application lifetime
    // Command buffer submission info is set by each example
    submitInfo = vk::SubmitInfo();
    submitInfo.pWaitDstStageMask = &submitPipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores.acquireComplete;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores.renderComplete;
}

void ExampleBase::renderLoop() {
#if defined(__ANDROID__)
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
            // Convert to clamped timer value
            if (!paused) {
                timer += timerSpeed * frameTimer;
                if (timer > 1.0) {
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
        }
    }
#else
    auto tStart = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window)) {
        auto tEnd = std::chrono::high_resolution_clock::now();
        auto tDiff = std::chrono::duration<float, std::milli>(tEnd - tStart).count();
        auto tDiffSeconds = tDiff / 1000.0f;
        tStart = tEnd;
        glfwPollEvents();

        if (glfwJoystickPresent(0)) {
            // FIXME implement joystick handling
            int axisCount{ 0 };
            const float* axes = glfwGetJoystickAxes(0, &axisCount);
            if (axisCount >= 2) {
                gamePadState.axes.x = axes[0] * 0.01f;
                gamePadState.axes.y = axes[1] * -0.01f;
            }
            if (axisCount >= 4) {
            }
            if (axisCount >= 6) {
                float lt = (axes[4] + 1.0f) / 2.0f;
                float rt = (axes[5] + 1.0f) / 2.0f;
                gamePadState.axes.rz = (rt - lt);
            }
            uint32_t newButtons{ 0 };
            static uint32_t oldButtons{ 0 };
            {
                int buttonCount{ 0 };
                const uint8_t* buttons = glfwGetJoystickButtons(0, &buttonCount);
                for (uint8_t i = 0; i < buttonCount && i < 64; ++i) {
                    if (buttons[i]) {
                        newButtons |= (1 << i);
                    }
                }
            }
            auto changedButtons = newButtons & ~oldButtons;
            if (changedButtons & 0x01) {
                keyPressed(GAMEPAD_BUTTON_A);
            }
            if (changedButtons & 0x02) {
                keyPressed(GAMEPAD_BUTTON_B);
            }
            if (changedButtons & 0x04) {
                keyPressed(GAMEPAD_BUTTON_X);
            }
            if (changedButtons & 0x08) {
                keyPressed(GAMEPAD_BUTTON_Y);
            }
            if (changedButtons & 0x10) {
                keyPressed(GAMEPAD_BUTTON_L1);
            }
            if (changedButtons & 0x20) {
                keyPressed(GAMEPAD_BUTTON_R1);
            }
            oldButtons = newButtons;
        } else {
            memset(&gamePadState.axes, 0, sizeof(gamePadState.axes));
        }


        render();
        update(tDiffSeconds);

    }
#endif
}

std::string ExampleBase::getWindowTitle() {
    std::string device(deviceProperties.deviceName);
    std::string windowTitle;
    windowTitle = title + " - " + device + " - " + std::to_string(frameCounter) + " fps";
    return windowTitle;
}

const std::string& ExampleBase::getAssetPath() {
    return vkx::getAssetPath();
}

void ExampleBase::prepare() {
    if (enableValidation) {
        debug::setupDebugging(instance, vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning);
    }
    if (enableDebugMarkers) {
        debug::marker::setup(device);
    }
    cmdPool = getCommandPool();

    swapChain.create(size, enableVsync);
    setupDepthStencil();
    setupRenderPass();
    setupRenderPassBeginInfo();
    setupFrameBuffer();

    // Create a simple texture loader class
    textureLoader = new TextureLoader(*this);
#if defined(__ANDROID__)
    textureLoader->assetManager = androidApp->activity->assetManager;
#endif
    if (enableTextOverlay) {
        // Load the text rendering shaders
        textOverlay = new TextOverlay(*this,
            size.width,
            size.height,
            renderPass);
        updateTextOverlay();
    }
}
MeshBuffer ExampleBase::loadMesh(const std::string& filename, const MeshLayout& vertexLayout, float scale) {
    MeshLoader loader;
#if defined(__ANDROID__)
    loader.assetManager = androidApp->activity->assetManager;
#endif
    loader.load(filename);
    assert(loader.m_Entries.size() > 0);
    return loader.createBuffers(*this, vertexLayout, scale);
}

vk::SubmitInfo ExampleBase::prepareSubmitInfo(
    const std::vector<vk::CommandBuffer>& commandBuffers,
    vk::PipelineStageFlags *pipelineStages) {
    vk::SubmitInfo submitInfo;
    submitInfo.pWaitDstStageMask = pipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores.acquireComplete;
    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores.renderComplete;
    return submitInfo;
}

void ExampleBase::updateTextOverlay() {
    if (!enableTextOverlay)
        return;

    textOverlay->beginTextUpdate();
    textOverlay->addText(title, 5.0f, 5.0f, TextOverlay::alignLeft);

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << (frameTimer * 1000.0f) << "ms (" << lastFPS << " fps)";
    textOverlay->addText(ss.str(), 5.0f, 25.0f, TextOverlay::alignLeft);
    textOverlay->addText(deviceProperties.deviceName, 5.0f, 45.0f, TextOverlay::alignLeft);
    getOverlayText(textOverlay);
    textOverlay->endTextUpdate();

    trashCommandBuffers(textCmdBuffers);
    populateSubCommandBuffers(textCmdBuffers, [&](const vk::CommandBuffer& cmdBuffer) {
        textOverlay->writeCommandBuffer(cmdBuffer);
    });
    primaryCmdBuffersDirty = true;
}

void ExampleBase::getOverlayText(vkx::TextOverlay *textOverlay) {
    // Can be overriden in derived class
}

void ExampleBase::prepareFrame() {
    if (primaryCmdBuffersDirty) {
        buildCommandBuffers();
    }
    // Acquire the next image from the swap chaing
    currentBuffer = swapChain.acquireNextImage(semaphores.acquireComplete);
}

void ExampleBase::submitFrame() {
    swapChain.queuePresent(semaphores.renderComplete);
}

#if defined(__ANDROID__)
int32_t ExampleBase::handleAppInput(struct android_app* app, AInputEvent* event) {
    ExampleBase* vulkanExample = (ExampleBase*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        if (AInputEvent_getSource(event) == AINPUT_SOURCE_JOYSTICK) {
            vulkanExample->gamePadState.axes.x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0);
            vulkanExample->gamePadState.axes.y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0);
            vulkanExample->gamePadState.axes.z = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, 0);
            vulkanExample->gamePadState.axes.rz = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, 0);
        } else {
            // todo : touch input
        }
        return 1;
    }

    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        int32_t keyCode = AKeyEvent_getKeyCode((const AInputEvent*)event);
        int32_t action = AKeyEvent_getAction((const AInputEvent*)event);
        int32_t button = 0;

        if (action == AKEY_EVENT_ACTION_UP)
            return 0;

        switch (keyCode) {
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
            vulkanExample->keyPressed(GAMEPAD_BUTTON_START);
            break;
        };

        LOGD("Button %d pressed", keyCode);
    }

    return 0;
}

void ExampleBase::handleAppCommand(android_app * app, int32_t cmd) {
    assert(app->userData != NULL);
    ExampleBase* vulkanExample = (ExampleBase*)app->userData;
    switch (cmd) {
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
        if (vulkanExample->androidApp->window != NULL) {
            vulkanExample->initVulkan(false);
            vulkanExample->initSwapchain();
            vulkanExample->prepare();
            assert(vulkanExample->prepared);
        } else {
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
#else

void ExampleBase::KeyboardHandler(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ExampleBase* example = (ExampleBase*)glfwGetWindowUserPointer(window);
    if (action == GLFW_PRESS) {
        example->keyPressed(key);
    }
}



void ExampleBase::MouseHandler(GLFWwindow* window, int button, int action, int mods) {
    ExampleBase* example = (ExampleBase*)glfwGetWindowUserPointer(window);
    if (action == GLFW_PRESS) {
        glm::dvec2 mousePos; glfwGetCursorPos(window, &mousePos.x, &mousePos.y);
        example->mousePos = mousePos;
    }
}

void ExampleBase::MouseMoveHandler(GLFWwindow* window, double posx, double posy) {
    ExampleBase* example = (ExampleBase*)glfwGetWindowUserPointer(window);
    example->mouseMoved(glm::vec2(posx, posy));
}

void ExampleBase::MouseScrollHandler(GLFWwindow* window, double xoffset, double yoffset) {
    ExampleBase* example = (ExampleBase*)glfwGetWindowUserPointer(window);
    example->mouseScrolled(yoffset);
}


void ExampleBase::CloseHandler(GLFWwindow* window) {
    ExampleBase* example = (ExampleBase*)glfwGetWindowUserPointer(window);
    example->prepared = false;
    glfwSetWindowShouldClose(window, 1);
}

void ExampleBase::FramebufferSizeHandler(GLFWwindow* window, int width, int height) {
    ExampleBase* example = (ExampleBase*)glfwGetWindowUserPointer(window);
    example->windowResize(glm::uvec2(width, height));
}

void ExampleBase::setupWindow() {
    bool fullscreen = false;

#ifdef _WIN32
    // Check command line arguments
    for (int32_t i = 0; i < __argc; i++) {
        if (__argv[i] == std::string("-fullscreen")) {
            fullscreen = true;
        }
    }
#endif

    if (fullscreen) {
        // TODO 
    }

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto monitor = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(monitor);
    size.width = mode->width;
    size.height = mode->height;

    if (fullscreen) {
        window = glfwCreateWindow(size.width, size.height, "My Title", monitor, NULL);
    } else {
        size.width /= 2;
        size.height /= 2;
        window = glfwCreateWindow(size.width, size.height, "Window Title", NULL, NULL);
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, KeyboardHandler);
    glfwSetMouseButtonCallback(window, MouseHandler);
    glfwSetCursorPosCallback(window, MouseMoveHandler);
    glfwSetWindowCloseCallback(window, CloseHandler);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeHandler);
    glfwSetScrollCallback(window, MouseScrollHandler);
    if (!window) {
        throw std::runtime_error("Could not create window");
    }
    swapChain.createSurface(window);
    camera.setAspectRatio(size);
}

#endif

void ExampleBase::setupDepthStencil() {
    depthStencil.destroy();

    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    vk::ImageCreateInfo image;
    image.imageType = vk::ImageType::e2D;
    image.extent = vk::Extent3D{ size.width, size.height, 1 };
    image.format = depthFormat;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc;
    depthStencil = createImage(image, vk::MemoryPropertyFlagBits::eDeviceLocal);

    withPrimaryCommandBuffer([&](const vk::CommandBuffer& setupCmdBuffer) {
        setImageLayout(
            setupCmdBuffer,
            depthStencil.image,
            aspect,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal);
    });


    vk::ImageViewCreateInfo depthStencilView;
    depthStencilView.viewType = vk::ImageViewType::e2D;
    depthStencilView.format = depthFormat;
    depthStencilView.subresourceRange.aspectMask = aspect;
    depthStencilView.subresourceRange.levelCount = 1;
    depthStencilView.subresourceRange.layerCount = 1;
    depthStencilView.image = depthStencil.image;
    depthStencil.view = device.createImageView(depthStencilView);
}

void ExampleBase::setupFrameBuffer() {
    // Recreate the frame buffers
    if (!framebuffers.empty()) {
        for (uint32_t i = 0; i < framebuffers.size(); i++) {
            device.destroyFramebuffer(framebuffers[i]);
        }
        framebuffers.clear();
    }

    vk::ImageView attachments[2];

    // Depth/Stencil attachment is the same for all frame buffers
    attachments[1] = depthStencil.view;

    vk::FramebufferCreateInfo framebufferCreateInfo;
    framebufferCreateInfo.renderPass = renderPass;
    framebufferCreateInfo.attachmentCount = 2;
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.width = size.width;
    framebufferCreateInfo.height = size.height;
    framebufferCreateInfo.layers = 1;

    // Create frame buffers for every swap chain image
    framebuffers = swapChain.createFramebuffers(framebufferCreateInfo);
}

void ExampleBase::setupRenderPass() {
    if (renderPass) {
        device.destroyRenderPass(renderPass);
    }

    std::vector<vk::AttachmentDescription> attachments;
    attachments.resize(2);

    // Color attachment
    attachments[0].format = colorformat;
    attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
    attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
    attachments[0].initialLayout = vk::ImageLayout::eUndefined;
    attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    // Depth attachment
    attachments[1].format = depthFormat;
    attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
    attachments[1].storeOp = vk::AttachmentStoreOp::eDontCare;
    attachments[1].initialLayout = vk::ImageLayout::eUndefined;
    attachments[1].finalLayout = vk::ImageLayout::eUndefined;

    // Only one depth attachment, so put it first in the references
    vk::AttachmentReference depthReference;
    depthReference.attachment = 1;
    depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    std::vector<vk::AttachmentReference> colorAttachmentReferences;
    {
        vk::AttachmentReference colorReference;
        colorReference.attachment = 0;
        colorReference.layout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttachmentReferences.push_back(colorReference);
    }

    std::vector<vk::SubpassDescription> subpasses;
    std::vector<vk::SubpassDependency> subpassDependencies;
    {
        vk::SubpassDependency dependency;
        dependency.srcSubpass = 0;
        dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        subpassDependencies.push_back(dependency);

        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.pDepthStencilAttachment = &depthReference;
        subpass.colorAttachmentCount = colorAttachmentReferences.size();
        subpass.pColorAttachments = colorAttachmentReferences.data();
        subpasses.push_back(subpass);
    }

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = subpasses.size();
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = subpassDependencies.size();
    renderPassInfo.pDependencies = subpassDependencies.data();
    renderPass = device.createRenderPass(renderPassInfo);
}

void ExampleBase::windowResize(const glm::uvec2& newSize) {
    if (!prepared) {
        return;
    }
    prepared = false;

    queue.waitIdle();
    device.waitIdle();

    // Recreate swap chain
    size.width = newSize.x;
    size.height = newSize.y;
    camera.setAspectRatio(size);
    swapChain.create(size, enableVsync);

    setupDepthStencil();
    setupFrameBuffer();
    if (enableTextOverlay && textOverlay->visible) {
        updateTextOverlay();
    }
    setupRenderPassBeginInfo();

    // Notify derived class
    windowResized();

    // Can be overriden in derived class
    updateDrawCommandBuffers();

    // Command buffers need to be recreated as they may store
    // references to the recreated frame buffer
    buildCommandBuffers();

    viewChanged();

    prepared = true;
}

void ExampleBase::windowResized() {}


