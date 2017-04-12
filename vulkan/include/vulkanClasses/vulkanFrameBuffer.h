//
//  Created by Bradley Austin Davis on 2016/03/19
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <vector>
#include <algorithm>
#include <vulkan/vulkan.hpp>

#include "vulkanContext.h"

namespace vkx {

	/**
	* @brief Encapsulates a single frame buffer attachment
	*/
	struct FramebufferAttachment : public CreateImageResult {

		vk::Device device;

		vk::Image image;
		vk::DeviceMemory memory;
		vk::ImageView view;

		vk::Sampler sampler;
		vk::Format format{ vk::Format::eUndefined };
		vk::ImageSubresourceRange subresourceRange;
		vk::AttachmentDescription description;


		//void destroy() {

		//}

		/**
		* @brief Returns true if the attachment has a depth component
		*/
		bool hasDepth() {
			std::vector<vk::Format> formats = {
				vk::Format::eD16Unorm,
				vk::Format::eX8D24UnormPack32,
				vk::Format::eD32Sfloat,
				vk::Format::eD16UnormS8Uint,
				vk::Format::eD24UnormS8Uint,
				vk::Format::eD32SfloatS8Uint,
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		/**
		* @brief Returns true if the attachment has a stencil component
		*/
		
		bool hasStencil() {
			std::vector<vk::Format> formats = {
				vk::Format::eS8Uint,
				vk::Format::eD16UnormS8Uint,
				vk::Format::eD24UnormS8Uint,
				vk::Format::eD32SfloatS8Uint,
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		/**
		* @brief Returns true if the attachment is a depth and/or stencil attachment
		*/
		bool isDepthStencil() {
			return(hasDepth() || hasStencil());
		}



	};










	struct Framebuffer {
		
		//using FramebufferAttachment = CreateImageResult;

		uint32_t width, height;
		
		vk::Device device;
		const vkx::Context *context;// todo: remove


		vk::Framebuffer framebuffer;
		vk::RenderPass renderPass;

		// frame buffer attachments

		std::vector<FramebufferAttachment> colors;
		std::vector<FramebufferAttachment> attachments;

		

		// depth attachment
		FramebufferAttachment depthAttachment;

		bool hasDepth = false;


		void destroy();

		// Prepare a new framebuffer for offscreen rendering
		// The contents of this framebuffer are then
		// blitted to our render target
		void create(const vkx::Context &context, const glm::uvec2& size, const std::vector<vk::Format> &colorFormats, vk::Format depthFormat, const vk::RenderPass &renderPass, vk::ImageUsageFlags colorUsage = vk::ImageUsageFlagBits::eSampled, vk::ImageUsageFlags depthUsage = vk::ImageUsageFlags());
	
	












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
		void createSampler(vk::Filter magFilter, vk::Filter minFilter, vk::SamplerAddressMode adressMode) {
			vk::SamplerCreateInfo samplerInfo;
			samplerInfo.magFilter = magFilter;
			samplerInfo.minFilter = minFilter;
			samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			samplerInfo.addressModeU = adressMode;
			samplerInfo.addressModeV = adressMode;
			samplerInfo.addressModeW = adressMode;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.maxAnisotropy = 0;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 1.0f;
			samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
			//this->sampler = context->device.createSampler(samplerInfo, nullptr);
			//return sampler;
		}
	
	
	
		/**
		* Creates a default render pass setup with one sub pass
		*
		* @return VK_SUCCESS if all resources have been created successfully
		*/
		void createRenderPass() {

			std::vector<vk::AttachmentDescription> attachmentDescriptions;
			for (auto &attachment : attachments) {
				attachmentDescriptions.push_back(attachment.description);
			};

			// Collect attachment references
			std::vector<vk::AttachmentReference> colorReferences;
			vk::AttachmentReference depthReference = {};
			bool hasDepth = false;
			bool hasColor = false;

			uint32_t attachmentIndex = 0;

			for (auto &attachment : attachments) {
				if (attachment.isDepthStencil()) {
					// Only one depth attachment allowed
					assert(!hasDepth);
					depthReference.attachment = attachmentIndex;
					depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					hasDepth = true;
				} else {
					colorReferences.push_back({ attachmentIndex, vk::ImageLayout::eColorAttachmentOptimal });
					hasColor = true;
				}
				attachmentIndex++;
			};

			// Default render pass setup uses only one subpass
			vk::SubpassDescription subpass = {};
			subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
			if (hasColor) {
				subpass.pColorAttachments = colorReferences.data();
				subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			}
			if (hasDepth) {
				subpass.pDepthStencilAttachment = &depthReference;
			}

			// Use subpass dependencies for attachment layout transitions
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

			// Create render pass
			vk::RenderPassCreateInfo renderPassInfo;
			renderPassInfo.pAttachments = attachmentDescriptions.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			this->renderPass = device.createRenderPass(renderPassInfo, nullptr);

			std::vector<vk::ImageView> attachmentViews;
			for (auto attachment : attachments) {
				attachmentViews.push_back(attachment.view);
			}

			// Find. max number of layers across attachments
			uint32_t maxLayers = 0;
			for (auto attachment : attachments) {
				if (attachment.subresourceRange.layerCount > maxLayers) {
					maxLayers = attachment.subresourceRange.layerCount;
				}
			}

			vk::FramebufferCreateInfo framebufferInfo;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.pAttachments = attachmentViews.data();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = maxLayers;
			//VK_CHECK_RESULT(vkCreateFramebuffer(vulkanDevice->logicalDevice, &framebufferInfo, nullptr, &framebuffer));


			//return VK_SUCCESS;
		}


















		// Create a frame buffer attachment
		vkx::FramebufferAttachment createAttachment(
			vk::Format format,
			vk::ImageUsageFlagBits usage,
			uint32_t width,
			uint32_t height,
			uint32_t layerCount = 1)
		{


			FramebufferAttachment newAttachment;
			newAttachment.format = format;


			// attachment is a pointer
			// image created is stored there

			vk::ImageAspectFlags aspectMask;
			vk::ImageLayout imageLayout;

			// todo: don't cast, figure out how to & these

			// if this is a color attachment
			if ((VkImageUsageFlagBits)usage & (VkImageUsageFlagBits)vk::ImageUsageFlagBits::eColorAttachment) {
				aspectMask = vk::ImageAspectFlagBits::eColor;
				imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			}
			// if this is a depth stencil attachment
			if ((VkImageUsageFlagBits)usage & (VkImageUsageFlagBits)vk::ImageUsageFlagBits::eDepthStencilAttachment) {
				//aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
				aspectMask = vk::ImageAspectFlagBits::eDepth;// changed 4/11/17 bc validation layers
				imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			}


			// Select aspect mask and layout depending on usage

			// Color attachment
			if ((VkImageUsageFlagBits)usage & (VkImageUsageFlagBits)vk::ImageUsageFlagBits::eColorAttachment) {
				aspectMask = vk::ImageAspectFlagBits::eColor;
			}

			// Depth (and/or stencil) attachment
			if ((VkImageUsageFlagBits)usage & (VkImageUsageFlagBits)vk::ImageUsageFlagBits::eDepthStencilAttachment) {
				if (newAttachment.hasDepth()) {
					aspectMask = vk::ImageAspectFlagBits::eDepth;
				}
				if (newAttachment.hasStencil()) {
					aspectMask = aspectMask | vk::ImageAspectFlagBits::eStencil;
				}
			}



			vk::ImageCreateInfo imageInfo;
			imageInfo.imageType = vk::ImageType::e2D;
			imageInfo.format = format;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = layerCount;
			imageInfo.samples = vk::SampleCountFlagBits::e1;
			imageInfo.tiling = vk::ImageTiling::eOptimal;
			imageInfo.usage = usage | vk::ImageUsageFlagBits::eSampled;

			//imageInfo.initialLayout = imageLayout;// added 4/11/17





			
			


			// create image



			//vk::Image image = context->createImage(imageInfo, vk::MemoryPropertyFlagBits::eDeviceLocal).image;

			CreateImageResult temp = context->createImage(imageInfo, vk::MemoryPropertyFlagBits::eDeviceLocal);
			newAttachment.image = temp.image;
			newAttachment.memory = temp.memory;

			//vk::Image image = device.createImage(imageInfo);
			//newAttachment.image = device.createImage(imageInfo);

			//vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(newAttachment.image);
			//vk::MemoryAllocateInfo memAllocInfo;
			//memAllocInfo.allocationSize = memReqs.size;
			//memAllocInfo.memoryTypeIndex = context->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
			//newAttachment.memory = device.allocateMemory(memAllocInfo);
			//device.bindImageMemory(newAttachment.image, newAttachment.memory, 0);


			newAttachment.subresourceRange = {};
			newAttachment.subresourceRange.aspectMask = aspectMask;
			newAttachment.subresourceRange.levelCount = 1;
			newAttachment.subresourceRange.layerCount = layerCount;

			// create image view
			vk::ImageViewCreateInfo imageViewInfo;
			imageViewInfo.viewType = (layerCount == 1) ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray;// if there is more than one layer it's a 2DArray
			imageViewInfo.format = format;
			imageViewInfo.subresourceRange = newAttachment.subresourceRange;
			//todo: workaround for depth+stencil attachments
			imageViewInfo.subresourceRange.aspectMask = (newAttachment.hasDepth()) ? vk::ImageAspectFlagBits::eDepth : aspectMask;
			
			imageViewInfo.image = newAttachment.image;
			vk::ImageView imageView = device.createImageView(imageViewInfo);





			// Fill attachment description
			newAttachment.description = {};
			newAttachment.description.samples = vk::SampleCountFlagBits::e1;
			newAttachment.description.loadOp = vk::AttachmentLoadOp::eClear;
			newAttachment.description.storeOp = ((VkImageUsageFlagBits)usage & (VkImageUsageFlagBits)vk::ImageUsageFlagBits::eSampled) ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare;// todo figure out bitmasks with vulkan.hpp
			newAttachment.description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
			newAttachment.description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
			newAttachment.description.format = format;
			newAttachment.description.initialLayout = vk::ImageLayout::eUndefined;
			// Final layout
			// If not, final layout depends on attachment type
			if (newAttachment.hasDepth() || newAttachment.hasStencil()) {
				newAttachment.description.finalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
			} else {
				newAttachment.description.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			}
			

			
			
			newAttachment.view = imageView;
			newAttachment.image = newAttachment.image;

			this->attachments.push_back(newAttachment);

			return newAttachment;
		}
	
	
	
	
	
	
	};







	struct MyFrameBuffer {
		using Attachment = CreateImageResult;
		vk::Device device;
		vk::Framebuffer framebuffer;
		vk::RenderPass renderPass;


		// frame buffer attachments
		Attachment depthAttachment;
		std::vector<Attachment> attachments;

		//void destroy();

		// Prepare a new framebuffer for offscreen rendering
		// The contents of this framebuffer are then
		// blitted to our render target
		//void create(const vkx::Context &context, const glm::uvec2& size, const std::vector<vk::Format> &colorFormats, vk::Format depthFormat, const vk::RenderPass &renderPass, vk::ImageUsageFlags colorUsage = vk::ImageUsageFlagBits::eSampled, vk::ImageUsageFlags depthUsage = vk::ImageUsageFlags());
	};
}
