#pragma once

#include "vulkanContext.h"
#include "vulkanFramebuffer.h"

namespace vkx {

	struct Offscreen {
		const vkx::Context& context;
		bool active{ true };

		vk::RenderPass renderPass;

		vk::CommandBuffer cmdBuffer;
		vk::Semaphore renderComplete;



		//struct {
		//	vkx::MyFrameBuffer offscreen;
		//	vkx::MyFrameBuffer ssaoGenerate;
		//	vkx::MyFrameBuffer ssaoBlur;
		//} SSAOFrameBuffers;


		// options

		glm::uvec2 size = glm::uvec2(1024);
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
			
			addDeferredFramebuffer();
			addSSAOGenerateFramebuffer();
			addSSAOBlurFramebuffer();



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
			for (auto& framebuffer : framebuffers) {
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
				aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
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

			//if (enableNVDedicatedAllocation) {
			//	VkDedicatedAllocationImageCreateInfoNV dedicatedImageInfo{ VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV };
			//	dedicatedImageInfo.dedicatedAllocation = VK_TRUE;
			//	image.pNext = &dedicatedImageInfo;
			//}

			vk::ImageViewCreateInfo imageViewInfo;
			imageViewInfo.viewType = vk::ImageViewType::e2D;
			imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
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




			//vk::MemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
			//vk::MemoryRequirements memReqs;
			//vkGetImageMemoryRequirements(device, attachment->image, &memReqs);
			//memAlloc.allocationSize = memReqs.size;
			//memAlloc.memoryTypeIndex = getMemTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

			//if (enableNVDedicatedAllocation)
			//{
			//	VkDedicatedAllocationMemoryAllocateInfoNV dedicatedAllocationInfo{ VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV };
			//	dedicatedAllocationInfo.image = attachment->image;
			//	memAlloc.pNext = &dedicatedAllocationInfo;
			//}

			//VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &attachment->mem));
			//VK_CHECK_RESULT(vkBindImageMemory(device, attachment->image, attachment->mem, 0));

			//VkImageViewCreateInfo imageView = vkTools::initializers::imageViewCreateInfo();
			//imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			//imageView.format = format;
			//imageView.subresourceRange = {};
			//imageView.subresourceRange.aspectMask = aspectMask;
			//imageView.subresourceRange.baseMipLevel = 0;
			//imageView.subresourceRange.levelCount = 1;
			//imageView.subresourceRange.baseArrayLayer = 0;
			//imageView.subresourceRange.layerCount = 1;
			//imageView.image = attachment->image;
			//VK_CHECK_RESULT(vkCreateImageView(device, &imageView, nullptr, &attachment->view));
		}


































		void addDeferredFramebuffer() {

			// -------------------------------------------------------------------------------
			// -------------------------------------------------------------------------------
			// Options
			
			glm::uvec2 size = glm::uvec2(1024);
			std::vector<vk::Format> colorFormats = std::vector<vk::Format>{{
					vk::Format::eR16G16B16A16Sfloat,
					vk::Format::eR16G16B16A16Sfloat,
					vk::Format::eR8G8B8A8Unorm
			}};
			// This value is chosen as an invalid default that signals that the code should pick a specific depth buffer
			// Alternative, you can set this to undefined to explicitly declare you want no depth buffer.
			vk::Format depthFormat = vk::Format::eR8Uscaled;

			// Find a suitable depth format
			if (depthFormat == vk::Format::eR8Uscaled) {
				depthFormat = vkx::getSupportedDepthFormat(context.physicalDevice);
			}

			vk::ImageUsageFlags attachmentUsage{ vk::ImageUsageFlagBits::eSampled };
			vk::ImageUsageFlags depthAttachmentUsage;
			vk::ImageLayout colorFinalLayout{ vk::ImageLayout::eShaderReadOnlyOptimal };
			vk::ImageLayout depthFinalLayout{ vk::ImageLayout::eUndefined };


			// End of options
			// ---------------------------------------------------------------------------------


			vkx::Framebuffer deferredFramebuffer;
			deferredFramebuffer.device = context.device;


			// create deferred render pass
			{

				vk::SubpassDescription subpass;
				subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

				std::vector<vk::AttachmentDescription> attachments;
				std::vector<vk::AttachmentReference> colorAttachmentReferences;
				attachments.resize(colorFormats.size());
				colorAttachmentReferences.resize(attachments.size());
				// Color attachment
				for (size_t i = 0; i < attachments.size(); ++i) {
					attachments[i].format = colorFormats[i];
					attachments[i].loadOp = vk::AttachmentLoadOp::eClear;
					attachments[i].storeOp = colorFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
					attachments[i].initialLayout = vk::ImageLayout::eUndefined;
					attachments[i].finalLayout = colorFinalLayout;

					vk::AttachmentReference& attachmentReference = colorAttachmentReferences[i];
					attachmentReference.attachment = i;
					attachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

					subpass.colorAttachmentCount = colorAttachmentReferences.size();
					subpass.pColorAttachments = colorAttachmentReferences.data();
				}

				// Do we have a depth format?
				vk::AttachmentReference depthAttachmentReference;
				if (depthFormat != vk::Format::eUndefined) {
					vk::AttachmentDescription depthAttachment;
					depthAttachment.format = depthFormat;
					depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
					// We might be using the depth attacment for something, so preserve it if it's final layout is not undefined
					depthAttachment.storeOp = depthFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
					depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
					depthAttachment.finalLayout = depthFinalLayout;
					attachments.push_back(depthAttachment);
					depthAttachmentReference.attachment = attachments.size() - 1;
					depthAttachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					subpass.pDepthStencilAttachment = &depthAttachmentReference;
				}

				std::vector<vk::SubpassDependency> subpassDependencies;
				{
					if ((colorFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (colorFinalLayout != vk::ImageLayout::eUndefined)) {
						// Implicit transition 
						vk::SubpassDependency dependency;
						dependency.srcSubpass = 0;
						dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
						dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

						dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
						dependency.dstAccessMask = vkx::accessFlagsForLayout(colorFinalLayout);
						dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
						subpassDependencies.push_back(dependency);
					}

					if ((depthFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (depthFinalLayout != vk::ImageLayout::eUndefined)) {
						// Implicit transition 
						vk::SubpassDependency dependency;
						dependency.srcSubpass = 0;
						dependency.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
						dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;

						dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
						dependency.dstAccessMask = vkx::accessFlagsForLayout(depthFinalLayout);
						dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
						subpassDependencies.push_back(dependency);
					}
				}

				//if (renderPass) {
				//	context.device.destroyRenderPass(renderPass);
				//}

				vk::RenderPassCreateInfo renderPassInfo;
				renderPassInfo.attachmentCount = attachments.size();
				renderPassInfo.pAttachments = attachments.data();
				renderPassInfo.subpassCount = 1;
				renderPassInfo.pSubpasses = &subpass;
				renderPassInfo.dependencyCount = subpassDependencies.size();
				renderPassInfo.pDependencies = subpassDependencies.data();
				
				deferredFramebuffer.renderPass = context.device.createRenderPass(renderPassInfo);
			}

			// create frame buffer:
			{


				deferredFramebuffer.colors.resize(colorFormats.size());

				// Color attachment
				vk::ImageCreateInfo image;
				image.imageType = vk::ImageType::e2D;
				image.extent.width = size.x;
				image.extent.height = size.y;
				image.extent.depth = 1;
				image.mipLevels = 1;
				image.arrayLayers = 1;
				image.samples = vk::SampleCountFlagBits::e1;
				image.tiling = vk::ImageTiling::eOptimal;
				// vk::Image of the framebuffer is blit source
				image.usage = vk::ImageUsageFlagBits::eColorAttachment | attachmentUsage;

				vk::ImageViewCreateInfo colorImageView;
				colorImageView.viewType = vk::ImageViewType::e2D;
				colorImageView.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				colorImageView.subresourceRange.levelCount = 1;
				colorImageView.subresourceRange.layerCount = 1;

				for (size_t i = 0; i < colorFormats.size(); ++i) {
					image.format = colorFormats[i];
					deferredFramebuffer.colors[i] = context.createImage(image, vk::MemoryPropertyFlagBits::eDeviceLocal);
					colorImageView.format = colorFormats[i];
					colorImageView.image = deferredFramebuffer.colors[i].image;
					deferredFramebuffer.colors[i].view = context.device.createImageView(colorImageView);
				}


				bool useDepth = depthFormat != vk::Format::eUndefined;
				// Depth stencil attachment
				if (useDepth) {
					image.format = depthFormat;
					image.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | depthAttachmentUsage;
					deferredFramebuffer.depthAttachment = context.createImage(image, vk::MemoryPropertyFlagBits::eDeviceLocal);

					vk::ImageViewCreateInfo depthStencilView;
					depthStencilView.viewType = vk::ImageViewType::e2D;
					depthStencilView.format = depthFormat;
					depthStencilView.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
					depthStencilView.subresourceRange.levelCount = 1;
					depthStencilView.subresourceRange.layerCount = 1;
					depthStencilView.image = deferredFramebuffer.depthAttachment.image;
					deferredFramebuffer.depthAttachment.view = context.device.createImageView(depthStencilView);

				}

				std::vector<vk::ImageView> attachments;
				attachments.resize(deferredFramebuffer.colors.size());
				for (size_t i = 0; i < deferredFramebuffer.colors.size(); ++i) {
					attachments[i] = deferredFramebuffer.colors[i].view;
				}
				if (useDepth) {
					attachments.push_back(deferredFramebuffer.depthAttachment.view);
				}

				vk::FramebufferCreateInfo fbufCreateInfo;
				fbufCreateInfo.renderPass = deferredFramebuffer.renderPass;
				fbufCreateInfo.attachmentCount = (uint32_t)attachments.size();
				fbufCreateInfo.pAttachments = attachments.data();
				fbufCreateInfo.width = size.x;
				fbufCreateInfo.height = size.y;
				fbufCreateInfo.layers = 1;
				deferredFramebuffer.framebuffer = context.device.createFramebuffer(fbufCreateInfo);


			}






			//deferredFramebuffer.create(context, size, colorFormats, depthFormat, renderPass, attachmentUsage, depthAttachmentUsage);


			// create sampler
			{
				// Create sampler
				vk::SamplerCreateInfo samplerInfo;
				samplerInfo.magFilter = vk::Filter::eLinear;
				samplerInfo.minFilter = vk::Filter::eLinear;
				samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
				samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
				samplerInfo.addressModeV = samplerInfo.addressModeU;
				samplerInfo.addressModeW = samplerInfo.addressModeU;
				samplerInfo.mipLodBias = 0.0f;
				samplerInfo.maxAnisotropy = 0;
				samplerInfo.compareOp = vk::CompareOp::eNever;
				samplerInfo.minLod = 0.0f;
				samplerInfo.maxLod = 0.0f;
				samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;

				//for (auto &framebuffer : framebuffers) {
				//	if (attachmentUsage | vk::ImageUsageFlagBits::eSampled) {
				//		for (auto& color : framebuffer.colors) {
				//			color.sampler = context.device.createSampler(samplerInfo);
				//		}
				//	}
				//	if (depthAttachmentUsage | vk::ImageUsageFlagBits::eSampled) {
				//		framebuffer.depthAttachment.sampler = context.device.createSampler(samplerInfo);
				//	}
				//}

				auto &framebuffer = deferredFramebuffer;
				if (attachmentUsage | vk::ImageUsageFlagBits::eSampled) {
					for (auto &color : framebuffer.colors) {
						color.sampler = context.device.createSampler(samplerInfo);
					}
				}
				if (depthAttachmentUsage | vk::ImageUsageFlagBits::eSampled) {
					framebuffer.depthAttachment.sampler = context.device.createSampler(samplerInfo);
				}

			}


			framebuffers.push_back(deferredFramebuffer);
		}






















































		// add SSAO frame buffer
		void addSSAOGenerateFramebuffer() {

			// -------------------------------------------------------------------------------
			// -------------------------------------------------------------------------------
			// Options

			glm::uvec2 size = glm::uvec2(1024);
			std::vector<vk::Format> colorFormats = std::vector<vk::Format>{ {
					vk::Format::eR16G16B16A16Sfloat,
					vk::Format::eR16G16B16A16Sfloat,
					vk::Format::eR8G8B8A8Unorm
			}};
			// This value is chosen as an invalid default that signals that the code should pick a specific depth buffer
			// Alternative, you can set this to undefined to explicitly declare you want no depth buffer.
			vk::Format depthFormat = vk::Format::eR8Uscaled;

			// Find a suitable depth format
			if (depthFormat == vk::Format::eR8Uscaled) {
				depthFormat = vkx::getSupportedDepthFormat(context.physicalDevice);
			}

			vk::ImageUsageFlags attachmentUsage{ vk::ImageUsageFlagBits::eSampled };
			vk::ImageUsageFlags depthAttachmentUsage;
			vk::ImageLayout colorFinalLayout{ vk::ImageLayout::eShaderReadOnlyOptimal };
			vk::ImageLayout depthFinalLayout{ vk::ImageLayout::eUndefined };


			// End of options
			// ---------------------------------------------------------------------------------





			









			vkx::Framebuffer SSAOGenerateFramebuffer;
			SSAOGenerateFramebuffer.device = context.device;


			SSAOGenerateFramebuffer.attachments.resize(1);
			SSAOGenerateFramebuffer.attachments[0] = createAttachment(vk::Format::eR8Unorm, vk::ImageUsageFlagBits::eColorAttachment, this->size.x, this->size.y);


			//// create deferred render pass
			//{

			//	vk::SubpassDescription subpass;
			//	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

			//	std::vector<vk::AttachmentDescription> attachments;
			//	std::vector<vk::AttachmentReference> colorAttachmentReferences;
			//	attachments.resize(colorFormats.size());
			//	colorAttachmentReferences.resize(attachments.size());

			//	// Color attachment
			//	for (size_t i = 0; i < attachments.size(); ++i) {
			//		attachments[i].format = colorFormats[i];
			//		attachments[i].loadOp = vk::AttachmentLoadOp::eClear;
			//		attachments[i].storeOp = colorFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
			//		attachments[i].initialLayout = vk::ImageLayout::eUndefined;
			//		attachments[i].finalLayout = colorFinalLayout;

			//		vk::AttachmentReference& attachmentReference = colorAttachmentReferences[i];
			//		attachmentReference.attachment = i;
			//		attachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

			//		subpass.colorAttachmentCount = colorAttachmentReferences.size();
			//		subpass.pColorAttachments = colorAttachmentReferences.data();
			//	}

			//	// Do we have a depth format?
			//	vk::AttachmentReference depthAttachmentReference;
			//	if (depthFormat != vk::Format::eUndefined) {
			//		vk::AttachmentDescription depthAttachment;
			//		depthAttachment.format = depthFormat;
			//		depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
			//		// We might be using the depth attacment for something, so preserve it if it's final layout is not undefined
			//		depthAttachment.storeOp = depthFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
			//		depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
			//		depthAttachment.finalLayout = depthFinalLayout;
			//		attachments.push_back(depthAttachment);
			//		depthAttachmentReference.attachment = attachments.size() - 1;
			//		depthAttachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			//		subpass.pDepthStencilAttachment = &depthAttachmentReference;
			//	}

			//	std::vector<vk::SubpassDependency> subpassDependencies;
			//	{
			//		if ((colorFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (colorFinalLayout != vk::ImageLayout::eUndefined)) {
			//			// Implicit transition 
			//			vk::SubpassDependency dependency;
			//			dependency.srcSubpass = 0;
			//			dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			//			dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

			//			dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
			//			dependency.dstAccessMask = vkx::accessFlagsForLayout(colorFinalLayout);
			//			dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
			//			subpassDependencies.push_back(dependency);
			//		}

			//		if ((depthFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (depthFinalLayout != vk::ImageLayout::eUndefined)) {
			//			// Implicit transition 
			//			vk::SubpassDependency dependency;
			//			dependency.srcSubpass = 0;
			//			dependency.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			//			dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;

			//			dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
			//			dependency.dstAccessMask = vkx::accessFlagsForLayout(depthFinalLayout);
			//			dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
			//			subpassDependencies.push_back(dependency);
			//		}
			//	}

			//	//if (renderPass) {
			//	//	context.device.destroyRenderPass(renderPass);
			//	//}

			//	vk::RenderPassCreateInfo renderPassInfo;
			//	renderPassInfo.attachmentCount = attachments.size();
			//	renderPassInfo.pAttachments = attachments.data();
			//	renderPassInfo.subpassCount = 1;
			//	renderPassInfo.pSubpasses = &subpass;
			//	renderPassInfo.dependencyCount = subpassDependencies.size();
			//	renderPassInfo.pDependencies = subpassDependencies.data();

			//	SSAOGenerateFramebuffer.renderPass = context.device.createRenderPass(renderPassInfo);
			//}






			// SSAO 
			{
				vk::AttachmentDescription attachmentDescription;
				attachmentDescription.format = SSAOGenerateFramebuffer.attachments[0].format;
				attachmentDescription.samples = vk::SampleCountFlagBits::e1;
				attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
				attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
				attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
				attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
				attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
				attachmentDescription.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

				vk::AttachmentReference colorReference = { 0, vk::ImageLayout::eColorAttachmentOptimal };

				vk::SubpassDescription subpass;
				subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
				subpass.pColorAttachments = &colorReference;
				subpass.colorAttachmentCount = 1;

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

				vk::RenderPassCreateInfo renderPassInfo;
				renderPassInfo.pAttachments = &attachmentDescription;
				renderPassInfo.attachmentCount = 1;
				renderPassInfo.subpassCount = 1;
				renderPassInfo.pSubpasses = &subpass;
				renderPassInfo.dependencyCount = 2;
				renderPassInfo.pDependencies = dependencies.data();
				SSAOGenerateFramebuffer.renderPass = context.device.createRenderPass(renderPassInfo, nullptr);

				vk::FramebufferCreateInfo fbufCreateInfo;
				fbufCreateInfo.renderPass = SSAOGenerateFramebuffer.renderPass;
				fbufCreateInfo.pAttachments = &SSAOGenerateFramebuffer.attachments[0].view;
				fbufCreateInfo.attachmentCount = 1;
				fbufCreateInfo.width = this->size.x;
				fbufCreateInfo.height = this->size.y;
				fbufCreateInfo.layers = 1;
				SSAOGenerateFramebuffer.framebuffer = context.device.createFramebuffer(fbufCreateInfo, nullptr);
			}



















			//// create frame buffer:
			//{


			//	SSAOGenerateFramebuffer.colors.resize(colorFormats.size());

			//	// Color attachment
			//	vk::ImageCreateInfo image;
			//	image.imageType = vk::ImageType::e2D;
			//	image.extent.width = size.x;
			//	image.extent.height = size.y;
			//	image.extent.depth = 1;
			//	image.mipLevels = 1;
			//	image.arrayLayers = 1;
			//	image.samples = vk::SampleCountFlagBits::e1;
			//	image.tiling = vk::ImageTiling::eOptimal;
			//	// vk::Image of the framebuffer is blit source
			//	image.usage = vk::ImageUsageFlagBits::eColorAttachment | attachmentUsage;

			//	vk::ImageViewCreateInfo colorImageView;
			//	colorImageView.viewType = vk::ImageViewType::e2D;
			//	colorImageView.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			//	colorImageView.subresourceRange.levelCount = 1;
			//	colorImageView.subresourceRange.layerCount = 1;

			//	for (size_t i = 0; i < colorFormats.size(); ++i) {
			//		image.format = colorFormats[i];
			//		SSAOGenerateFramebuffer.colors[i] = context.createImage(image, vk::MemoryPropertyFlagBits::eDeviceLocal);
			//		colorImageView.format = colorFormats[i];
			//		colorImageView.image = SSAOGenerateFramebuffer.colors[i].image;
			//		SSAOGenerateFramebuffer.colors[i].view = context.device.createImageView(colorImageView);
			//	}


			//	bool useDepth = depthFormat != vk::Format::eUndefined;
			//	// Depth stencil attachment
			//	if (useDepth) {
			//		image.format = depthFormat;
			//		image.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | depthAttachmentUsage;
			//		SSAOGenerateFramebuffer.depthAttachment = context.createImage(image, vk::MemoryPropertyFlagBits::eDeviceLocal);

			//		vk::ImageViewCreateInfo depthStencilView;
			//		depthStencilView.viewType = vk::ImageViewType::e2D;
			//		depthStencilView.format = depthFormat;
			//		depthStencilView.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
			//		depthStencilView.subresourceRange.levelCount = 1;
			//		depthStencilView.subresourceRange.layerCount = 1;
			//		depthStencilView.image = SSAOGenerateFramebuffer.depthAttachment.image;
			//		SSAOGenerateFramebuffer.depthAttachment.view = context.device.createImageView(depthStencilView);

			//	}

			//	std::vector<vk::ImageView> attachments;
			//	attachments.resize(SSAOGenerateFramebuffer.colors.size());
			//	for (size_t i = 0; i < SSAOGenerateFramebuffer.colors.size(); ++i) {
			//		attachments[i] = SSAOGenerateFramebuffer.colors[i].view;
			//	}
			//	if (useDepth) {
			//		attachments.push_back(SSAOGenerateFramebuffer.depthAttachment.view);
			//	}

			//	vk::FramebufferCreateInfo fbufCreateInfo;
			//	fbufCreateInfo.renderPass = SSAOGenerateFramebuffer.renderPass;
			//	fbufCreateInfo.attachmentCount = (uint32_t)attachments.size();
			//	fbufCreateInfo.pAttachments = attachments.data();
			//	fbufCreateInfo.width = size.x;
			//	fbufCreateInfo.height = size.y;
			//	fbufCreateInfo.layers = 1;
			//	SSAOGenerateFramebuffer.framebuffer = context.device.createFramebuffer(fbufCreateInfo);
			//}
















			framebuffers.push_back(SSAOGenerateFramebuffer);
		}




























		void addSSAOBlurFramebuffer() {



			vkx::Framebuffer SSAOBlurFramebuffer;
			SSAOBlurFramebuffer.device = context.device;




			SSAOBlurFramebuffer.attachments.resize(1);
			SSAOBlurFramebuffer.attachments[0] = createAttachment(vk::Format::eR8Unorm, vk::ImageUsageFlagBits::eColorAttachment, this->size.x, this->size.y);


			// SSAO Blur 
			{
				vk::AttachmentDescription attachmentDescription;
				attachmentDescription.format = SSAOBlurFramebuffer.attachments[0].format;
				attachmentDescription.samples = vk::SampleCountFlagBits::e1;
				attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
				attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
				attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
				attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
				attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
				attachmentDescription.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

				vk::AttachmentReference colorReference = { 0, vk::ImageLayout::eColorAttachmentOptimal };

				vk::SubpassDescription subpass;
				subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
				subpass.pColorAttachments = &colorReference;
				subpass.colorAttachmentCount = 1;

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

				vk::RenderPassCreateInfo renderPassInfo;
				renderPassInfo.pAttachments = &attachmentDescription;
				renderPassInfo.attachmentCount = 1;
				renderPassInfo.subpassCount = 1;
				renderPassInfo.pSubpasses = &subpass;
				renderPassInfo.dependencyCount = 2;
				renderPassInfo.pDependencies = dependencies.data();
				SSAOBlurFramebuffer.renderPass = context.device.createRenderPass(renderPassInfo, nullptr);

				vk::FramebufferCreateInfo fbufCreateInfo;
				fbufCreateInfo.renderPass = SSAOBlurFramebuffer.renderPass;
				fbufCreateInfo.pAttachments = &SSAOBlurFramebuffer.attachments[0].view;
				fbufCreateInfo.attachmentCount = 1;
				fbufCreateInfo.width = this->size.x;
				fbufCreateInfo.height = this->size.y;
				fbufCreateInfo.layers = 1;
				SSAOBlurFramebuffer.framebuffer = context.device.createFramebuffer(fbufCreateInfo, nullptr);
			}












			framebuffers.push_back(SSAOBlurFramebuffer);
		}




















		//// Prepare a new framebuffer for offscreen rendering
		//// The contents of this framebuffer are then
		//// blitted to our render target
		//void prepareOffscreenFramebuffers() {


		//	// 3 attachments for position, specular and albedo
		//	SSAOFrameBuffers.offscreen.attachments.resize(3);
		//	// 1 attachment for SSAO
		//	SSAOFrameBuffers.ssaoGenerate.attachments.resize(1);
		//	// 1 attachment for SSAO Blur
		//	SSAOFrameBuffers.ssaoBlur.attachments.resize(1);

		//	//vk::CommandBuffer layoutCmd = vkx::VulkanApp::createCommandBuffer(vk::CommandBufferLevel::ePrimary, true);

		//	#if defined(__ANDROID__)
		//	const uint32_t ssaoWidth = width / 2;
		//	const uint32_t ssaoHeight = height / 2;
		//	#else
		//	const uint32_t ssaoWidth = this->size.x;
		//	const uint32_t ssaoHeight = this->size.y;
		//	#endif

		//	uint32_t width = 1280;
		//	uint32_t height = 720;


		//	//SSAOFrameBuffers.offscreen.setSize(width, height);
		//	//SSAOFrameBuffers.ssaoGenerate.setSize(ssaoWidth, ssaoHeight);
		//	//SSAOFrameBuffers.ssaoBlur.setSize(width, height);

		//	// Offscreen framebuffer, Color attachments

		//	// Attachment 0: World space positions
		//	SSAOFrameBuffers.offscreen.attachments[0] = createAttachment(vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eColorAttachment, /*&SSAOFrameBuffers.offscreen.attachments[0],*/ /*layoutCmd,*/ width, height);

		//	// Attachment 1: World space normal
		//	SSAOFrameBuffers.offscreen.attachments[1] = createAttachment(vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment, /*&SSAOFrameBuffers.offscreen.attachments[1],*/ /*layoutCmd,*/ width, height);

		//	// Attachment 1: Packed colors, specular
		//	SSAOFrameBuffers.offscreen.attachments[2] = createAttachment(vk::Format::eR32G32B32A32Uint, vk::ImageUsageFlagBits::eColorAttachment, /*&SSAOFrameBuffers.offscreen.attachments[2],*/ /*layoutCmd,*/ width, height);


		//	// Offscreen depth attachment:

		//	// Find a suitable depth format
		//	if (depthFormat == vk::Format::eR8Uscaled) {
		//		depthFormat = vkx::getSupportedDepthFormat(context.physicalDevice);
		//	}

		//	createAttachment(depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment, /*&SSAOFrameBuffers.offscreen.depthAttachment,*/ /*layoutCmd,*/ width, height);

		//	// framebuffer

		//	// SSAO
		//	SSAOFrameBuffers.ssaoGenerate.attachments[0] = createAttachment(vk::Format::eR8Unorm, vk::ImageUsageFlagBits::eColorAttachment, /*&SSAOFrameBuffers.ssaoGenerate.attachments[0],*/ /*layoutCmd,*/ ssaoWidth, ssaoHeight);// Color																																				
		//	// SSAO blur
		//	SSAOFrameBuffers.ssaoBlur.attachments[0] = createAttachment(vk::Format::eR8Unorm, vk::ImageUsageFlagBits::eColorAttachment, /*&SSAOFrameBuffers.ssaoBlur.attachments[0],*/ /*layoutCmd,*/ width, height);// Color

		//	//VulkanExampleBase::flushCommandBuffer(layoutCmd, queue, true);

		//	// G-Buffer creation
		//	{
		//		std::array<vk::AttachmentDescription, 4> attachmentDescs = {};

		//		// Init attachment properties
		//		for (uint32_t i = 0; i < static_cast<uint32_t>(attachmentDescs.size()); i++) {
		//			attachmentDescs[i].samples = vk::SampleCountFlagBits::e1;
		//			attachmentDescs[i].loadOp = vk::AttachmentLoadOp::eClear;
		//			attachmentDescs[i].storeOp = vk::AttachmentStoreOp::eStore;
		//			attachmentDescs[i].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		//			attachmentDescs[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		//			attachmentDescs[i].finalLayout = (i == 3) ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
		//		}

		//		// Formats
		//		attachmentDescs[0].format = SSAOFrameBuffers.offscreen.attachments[0].format;
		//		attachmentDescs[1].format = SSAOFrameBuffers.offscreen.attachments[1].format;
		//		attachmentDescs[2].format = SSAOFrameBuffers.offscreen.attachments[2].format;
		//		attachmentDescs[3].format = SSAOFrameBuffers.offscreen.depthAttachment.format;

		//		// color attachment references
		//		std::vector<vk::AttachmentReference> colorReferences;
		//		colorReferences.push_back({ 0, vk::ImageLayout::eColorAttachmentOptimal });
		//		colorReferences.push_back({ 1, vk::ImageLayout::eColorAttachmentOptimal });
		//		colorReferences.push_back({ 2, vk::ImageLayout::eColorAttachmentOptimal });

		//		// depth reference
		//		vk::AttachmentReference depthReference;
		//		depthReference.attachment = 3;
		//		depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		//		vk::SubpassDescription subpass;
		//		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		//		subpass.pColorAttachments = colorReferences.data();
		//		subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
		//		subpass.pDepthStencilAttachment = &depthReference;

		//		// Use subpass dependencies for attachment layout transitions
		//		std::array<vk::SubpassDependency, 2> dependencies;

		//		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		//		dependencies[0].dstSubpass = 0;
		//		dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		//		dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		//		dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
		//		dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		//		dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		//		dependencies[1].srcSubpass = 0;
		//		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		//		dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		//		dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		//		dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		//		dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
		//		dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		//		// create offscreen render pass subpass?
		//		vk::RenderPassCreateInfo renderPassInfo;
		//		renderPassInfo.pAttachments = attachmentDescs.data();
		//		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
		//		renderPassInfo.subpassCount = 1;
		//		renderPassInfo.pSubpasses = &subpass;
		//		renderPassInfo.dependencyCount = 2;
		//		renderPassInfo.pDependencies = dependencies.data();
		//		SSAOFrameBuffers.offscreen.renderPass = context.device.createRenderPass(renderPassInfo, nullptr);

		//		std::vector<vk::ImageView> attachments;
		//		attachments.resize(3);
		//		for (size_t i = 0; i < 3; ++i) {
		//			attachments[i] = SSAOFrameBuffers.offscreen.attachments[i].view;// color attachments
		//		}
		//		attachments.push_back(SSAOFrameBuffers.offscreen.depthAttachment.view);// depth attachment

		//		vk::FramebufferCreateInfo fbufCreateInfo;/* = vkTools::initializers::framebufferCreateInfo();*/
		//		fbufCreateInfo.renderPass = SSAOFrameBuffers.offscreen.renderPass;
		//		fbufCreateInfo.pAttachments = attachments.data();
		//		fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		//		fbufCreateInfo.width = this->size.x;
		//		fbufCreateInfo.height = this->size.y;
		//		fbufCreateInfo.layers = 1;
		//		SSAOFrameBuffers.offscreen.framebuffer = context.device.createFramebuffer(fbufCreateInfo, nullptr);
		//	}





		//	// SSAO 
		//	{
		//		vk::AttachmentDescription attachmentDescription;
		//		attachmentDescription.format = SSAOFrameBuffers.ssaoGenerate.attachments[0].format;
		//		attachmentDescription.samples = vk::SampleCountFlagBits::e1;
		//		attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
		//		attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
		//		attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		//		attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		//		attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
		//		attachmentDescription.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		//		vk::AttachmentReference colorReference = { 0, vk::ImageLayout::eColorAttachmentOptimal };

		//		vk::SubpassDescription subpass;
		//		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		//		subpass.pColorAttachments = &colorReference;
		//		subpass.colorAttachmentCount = 1;

		//		std::array<vk::SubpassDependency, 2> dependencies;

		//		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		//		dependencies[0].dstSubpass = 0;
		//		dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		//		dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		//		dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
		//		dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		//		dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		//		dependencies[1].srcSubpass = 0;
		//		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		//		dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		//		dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		//		dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		//		dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
		//		dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		//		vk::RenderPassCreateInfo renderPassInfo;
		//		renderPassInfo.pAttachments = &attachmentDescription;
		//		renderPassInfo.attachmentCount = 1;
		//		renderPassInfo.subpassCount = 1;
		//		renderPassInfo.pSubpasses = &subpass;
		//		renderPassInfo.dependencyCount = 2;
		//		renderPassInfo.pDependencies = dependencies.data();
		//		SSAOFrameBuffers.ssaoGenerate.renderPass = context.device.createRenderPass(renderPassInfo, nullptr);

		//		vk::FramebufferCreateInfo fbufCreateInfo;/* = vkTools::initializers::framebufferCreateInfo();*/
		//		fbufCreateInfo.renderPass = SSAOFrameBuffers.ssaoGenerate.renderPass;
		//		fbufCreateInfo.pAttachments = &SSAOFrameBuffers.ssaoGenerate.attachments[0].view;
		//		fbufCreateInfo.attachmentCount = 1;
		//		fbufCreateInfo.width = this->size.x;
		//		fbufCreateInfo.height = this->size.y;
		//		fbufCreateInfo.layers = 1;
		//		SSAOFrameBuffers.ssaoGenerate.framebuffer = context.device.createFramebuffer(fbufCreateInfo, nullptr);
		//	}





		//	// SSAO Blur 
		//	{
		//		vk::AttachmentDescription attachmentDescription;
		//		attachmentDescription.format = SSAOFrameBuffers.ssaoBlur.attachments[0].format;
		//		attachmentDescription.samples = vk::SampleCountFlagBits::e1;
		//		attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
		//		attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
		//		attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		//		attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		//		attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
		//		attachmentDescription.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		//		vk::AttachmentReference colorReference = { 0, vk::ImageLayout::eColorAttachmentOptimal };

		//		vk::SubpassDescription subpass;
		//		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		//		subpass.pColorAttachments = &colorReference;
		//		subpass.colorAttachmentCount = 1;

		//		std::array<vk::SubpassDependency, 2> dependencies;

		//		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		//		dependencies[0].dstSubpass = 0;
		//		dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		//		dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		//		dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
		//		dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		//		dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		//		dependencies[1].srcSubpass = 0;
		//		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		//		dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		//		dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		//		dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		//		dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
		//		dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		//		vk::RenderPassCreateInfo renderPassInfo;
		//		renderPassInfo.pAttachments = &attachmentDescription;
		//		renderPassInfo.attachmentCount = 1;
		//		renderPassInfo.subpassCount = 1;
		//		renderPassInfo.pSubpasses = &subpass;
		//		renderPassInfo.dependencyCount = 2;
		//		renderPassInfo.pDependencies = dependencies.data();
		//		//VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &frameBuffers.ssaoBlur.renderPass));
		//		SSAOFrameBuffers.ssaoBlur.renderPass = context.device.createRenderPass(renderPassInfo, nullptr);

		//		vk::FramebufferCreateInfo fbufCreateInfo;/* = vkTools::initializers::framebufferCreateInfo();*/
		//		fbufCreateInfo.renderPass = SSAOFrameBuffers.ssaoBlur.renderPass;
		//		fbufCreateInfo.pAttachments = &SSAOFrameBuffers.ssaoBlur.attachments[0].view;
		//		fbufCreateInfo.attachmentCount = 1;
		//		fbufCreateInfo.width = this->size.x;
		//		fbufCreateInfo.height = this->size.y;
		//		fbufCreateInfo.layers = 1;
		//		//VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &frameBuffers.ssaoBlur.frameBuffer));
		//		SSAOFrameBuffers.ssaoBlur.framebuffer = context.device.createFramebuffer(fbufCreateInfo, nullptr);
		//	}

		//	// Shared sampler for color attachments
		//	vk::SamplerCreateInfo samplerInfo;/* = vkTools::initializers::samplerCreateInfo();*/
		//	samplerInfo.magFilter = vk::Filter::eLinear;
		//	samplerInfo.minFilter = vk::Filter::eLinear;
		//	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		//	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
		//	samplerInfo.addressModeV = samplerInfo.addressModeU;
		//	samplerInfo.addressModeW = samplerInfo.addressModeU;
		//	samplerInfo.mipLodBias = 0.0f;
		//	samplerInfo.maxAnisotropy = 0;
		//	samplerInfo.minLod = 0.0f;
		//	samplerInfo.maxLod = 1.0f;
		//	samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
		//	//VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &colorSampler));
		//	SSAOFrameBuffers.ssaoBlur.attachments[0].sampler = context.device.createSampler(samplerInfo, nullptr);
		//	//SSAOFrameBuffers.ssaoBlur.attachments[0].sampler;
		//	//SSAOFrameBuffers.ssaoBlur.attachments[0].sampler;
		//	
		//	//colorSampler = context.device.createSampler(sampler, nullptr);
		//}



























		protected:
		//void prepareSampler() {
		//	// Create sampler
		//	vk::SamplerCreateInfo sampler;
		//	sampler.magFilter = vk::Filter::eLinear;
		//	sampler.minFilter = vk::Filter::eLinear;
		//	sampler.mipmapMode = vk::SamplerMipmapMode::eLinear;
		//	sampler.addressModeU = vk::SamplerAddressMode::eClampToEdge;
		//	sampler.addressModeV = sampler.addressModeU;
		//	sampler.addressModeW = sampler.addressModeU;
		//	sampler.mipLodBias = 0.0f;
		//	sampler.maxAnisotropy = 0;
		//	sampler.compareOp = vk::CompareOp::eNever;
		//	sampler.minLod = 0.0f;
		//	sampler.maxLod = 0.0f;
		//	sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
		//	for (auto &framebuffer : framebuffers) {
		//		if (attachmentUsage | vk::ImageUsageFlagBits::eSampled) {
		//			for (auto& color : framebuffer.colors) {
		//				color.sampler = context.device.createSampler(sampler);
		//			}
		//		}
		//		if (depthAttachmentUsage | vk::ImageUsageFlagBits::eSampled) {
		//			framebuffer.depthAttachment.sampler = context.device.createSampler(sampler);
		//		}
		//	}
		//}

		//virtual void prepareRenderPass() {

		//	vk::SubpassDescription subpass;
		//	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

		//	std::vector<vk::AttachmentDescription> attachments;
		//	std::vector<vk::AttachmentReference> colorAttachmentReferences;
		//	attachments.resize(colorFormats.size());
		//	colorAttachmentReferences.resize(attachments.size());
		//	// Color attachment
		//	for (size_t i = 0; i < attachments.size(); ++i) {
		//		attachments[i].format = colorFormats[i];
		//		attachments[i].loadOp = vk::AttachmentLoadOp::eClear;
		//		attachments[i].storeOp = colorFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
		//		attachments[i].initialLayout = vk::ImageLayout::eUndefined;
		//		attachments[i].finalLayout = colorFinalLayout;

		//		vk::AttachmentReference& attachmentReference = colorAttachmentReferences[i];
		//		attachmentReference.attachment = i;
		//		attachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

		//		subpass.colorAttachmentCount = colorAttachmentReferences.size();
		//		subpass.pColorAttachments = colorAttachmentReferences.data();
		//	}

		//	// Do we have a depth format?
		//	vk::AttachmentReference depthAttachmentReference;
		//	if (depthFormat != vk::Format::eUndefined) {
		//		vk::AttachmentDescription depthAttachment;
		//		depthAttachment.format = depthFormat;
		//		depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		//		// We might be using the depth attacment for something, so preserve it if it's final layout is not undefined
		//		depthAttachment.storeOp = this->depthFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
		//		depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
		//		depthAttachment.finalLayout = this->depthFinalLayout;
		//		attachments.push_back(depthAttachment);
		//		depthAttachmentReference.attachment = attachments.size() - 1;
		//		depthAttachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		//		subpass.pDepthStencilAttachment = &depthAttachmentReference;
		//	}

		//	std::vector<vk::SubpassDependency> subpassDependencies;
		//	{
		//		if ((colorFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (colorFinalLayout != vk::ImageLayout::eUndefined)) {
		//			// Implicit transition 
		//			vk::SubpassDependency dependency;
		//			dependency.srcSubpass = 0;
		//			dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		//			dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		//			dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		//			dependency.dstAccessMask = vkx::accessFlagsForLayout(colorFinalLayout);
		//			dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		//			subpassDependencies.push_back(dependency);
		//		}

		//		if ((depthFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (depthFinalLayout != vk::ImageLayout::eUndefined)) {
		//			// Implicit transition 
		//			vk::SubpassDependency dependency;
		//			dependency.srcSubpass = 0;
		//			dependency.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		//			dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;

		//			dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		//			dependency.dstAccessMask = vkx::accessFlagsForLayout(depthFinalLayout);
		//			dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		//			subpassDependencies.push_back(dependency);
		//		}
		//	}

		//	if (renderPass) {
		//		context.device.destroyRenderPass(renderPass);
		//	}

		//	vk::RenderPassCreateInfo renderPassInfo;
		//	renderPassInfo.attachmentCount = attachments.size();
		//	renderPassInfo.pAttachments = attachments.data();
		//	renderPassInfo.subpassCount = 1;
		//	renderPassInfo.pSubpasses = &subpass;
		//	renderPassInfo.dependencyCount = subpassDependencies.size();
		//	renderPassInfo.pDependencies = subpassDependencies.data();
		//	
		//	renderPass = context.device.createRenderPass(renderPassInfo);
		//	//framebuffers[0].renderPass = context.device.createRenderPass(renderPassInfo);
		//}



		//void prepareRenderPasses() {

		//	for (int i = 0; i < framebuffers.size(); ++i) {
		//		//auto &framebuffer = framebuffers[i];

		//		vk::SubpassDescription subpass;
		//		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

		//		std::vector<vk::AttachmentDescription> attachmentDescs;
		//		std::vector<vk::AttachmentReference> colorAttachmentReferences;
		//		attachmentDescs.resize(colorFormats.size());
		//		colorAttachmentReferences.resize(attachmentDescs.size());

		//		// Color attachment
		//		for (size_t i = 0; i < attachmentDescs.size(); ++i) {
		//			attachmentDescs[i].format = colorFormats[i];
		//			attachmentDescs[i].loadOp = vk::AttachmentLoadOp::eClear;
		//			attachmentDescs[i].storeOp = colorFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
		//			attachmentDescs[i].initialLayout = vk::ImageLayout::eUndefined;
		//			attachmentDescs[i].finalLayout = colorFinalLayout;

		//			vk::AttachmentReference& attachmentReference = colorAttachmentReferences[i];
		//			attachmentReference.attachment = i;
		//			attachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

		//			subpass.colorAttachmentCount = colorAttachmentReferences.size();
		//			subpass.pColorAttachments = colorAttachmentReferences.data();
		//		}

		//		// Do we have a depth format?
		//		vk::AttachmentReference depthAttachmentReference;
		//		if (depthFormat != vk::Format::eUndefined) {
		//			vk::AttachmentDescription depthAttachment;
		//			depthAttachment.format = depthFormat;
		//			depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		//			// We might be using the depth attacment for something, so preserve it if it's final layout is not undefined
		//			depthAttachment.storeOp = depthFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
		//			depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
		//			depthAttachment.finalLayout = depthFinalLayout;
		//			attachmentDescs.push_back(depthAttachment);
		//			depthAttachmentReference.attachment = attachmentDescs.size() - 1;
		//			depthAttachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		//			subpass.pDepthStencilAttachment = &depthAttachmentReference;
		//		}

		//		std::vector<vk::SubpassDependency> subpassDependencies;
		//		{
		//			if ((colorFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (colorFinalLayout != vk::ImageLayout::eUndefined)) {
		//				// Implicit transition 
		//				vk::SubpassDependency dependency;
		//				dependency.srcSubpass = 0;
		//				dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		//				dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		//				dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		//				dependency.dstAccessMask = vkx::accessFlagsForLayout(colorFinalLayout);
		//				dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		//				subpassDependencies.push_back(dependency);
		//			}

		//			if ((depthFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (depthFinalLayout != vk::ImageLayout::eUndefined)) {
		//				// Implicit transition 
		//				vk::SubpassDependency dependency;
		//				dependency.srcSubpass = 0;
		//				dependency.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		//				dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;

		//				dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		//				dependency.dstAccessMask = vkx::accessFlagsForLayout(depthFinalLayout);
		//				dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		//				subpassDependencies.push_back(dependency);
		//			}
		//		}

		//		//if (renderPass) {
		//		//	context.device.destroyRenderPass(renderPass);
		//		//}

		//		vk::RenderPassCreateInfo renderPassInfo;
		//		renderPassInfo.attachmentCount = attachmentDescs.size();
		//		renderPassInfo.pAttachments = attachmentDescs.data();
		//		renderPassInfo.subpassCount = 1;
		//		renderPassInfo.pSubpasses = &subpass;
		//		renderPassInfo.dependencyCount = subpassDependencies.size();
		//		renderPassInfo.pDependencies = subpassDependencies.data();
		//		framebuffers[i].renderPass = context.device.createRenderPass(renderPassInfo);


		//	}
		//}





	};
}