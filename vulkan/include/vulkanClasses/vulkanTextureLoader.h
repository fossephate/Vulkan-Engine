/*
* Texture loader for Vulkan
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#if defined(__ANDROID__)
	#include <android/asset_manager.h>
#endif

#include <vulkan/vulkan.hpp>
#include "vulkanDevice.h"
#include <gli/gli.hpp>





namespace vkTools
{
	/**
	* @brief Encapsulates a Vulkan texture object (including view, sampler, descriptor, etc.)
	*/
	struct VulkanTexture
	{
		vk::Sampler sampler;
		vk::Image image;
		vk::ImageLayout imageLayout;
		vk::DeviceMemory deviceMemory;
		vk::ImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
		uint32_t layerCount;
		vk::DescriptorImageInfo descriptor;
	};

	/**
	* @brief A simple Vulkan texture uploader for getting images into GPU memory
	*/
	class VulkanTextureLoader
	{
		private:
			vkx::VulkanDevice *vulkanDevice;
			vk::Queue queue;
			vk::CommandBuffer cmdBuffer;
			vk::CommandPool cmdPool;
		public:
			#if defined(__ANDROID__)
				AAssetManager* assetManager = nullptr;
			#endif

			/**
			* Default constructor
			*
			* @param vulkanDevice Pointer to a valid VulkanDevice
			* @param queue Queue for the copy commands when using staging (queue must support transfers)
			* @param cmdPool Commandpool used to get command buffers for copies and layout transitions
			*/
			VulkanTextureLoader(vkx::VulkanDevice * vulkanDevice, vk::Queue queue, vk::CommandPool cmdPool);

			/**
			* Default destructor
			*
			* @note Does not free texture resources
			*/
			~VulkanTextureLoader();

			/**
			* Load a 2D texture including all mip levels
			*
			* @param filename File to load
			* @param format Vulkan format of the image data stored in the file
			* @param texture Pointer to the texture object to load the image into
			* @param (Optional) forceLinear Force linear tiling (not advised, defaults to false)
			* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
			*
			* @note Only supports .ktx and .dds
			*/
			void loadTexture(std::string filename, vk::Format format, VulkanTexture *texture, bool forceLinear = false, vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);

			/**
			* Load a cubemap texture including all mip levels from a single file
			*
			* @param filename File to load
			* @param format Vulkan format of the image data stored in the file
			* @param texture Pointer to the texture object to load the image into
			*
			* @note Only supports .ktx and .dds
			*/
			void loadCubemap(std::string filename, vk::Format format, VulkanTexture *texture, vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);

			/**
			* Load a texture array including all mip levels from a single file
			*
			* @param filename File to load
			* @param format Vulkan format of the image data stored in the file
			* @param texture Pointer to the texture object to load the image into
			*
			* @note Only supports .ktx and .dds
			*/
			void loadTextureArray(std::string filename, vk::Format format, VulkanTexture *texture, vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);

			/**
			* Free all Vulkan resources used by a texture object
			*
			* @param texture Texture object whose resources are to be freed
			*/
			void destroyTexture(VulkanTexture texture);
	};
};
