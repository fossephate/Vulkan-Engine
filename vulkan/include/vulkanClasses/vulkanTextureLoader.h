/*
* Texture loader for Vulkan
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vulkan/vulkan.hpp>
#pragma warning(disable: 4996 4244 4267)
#include <gli/gli.hpp>
#include "vulkanTools.h"
#include "vulkanContext.h"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

namespace vkx {

	struct Texture {
		vk::Device device;
		vk::Image image;
		vk::DeviceMemory memory;
		vk::Sampler sampler;
		vk::ImageLayout imageLayout{ vk::ImageLayout::eShaderReadOnlyOptimal };
		vk::ImageView view;
		vk::Extent3D extent{ 0, 0, 1 };
		vk::DescriptorImageInfo descriptor;

		uint32_t mipLevels{ 1 };
		uint32_t layerCount{ 1 };

		Texture& operator=(const vkx::CreateImageResult& created) {
			device = created.device;
			image = created.image;
			memory = created.memory;
			return *this;
		}

		void destroy();
	};

	class TextureLoader {
	private:
		//Context context;
		//const Context &context;
		//Context context;
		Context *context;

		vk::CommandBuffer cmdBuffer;

		

	public:

		TextureLoader(const Context &context);

		~TextureLoader();

		#if defined(__ANDROID__)
		AAssetManager* assetManager = nullptr;
		#endif

		// Load a 2D texture
		Texture loadTexture(const std::string& filename, vk::Format format, bool forceLinear = false, vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);

		// Load a cubemap texture (single file)
		Texture loadCubemap(const std::string& filename, vk::Format format);

		// Load an array texture (single file)
		Texture loadTextureArray(const std::string& filename, vk::Format format);
	};
}
