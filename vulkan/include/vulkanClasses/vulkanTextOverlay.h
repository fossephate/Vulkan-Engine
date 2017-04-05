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

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "vulkanTools.h"
#include "vulkanDebug.h"
#include "vulkanContext.h"

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

namespace vkx {
	// Mostly self-contained text overlay class
	// todo : comment
	class TextOverlay {
		private:
			Context context;

			uint32_t &framebufferWidth;
			uint32_t &framebufferHeight;

			CreateImageResult texture;
			CreateBufferResult vertexBuffer;

			vk::DescriptorPool descriptorPool;
			vk::DescriptorSetLayout descriptorSetLayout;
			vk::DescriptorSet descriptorSet;
			vk::PipelineLayout pipelineLayout;
			vk::Pipeline pipeline;

			// Pointer to mapped vertex buffer
			glm::vec4 *mapped = nullptr;

			stb_fontchar stbFontData[STB_NUM_CHARS];
			uint32_t numLetters;

		public:

			enum TextAlign { alignLeft, alignCenter, alignRight };
			std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

			bool visible = true;
			bool invalidated = false;

			TextOverlay(
				Context context,
				uint32_t& framebufferwidth,
				uint32_t& framebufferheight,
				vk::RenderPass renderPass);

			~TextOverlay();

			// Prepare all vulkan resources required to render the font
			// The text overlay uses separate resources for descriptors (pool, sets, layouts), pipelines and command buffers
			void prepareResources();

			// Prepare a separate pipeline for the font rendering decoupled from the main application
			void preparePipeline(vk::RenderPass renderPass);

			// Map buffer 
			void beginTextUpdate();

			// Add text to the current buffer
			// todo : drop shadow? color attribute?
			void addText(std::string text, float x, float y, TextAlign align);

			// Unmap buffer and update command buffers
			void endTextUpdate();

			// Needs to be called by the application
			void writeCommandBuffer(const vk::CommandBuffer& cmdBuffer);
	};
}
