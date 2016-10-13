/*
* Vulkan framebuffer class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <algorithm>
#include <iterator>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "./vulkanDevice.h"
#include "./vulkanTools.h"

namespace vkx
{
	/**
	* @brief Encapsulates a single frame buffer attachment
	*/
	struct FramebufferAttachment
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkFormat format;
		VkImageSubresourceRange subresourceRange;
		VkAttachmentDescription description;
		VkImageLayout initialLayout;

		/**
		* @brief Returns true if the attachment has a depth component
		*/
		bool hasDepth();

		/**
		* @brief Returns true if the attachment has a stencil component
		*/
		bool hasStencil();

		/**
		* @brief Returns true if the attachment is a depth and/or stencil attachment
		*/
		bool isDepthStencil();

	};

	/**
	* @brief Describes the attributes of an attachment to be created
	*/
	struct AttachmentCreateInfo
	{
		uint32_t width, height;
		uint32_t layerCount;
		VkFormat format;
		VkImageUsageFlags usage;
	};

	/**
	* @brief Encapsulates a complete Vulkan framebuffer with an arbitrary number and combination of attachments
	*/
	struct Framebuffer
	{
		private:
			vkx::VulkanDevice *vulkanDevice;
		public:
			uint32_t width, height;
			VkFramebuffer framebuffer;
			VkRenderPass renderPass;
			VkSampler sampler;
			std::vector<vkx::FramebufferAttachment> attachments;

			/**
			* Default constructor
			*
			* @param vulkanDevice Pointer to a valid VulkanDevice
			*/
			Framebuffer(vkx::VulkanDevice *vulkanDevice);

			/**
			* Destroy and free Vulkan resources used for the framebuffer and all of it's attachments
			*/
			~Framebuffer();

			/**
			* Add a new attachment described by createinfo to the framebuffer's attachment list
			*
			* @param createinfo Structure that specifices the framebuffer to be constructed
			*
			* @return Index of the new attachment
			*/
			uint32_t addAttachment(vkx::AttachmentCreateInfo createinfo);

			/**
			* Creates a default sampler for sampling from any of the framebuffer attachments
			* Applications are free to create their own samplers for different use cases
			*
			* @param magFilter Magnification filter for lookups
			* @param minFilter Minification filter for lookups
			* @param adressMode Adressing mode for the U,V and W coordinates
			*
			* @return VkResult for the sampler creation
			*/
			VkResult createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode adressMode);

			/**
			* Creates a default render pass setup with one sub pass
			*
			* @return VK_SUCCESS if all resources have been created successfully
			*/
			VkResult createRenderPass();
	};
}