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
#include "vulkanDevice.h"
#include "vulkanTools.h"

namespace vkx
{
	/**
	* @brief Encapsulates a single frame buffer attachment
	*/
	struct FramebufferAttachment
	{
		vk::Image image;
		vk::DeviceMemory memory;
		vk::ImageView view;
		vk::Format format;
		vk::ImageSubresourceRange subresourceRange;
		vk::AttachmentDescription description;
		vk::ImageLayout initialLayout;

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
		vk::Format format;
		vk::ImageUsageFlags usage;
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
			vk::Framebuffer framebuffer;
			vk::RenderPass renderPass;
			vk::Sampler sampler;
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
			* @return vk::Result for the sampler creation
			*/
			vk::Result createSampler(vk::Filter magFilter, vk::Filter minFilter, vk::SamplerAddressMode adressMode);

			/**
			* Creates a default render pass setup with one sub pass
			*
			* @return VK_SUCCESS if all resources have been created successfully
			*/
			vk::Result createRenderPass();
	};
}