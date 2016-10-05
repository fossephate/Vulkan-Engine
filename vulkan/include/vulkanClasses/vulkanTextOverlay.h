/*
* Text overlay class for displaying debug information
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include <vulkan/vulkan.hpp>
#include "vulkanTools.h"
#include "vulkanDebug.h"
#include "vulkanBuffer.h"
#include "vulkanDevice.h"

#include "../external/stb/stb_font_consolas_24_latin1.inl"

// Defines for the STB font used
// STB font files can be found at http://nothings.org/stb/font/
#define STB_FONT_NAME stb_font_consolas_24_latin1
#define STB_FONT_WIDTH STB_FONT_consolas_24_latin1_BITMAP_WIDTH
#define STB_FONT_HEIGHT STB_FONT_consolas_24_latin1_BITMAP_HEIGHT 
#define STB_FIRST_CHAR STB_FONT_consolas_24_latin1_FIRST_CHAR
#define STB_NUM_CHARS STB_FONT_consolas_24_latin1_NUM_CHARS

// Max. number of chars the text overlay buffer can hold
#define MAX_CHAR_COUNT 1024

/**
* @brief Mostly self-contained text overlay class
* @note Will only work with compatible render passes
*/
namespace vkx
{
	class VulkanTextOverlay
	{
		private:
			vkx::VulkanDevice * vulkanDevice;

			vk::Queue queue;
			vk::Format colorFormat;
			vk::Format depthFormat;

			uint32_t *frameBufferWidth;
			uint32_t *frameBufferHeight;

			vk::Sampler sampler;
			vk::Image image;
			vk::ImageView view;
			vkx::Buffer vertexBuffer;
			vk::DeviceMemory imageMemory;
			vk::DescriptorPool descriptorPool;
			vk::DescriptorSetLayout descriptorSetLayout;
			vk::DescriptorSet descriptorSet;
			vk::PipelineLayout pipelineLayout;
			vk::PipelineCache pipelineCache;
			vk::Pipeline pipeline;
			vk::RenderPass renderPass;
			vk::CommandPool commandPool;
			std::vector<vk::Framebuffer*> frameBuffers;
			std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
			vk::Fence fence;

			// Used during text updates
			glm::vec4 *mappedLocal = nullptr;

			stb_fontchar stbFontData[STB_NUM_CHARS];
			uint32_t numLetters;

		public:

			enum TextAlign { alignLeft, alignCenter, alignRight };

			bool visible = true;
			bool invalidated = false;

			std::vector<vk::CommandBuffer> cmdBuffers;

			/**
			* Default constructor
			*
			* @param vulkanDevice Pointer to a valid VulkanDevice
			*/
			VulkanTextOverlay(
				vkx::VulkanDevice * vulkanDevice,
				vk::Queue queue,
				std::vector<vk::Framebuffer> & framebuffers,
				vk::Format colorformat,
				vk::Format depthformat,
				uint32_t *framebufferwidth,
				uint32_t *framebufferheight,
				std::vector<vk::PipelineShaderStageCreateInfo> shaderstages);

			/**
			* Default destructor, frees up all Vulkan resources acquired by the text overlay
			*/
			~VulkanTextOverlay();

			/**
			* Prepare all vulkan resources required to render the font
			* The text overlay uses separate resources for descriptors (pool, sets, layouts), pipelines and command buffers
			*/
			void prepareResources();

			/**
			* Prepare a separate pipeline for the font rendering decoupled from the main application
			*/
			void preparePipeline();

			/**
			* Prepare a separate render pass for rendering the text as an overlay
			*/
			void prepareRenderPass();

			/**
			* Maps the buffer, resets letter count
			*/
			void beginTextUpdate();

			/**
			* Add text to the current buffer
			*
			* @param text Text to add
			* @param x x position of the text to add in window coordinate space
			* @param y y position of the text to add in window coordinate space
			* @param align Alignment for the new text (left, right, center)
			*/
			void addText(std::string text, float x, float y, TextAlign align);

			/**
			* Unmap buffer and update command buffers
			*/
			void endTextUpdate();

			/**
			* Update the command buffers to reflect text changes
			*/
			void updateCommandBuffers();

			/**
			* Submit the text command buffers to a queue
			*/
			void submit(vk::Queue queue, uint32_t bufferindex, vk::SubmitInfo submitInfo);

			/**
			* Reallocate command buffers for the text overlay
			* @note Frees the existing command buffers
			*/
			void reallocateCommandBuffers();
		};
}