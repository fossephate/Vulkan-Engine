#pragma once

#include "vulkanContext.h"
#include "vulkanFramebuffer.h"

#define SHADOW_MAP_DIM 2048
#define NUM_LIGHTS_TOTAL 3

namespace vkx {

	struct Offscreen {
		const vkx::Context &context;
		bool active{ true };

		//vk::RenderPass renderPass;

		vk::CommandBuffer cmdBuffer;
		vk::Semaphore renderComplete;


		// options

		glm::uvec2 size;
		//std::vector<vk::Format> colorFormats = std::vector<vk::Format>{ {
		//		vk::Format::eR16G16B16A16Sfloat,
		//		vk::Format::eR16G16B16A16Sfloat,
		//		vk::Format::eR8G8B8A8Unorm
		//	} };
		//// This value is chosen as an invalid default that signals that the code should pick a specific depth buffer
		//// Alternative, you can set this to undefined to explicitly declare you want no depth buffer.
		//vk::Format depthFormat = vk::Format::eR8Uscaled;
		//vk::ImageUsageFlags attachmentUsage{ vk::ImageUsageFlagBits::eSampled };
		//vk::ImageUsageFlags depthAttachmentUsage;
		//vk::ImageLayout colorFinalLayout{ vk::ImageLayout::eShaderReadOnlyOptimal };
		//vk::ImageLayout depthFinalLayout{ vk::ImageLayout::eUndefined };


		std::vector<vkx::Framebuffer> framebuffers;

		Offscreen(const vkx::Context &context) : context(context) {}

		void prepare() {
			//assert(!colorFormats.empty());
			//assert(size != glm::uvec2());

			//// Find a suitable depth format
			//if (depthFormat == vk::Format::eR8Uscaled) {
			//	depthFormat = vkx::getSupportedDepthFormat(context.physicalDevice);
			//}


			cmdBuffer = context.device.allocateCommandBuffers(vkx::commandBufferAllocateInfo(context.getCommandPool(), vk::CommandBufferLevel::ePrimary, 1))[0];
			renderComplete = context.device.createSemaphore(vk::SemaphoreCreateInfo());
			//if (!renderPass) {
				//prepareRenderPass();
			//}

			//prepareRenderPasses();
			
			//addDeferredFramebuffer();
			addDeferredFramebuffer2();
			addSSAOGenerateFramebuffer();
			addSSAOBlurFramebuffer();

			addShadowPassFramebuffer();



			////prepareOffscreenFramebuffers();

			//framebuffers[0].create(context, size, colorFormats, depthFormat, renderPass, attachmentUsage, depthAttachmentUsage);

			////for (auto &framebuffer : framebuffers) {
			////	framebuffer.create(context, size, colorFormats, depthFormat, renderPass, attachmentUsage, depthAttachmentUsage);
			////}
			//prepareSampler();
		}


		void addFramebuffer() {

		}

		void destroy() {
			for (auto &framebuffer : framebuffers) {
				framebuffer.destroy();
			}
			framebuffers.clear();
			context.device.freeCommandBuffers(context.getCommandPool(), cmdBuffer);

			//context.device.destroyRenderPass(renderPass);
			context.device.destroySemaphore(renderComplete);
		}















		// Create a frame buffer attachment
		CreateImageResult createAttachment(
			vk::Format format,
			vk::ImageUsageFlagBits usage,
			uint32_t width,
			uint32_t height)
		{




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



			vk::ImageCreateInfo imageInfo;/* = vkx::imageCreateInfo();*/
			imageInfo.imageType = vk::ImageType::e2D;
			imageInfo.format = format;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.samples = vk::SampleCountFlagBits::e1;
			imageInfo.tiling = vk::ImageTiling::eOptimal;
			imageInfo.usage = usage | vk::ImageUsageFlagBits::eSampled;

			//imageInfo.initialLayout = imageLayout;// added 4/11/17

			//if (enableNVDedicatedAllocation) {
			//	VkDedicatedAllocationImageCreateInfoNV dedicatedImageInfo{ VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV };
			//	dedicatedImageInfo.dedicatedAllocation = VK_TRUE;
			//	image.pNext = &dedicatedImageInfo;
			//}

			vk::ImageViewCreateInfo imageViewInfo;
			imageViewInfo.viewType = vk::ImageViewType::e2D;

			//imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imageViewInfo.subresourceRange.aspectMask = aspectMask;// added 3/29/17

			imageViewInfo.subresourceRange.levelCount = 1;
			imageViewInfo.subresourceRange.layerCount = 1;



			CreateImageResult newAttachment;
			// create image
			newAttachment = context.createImage(imageInfo, vk::MemoryPropertyFlagBits::eDeviceLocal);
			newAttachment.format = format;// redundant


			imageViewInfo.format = format;// probably redundant
			imageViewInfo.image = newAttachment.image;
			// create image view
			newAttachment.view = context.device.createImageView(imageViewInfo);


			return newAttachment;
		}







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
		vk::Sampler createSampler(vk::Filter magFilter, vk::Filter minFilter, vk::SamplerAddressMode addressMode) {
			vk::SamplerCreateInfo samplerInfo;
			samplerInfo.magFilter = magFilter;
			samplerInfo.minFilter = minFilter;
			samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			samplerInfo.addressModeU = addressMode;
			samplerInfo.addressModeV = addressMode;
			samplerInfo.addressModeW = addressMode;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.maxAnisotropy = 0;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 1.0f;
			samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
			//return vkCreateSampler(vulkanDevice->logicalDevice, &samplerInfo, nullptr, &sampler);
			vk::Sampler sampler = context.device.createSampler(samplerInfo, nullptr);
			return sampler;
		}











		void addDeferredFramebuffer2() {

			vkx::Framebuffer deferredFramebuffer;
			deferredFramebuffer.device = context.device;
			deferredFramebuffer.context = &context;
			deferredFramebuffer.width = this->size.x;
			deferredFramebuffer.height = this->size.y;

			// Offscreen framebuffer, Color attachments

			// Attachment 0: World space positions
			deferredFramebuffer.createAttachment(vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eColorAttachment, this->size.x, this->size.y);

			// Attachment 1: World space normal
			deferredFramebuffer.createAttachment(vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment, this->size.x, this->size.y);


			// Attachment 2: Packed colors, specular
			deferredFramebuffer.createAttachment(vk::Format::eR32G32B32A32Uint, vk::ImageUsageFlagBits::eColorAttachment, this->size.x, this->size.y);


			// Offscreen depth attachment:

			// Find a suitable depth format
			vk::Format depthFormat = vkx::getSupportedDepthFormat(context.physicalDevice);
			/*deferredFramebuffer.depthAttachment = */deferredFramebuffer.createAttachment(depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment, this->size.x, this->size.y);

			//VulkanExampleBase::flushCommandBuffer(layoutCmd, queue, true);

			// todo: fix:
			//deferredFramebuffer.createRenderPass();

			// G-Buffer creation
			{
				std::array<vk::AttachmentDescription, 4> attachmentDescs = {};

				// Init attachment properties
				for (uint32_t i = 0; i < static_cast<uint32_t>(attachmentDescs.size()); i++) {
					attachmentDescs[i].samples = vk::SampleCountFlagBits::e1;
					attachmentDescs[i].loadOp = vk::AttachmentLoadOp::eClear;
					attachmentDescs[i].storeOp = vk::AttachmentStoreOp::eStore;
					attachmentDescs[i].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
					attachmentDescs[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
					// all but 3rd are eShaderReadOnlyOptimal, 3rd is eDepthStencilAttachmentOptimal 
					attachmentDescs[i].finalLayout = (i == 3) ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
				}






				// Formats
				attachmentDescs[0].format = deferredFramebuffer.attachments[0].format;
				attachmentDescs[1].format = deferredFramebuffer.attachments[1].format;
				attachmentDescs[2].format = deferredFramebuffer.attachments[2].format;
				//attachmentDescs[3].format = deferredFramebuffer.depthAttachment.format;
				attachmentDescs[3].format = deferredFramebuffer.attachments[3].format;

				// color attachment references
				std::vector<vk::AttachmentReference> colorReferences;
				colorReferences.push_back({ 0, vk::ImageLayout::eColorAttachmentOptimal });
				colorReferences.push_back({ 1, vk::ImageLayout::eColorAttachmentOptimal });
				colorReferences.push_back({ 2, vk::ImageLayout::eColorAttachmentOptimal });

				// depth reference
				vk::AttachmentReference depthReference;
				depthReference.attachment = 3;
				depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

				vk::SubpassDescription subpass;
				subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
				subpass.pColorAttachments = colorReferences.data();
				subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
				subpass.pDepthStencilAttachment = &depthReference;


				// Use subpass dependencies for attachment layout transitions
				std::vector<vk::SubpassDependency> dependencies;
				dependencies.resize(2);

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

				// create offscreen render pass subpass?
				vk::RenderPassCreateInfo renderPassInfo;
				renderPassInfo.pAttachments = attachmentDescs.data();
				renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
				renderPassInfo.subpassCount = 1;
				renderPassInfo.pSubpasses = &subpass;
				renderPassInfo.dependencyCount = 2;
				renderPassInfo.pDependencies = dependencies.data();
				deferredFramebuffer.renderPass = context.device.createRenderPass(renderPassInfo, nullptr);

				std::vector<vk::ImageView> attachments;
				attachments.resize(3);
				for (size_t i = 0; i < 3; ++i) {
					attachments[i] = deferredFramebuffer.attachments[i].view;// color attachments
				}
				//attachments.push_back(deferredFramebuffer.depthAttachment.view);// depth attachment
				attachments.push_back(deferredFramebuffer.attachments[3].view);// depth attachment

				vk::FramebufferCreateInfo fbufCreateInfo;
				fbufCreateInfo.renderPass = deferredFramebuffer.renderPass;
				fbufCreateInfo.pAttachments = attachments.data();
				fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
				fbufCreateInfo.width = this->size.x;
				fbufCreateInfo.height = this->size.y;
				fbufCreateInfo.layers = 1;
				// create framebuffer
				deferredFramebuffer.framebuffer = context.device.createFramebuffer(fbufCreateInfo, nullptr);
			}



			// SHARED!:
			deferredFramebuffer.attachments[0].sampler = createSampler(vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge);
			
			framebuffers.push_back(deferredFramebuffer);
		}



		// add SSAO frame buffer
		void addSSAOGenerateFramebuffer() {

			vkx::Framebuffer SSAOGenerateFramebuffer;
			SSAOGenerateFramebuffer.device = context.device;
			SSAOGenerateFramebuffer.context = &context;
			SSAOGenerateFramebuffer.width = this->size.x;
			SSAOGenerateFramebuffer.height = this->size.y;

			SSAOGenerateFramebuffer.createAttachment(vk::Format::eR8Unorm, vk::ImageUsageFlagBits::eColorAttachment, this->size.x, this->size.y);
			SSAOGenerateFramebuffer.createRenderPass();
			framebuffers.push_back(SSAOGenerateFramebuffer);
		}



		void addSSAOBlurFramebuffer() {

			vkx::Framebuffer SSAOBlurFramebuffer;
			SSAOBlurFramebuffer.device = context.device;
			SSAOBlurFramebuffer.context = &context;
			SSAOBlurFramebuffer.width = this->size.x;
			SSAOBlurFramebuffer.height = this->size.y;

			SSAOBlurFramebuffer.createAttachment(vk::Format::eR8Unorm, vk::ImageUsageFlagBits::eColorAttachment, this->size.x, this->size.y);
			SSAOBlurFramebuffer.createRenderPass();

			framebuffers.push_back(SSAOBlurFramebuffer);
		}






		void addShadowPassFramebuffer() {
			//int SHADOW_MAP_DIM = 2048;

			vkx::Framebuffer shadowFramebuffer;
			shadowFramebuffer.device = context.device;
			shadowFramebuffer.context = &context;
			shadowFramebuffer.width = SHADOW_MAP_DIM;
			shadowFramebuffer.height = SHADOW_MAP_DIM;
			//shadowFramebuffer.width = this->size.x;
			//shadowFramebuffer.height = this->size.y;

			vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;

			vk::Format shadowMapFormat = vk::Format::eD32SfloatS8Uint;

			//shadowFramebuffer.createAttachment(vk::Format::eR8Unorm, vk::ImageUsageFlagBits::eColorAttachment, this->size.x, this->size.y, 1);
			// depth stencil attachment:
			shadowFramebuffer.createAttachment(shadowMapFormat, usage, SHADOW_MAP_DIM, SHADOW_MAP_DIM, NUM_LIGHTS_TOTAL);
			//shadowFramebuffer.createAttachment(shadowMapFormat, usage, this->size.x, this->size.y, 3);

			shadowFramebuffer.attachments[0].sampler = createSampler(vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge);

			shadowFramebuffer.createRenderPass();



			framebuffers.push_back(shadowFramebuffer);

		}





	};
}