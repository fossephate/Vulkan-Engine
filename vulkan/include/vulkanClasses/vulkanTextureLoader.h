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
		vk::Device device = nullptr;
		vk::Image image = nullptr;
		vk::DeviceMemory memory = nullptr;
		vk::Sampler sampler = nullptr;

		vk::ImageLayout imageLayout{ vk::ImageLayout::eShaderReadOnlyOptimal };
		vk::ImageView view;
		vk::Extent3D extent{ 0, 0, 1 };
		vk::DescriptorImageInfo descriptor;

		uint32_t mipLevels{ 1 };
		uint32_t layerCount{ 1 };

		Texture &operator=(const vkx::CreateImageResult &created) {
			device = created.device;
			image = created.image;
			memory = created.memory;
			extent = created.extent;
			return *this;
		}

		void destroy() {
			if (sampler) {
				device.destroySampler(sampler);
				sampler = vk::Sampler();
			}
			if (view) {
				device.destroyImageView(view);
				view = vk::ImageView();
			}
			if (image) {
				device.destroyImage(image);
				image = vk::Image();
			}
			if (memory) {
				device.freeMemory(memory);
				memory = vk::DeviceMemory();
			}
		}
	};

	class TextureLoader {
		private:
			//Context context;
			const Context &context;
			//Context context;
			//Context *context;

			vk::CommandBuffer cmdBuffer;

			//vk::Queue queue;
			vk::CommandPool cmdPool;

		

		public:

			TextureLoader(const Context &context);

			TextureLoader(const Context &context, vk::CommandPool cmdPool);

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

			void createTexture(void * buffer, vk::DeviceSize bufferSize, vk::Format format, uint32_t width, uint32_t height, vkx::Texture * texture, vk::Filter filter = vk::Filter::eLinear, vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);
			
			//void createTexture(void * buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t width, uint32_t height, vkx::Texture * texture, VkFilter filter, VkImageUsageFlags imageUsageFlags);
		};
}
