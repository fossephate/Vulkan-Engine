/*
* Vulkan Example base class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

// include os specific files
#if defined(_WIN32)
	#pragma comment(linker, "/subsystem:windows")
	#include <windows.h>
	#include <fcntl.h>
	#include <io.h>
#elif defined(__linux__)
	#include <xcb/xcb.h>
#elif defined(__ANDROID__)
	#include <android/native_activity.h>
	#include <android/asset_manager.h>
	#include <android_native_app_glue.h>
	#include "vulkanAndroid.h"
#endif

#include <iostream>
#include <chrono>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <array>

// global
#include "main/global.h"

// load SDL2 if using it
#if USE_SDL2
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_syswm.h>
#endif

#include <vulkan/vulkan.hpp>

#include "keycodes.hpp"
#include "vulkanTools.h"
#include "vulkanDebug2.h"

//#include "vulkanDevice.h"
#include "vulkanContext.h"
#include "vulkanSwapChain.h"
#include "vulkanTextureLoader.h"
#include "vulkanMeshLoader.h"
#include "vulkanTextOverlay.h"
#include "camera.h"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1
#define ENABLE_VALIDATION true

namespace vkx
{

	struct UpdateOperation {
		const vk::Buffer buffer;
		const vk::DeviceSize size;
		const vk::DeviceSize offset;
		const uint32_t* data;

		template <typename T>
		UpdateOperation(const vk::Buffer& buffer, const T& data, vk::DeviceSize offset = 0) : buffer(buffer), size(sizeof(T)), offset(offset), data((uint32_t*)&data) {
			assert(0 == (sizeof(T) % 4));
			assert(0 == (offset % 4));
		}
	};

	class vulkanApp
	{
	private:
		// Set to true when example is created with enabled validation layers
		bool enableValidation{ false };
		// Set to true when the debug marker extension is detected
		bool enableDebugMarkers{ false };

		// fps timer (one second interval)
		float fpsTimer = 0.0f;

		//// Create application wide Vulkan instance
		//vk::Result createInstance(bool enableValidation);

		//// Get window title with example name, device, et.
		std::string getWindowTitle();

		///** brief Indicates that the view (position, rotation) has changed and */
		//bool viewUpdated = false;

		//// Destination dimensions for resizing the window
		//vk::Extent2D destSize{ 1280, 720 };

		//// Called if the window is resized and some resources have to be recreatesd
		//void windowResize(const glm::uvec2& newSize);



	protected:
		bool enableVsync{ false };
		// Command buffers used for rendering
		std::vector<vk::CommandBuffer> primaryCmdBuffers;
		std::vector<vk::CommandBuffer> textCmdBuffers;
		std::vector<vk::CommandBuffer> drawCmdBuffers;
		bool primaryCmdBuffersDirty{ true };
		std::vector<vk::ClearValue> clearValues;
		vk::RenderPassBeginInfo renderPassBeginInfo;


	protected:
		// Last frame time, measured using a high performance timer (if available)
		float frameTimer{ 1.0f };
		// Frame counter to display fps
		uint32_t frameCounter{ 0 };
		uint32_t lastFPS{ 0 };
		std::list<UpdateOperation> pendingUpdates;

		// Color buffer format
		vk::Format colorformat{ vk::Format::eB8G8R8A8Unorm };

		// Depth buffer format...  selected during Vulkan initialization
		vk::Format depthFormat{ vk::Format::eUndefined };

		vk::PipelineStageFlags submitPipelineStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		// Contains command buffers and semaphores to be presented to the queue
		vk::SubmitInfo submitInfo;
		// Global render pass for frame buffer writes
		vk::RenderPass renderPass;
		// List of available frame buffers (same as number of swap chain images)
		std::vector<vk::Framebuffer>framebuffers;
		// Active frame buffer index
		uint32_t currentBuffer = 0;
		// Descriptor set pool
		vk::DescriptorPool descriptorPool;
		// Wraps the swap chain to present images (framebuffers) to the windowing system
		vkx::VulkanSwapChain swapChain;

		// Command buffer pool
		vk::CommandPool cmdPool;

		// set references to context
		vkx::Context context{};
		vk::Device &device = context.device;
		vk::PhysicalDevice &physicalDevice = context.physicalDevice;
		vk::Queue &queue = context.queue;
		std::vector<vk::ShaderModule> &shaderModules = context.shaderModules;
		vk::PipelineCache &pipelineCache = context.pipelineCache;
		vk::Instance &instance = context.instance;
		std::vector<vk::PhysicalDevice> &physicalDevices = context.physicalDevices;
		
		vk::PhysicalDeviceProperties &deviceProperties = context.deviceProperties;
		vk::PhysicalDeviceFeatures &deviceFeatures = context.deviceFeatures;
		vk::PhysicalDeviceMemoryProperties &deviceMemoryProperties = context.deviceMemoryProperties;

		//trashCommandBuffers = context.trashCommandBuffers;
		//void(&trashCommandBuffers)(std::vector<vk::CommandBuffer>& cmdBuffers) = context.trashCommandBuffers;


		// Synchronization semaphores
		struct {
			// Swap chain image presentation
			vk::Semaphore acquireComplete;
			// Command buffer submission and execution
			vk::Semaphore renderComplete;
			vk::Semaphore transferComplete;
		} semaphores;

		// Simple texture loader
		vkx::TextureLoader *textureLoader{ nullptr };

		// Returns the base asset path (for shaders, models, textures) depending on the os
		const std::string& getAssetPath();




	public:

		bool prepared = false;
		vk::Extent2D size{ 1280, 720 };

		vk::ClearColorValue defaultClearColor = std::array<float, 4>{0.025f, 0.025f, 0.025f, 1.0f};

		float zoom = 0;

		// Defines a frame rate independent timer value clamped from -1.0...1.0
		// For use in animations, rotations, etc.
		float timer = 0.0f;
		// Multiplier for speeding up (or slowing down) the global timer
		float timerSpeed = 0.25f;

		bool paused = false;

		bool enableTextOverlay = true;
		vkx::TextOverlay *textOverlay;

		// Use to adjust mouse rotation speed
		float rotationSpeed = 1.0f;
		// Use to adjust mouse zoom speed
		float zoomSpeed = 1.0f;

		Camera camera;

		glm::vec3 rotation = glm::vec3();
		glm::vec3 cameraPos = glm::vec3();
		glm::vec2 mousePos;

		std::string title = "Vulkan Application";
		std::string name = "vulkanApplication";

		CreateImageResult depthStencil;

		enum keyState {down, up};

		struct {
			bool w = false;
			bool s = false;
			bool a = false;
			bool d = false;
			bool q = false;
			bool e = false;
			bool space = false;
			bool up_arrow = false;
			bool down_arrow = false;
			bool left_arrow = false;
			bool right_arrow = false;
			bool shift = false;
			bool i = false;
			bool k = false;
			bool j = false;
			bool l = false;
			bool u = false;
			bool o = false;
		} keyStates;

		glm::vec3 lookAtPoint = glm::vec3(0.0f, 0.0f, 0.0f);// not used

		struct {
			glm::vec2 current;
			glm::vec2 delta;
			bool movedThisFrame = false;
			struct {
				bool state = false;
				glm::vec2 pressedCoords;
				glm::vec2 releasedCoords;
			} leftMouseButton;

			struct {
				bool state = false;
				glm::vec2 pressedCoords;
				glm::vec2 releasedCoords;
			} middleMouseButton;

			struct {
				bool state = false;
				glm::vec2 pressedCoords;
				glm::vec2 releasedCoords;
			} rightMouseButton;
		} mouse;


		// Gamepad state (only one pad supported)
		struct {
			glm::vec2 axisLeft = glm::vec2(0.0f);
			glm::vec2 axisRight = glm::vec2(0.0f);
		} gamePadState;

		#if USE_SDL2
		SDL_Window * SDLWindow;
		SDL_SysWMinfo windowInfo;
		#endif


		// Default constructor
		vulkanApp(bool enableValidation);
		// destructor
		~vulkanApp();

		void run();

		// Setup the vulkan instance, enable required extensions and connect to the physical device (GPU)
		void initVulkan(bool enableValidation);


		virtual void setupWindow();

		// A default draw implementation
		virtual void draw();

		// Pure virtual render function (override in derived class)
		virtual void render();

		virtual void update(float deltaTime);

		// Called when view change occurs
		// Can be overriden in derived class to e.g. update uniform buffers 
		// Containing view dependant matrices
		virtual void viewChanged();

		// Called when the window has been resized
		// Can be overriden in derived class to recreate or rebuild resources attached to the frame buffer / swapchain
		virtual void windowResized();

		// Setup default depth and stencil views
		void setupDepthStencil();
		// Create framebuffers for all requested swap chain images
		// Can be overriden in derived class to setup a custom framebuffer (e.g. for MSAA)
		virtual void setupFrameBuffer();

		// Setup a default render pass
		// Can be overriden in derived class to setup a custom render pass (e.g. for MSAA)
		virtual void setupRenderPass();

		void populateSubCommandBuffers(std::vector<vk::CommandBuffer>& cmdBuffers, std::function<void(const vk::CommandBuffer& commandBuffer)> f);

		virtual void updatePrimaryCommandBuffer(const vk::CommandBuffer& cmdBuffer);

		virtual void updateDrawCommandBuffers() final;


		// Pure virtual function to be overriden by the dervice class
		// Called in case of an event where e.g. the framebuffer has to be rebuild and thus
		// all command buffers that may reference this
		virtual void updateDrawCommandBuffer(const vk::CommandBuffer& drawCommand) = 0;



		void drawCurrentCommandBuffer(const vk::Semaphore& semaphore = vk::Semaphore());



		void executePendingTransfers(vk::Semaphore transferPending);

		virtual void setupRenderPassBeginInfo();

		virtual void buildCommandBuffers();


		


		// Prepare commonly used Vulkan functions
		virtual void prepare();

		// Load a mesh (using ASSIMP) and create vulkan vertex and index buffers with given vertex layout
		vkx::MeshBuffer loadMesh(
			const std::string& filename,
			const vkx::MeshLayout& vertexLayout,
			float scale = 1.0f);


		// Start the main render loop
		void renderLoop();

		// Prepare a submit info structure containing
		// semaphores and submit buffer info for vkQueueSubmit
		vk::SubmitInfo prepareSubmitInfo(
			const std::vector<vk::CommandBuffer>& commandBuffers,
			vk::PipelineStageFlags *pipelineStages);

		void updateTextOverlay();


		// Called when the text overlay is updating
		// Can be overriden in derived class to add custom text to the overlay
		virtual void getOverlayText(vkx::TextOverlay * textOverlay);

		// Prepare the frame for workload submission
		// - Acquires the next image from the swap chain 
		// - Sets the default wait and signal semaphores
		void prepareFrame();

		// Submit the frames' workload 
		// - Submits the text overlay (if enabled)
		void submitFrame();


		

	};

}

//MessageBox(NULL, filename.c_str(), NULL, NULL);