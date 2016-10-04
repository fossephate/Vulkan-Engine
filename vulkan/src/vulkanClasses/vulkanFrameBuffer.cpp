#include "vulkanFrameBuffer.h"

/**
* @brief Returns true if the attachment has a depth component
*/

bool vkx::FramebufferAttachment::hasDepth()
{
	std::vector<vk::Format> formats =
	{
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

bool vkx::FramebufferAttachment::hasStencil()
{
	std::vector<vk::Format> formats =
	{
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

bool vkx::FramebufferAttachment::isDepthStencil()
{
	return(hasDepth() || hasStencil());
}

/**
* Default constructor
*
* @param vulkanDevice Pointer to a valid VulkanDevice
*/

vkx::Framebuffer::Framebuffer(vkx::VulkanDevice * vulkanDevice)
{
	assert(vulkanDevice);
	this->vulkanDevice = vulkanDevice;
}

/**
* Destroy and free Vulkan resources used for the framebuffer and all of it's attachments
*/

vkx::Framebuffer::~Framebuffer()
{
	assert(vulkanDevice);
	for (auto attachment : attachments)
	{
		//vkDestroyImage(vulkanDevice->logicalDevice, attachment.image, nullptr);
		vk::Device (vulkanDevice->logicalDevice).destroyImage(attachment.image, nullptr);
		//vkDestroyImageView(vulkanDevice->logicalDevice, attachment.view, nullptr);
		vk::Device(vulkanDevice->logicalDevice).destroyImageView(attachment.view, nullptr);
		//vkFreeMemory(vulkanDevice->logicalDevice, attachment.memory, nullptr);
		vk::Device(vulkanDevice->logicalDevice).freeMemory(attachment.memory, nullptr);
	}
	//vkDestroySampler(vulkanDevice->logicalDevice, sampler, nullptr);
	vk::Device(vulkanDevice->logicalDevice).destroySampler(sampler, nullptr);
	//vkDestroyRenderPass(vulkanDevice->logicalDevice, renderPass, nullptr);
	vk::Device(vulkanDevice->logicalDevice).destroyRenderPass(renderPass, nullptr);
	//vkDestroyFramebuffer(vulkanDevice->logicalDevice, framebuffer, nullptr);
	vk::Device(vulkanDevice->logicalDevice).destroyFramebuffer(framebuffer, nullptr);
}

/**
* Add a new attachment described by createinfo to the framebuffer's attachment list
*
* @param createinfo Structure that specifices the framebuffer to be constructed
*
* @return Index of the new attachment
*/

uint32_t vkx::Framebuffer::addAttachment(vkx::AttachmentCreateInfo createinfo)
{
	vkx::FramebufferAttachment attachment;

	attachment.format = createinfo.format;

	vk::ImageAspectFlags aspectMask = VK_FLAGS_NONE;
	vk::ImageLayout imageLayout;

	// Select aspect mask and layout depending on usage

	// Color attachment
	if (createinfo.usage & vk::ImageUsageFlagBits::eColorAttachment)
	{
		aspectMask = vk::ImageAspectFlagBits::eColor;
		attachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
		imageLayout = (createinfo.usage & vk::ImageUsageFlagBits::eSampled) ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eColorAttachmentOptimal;
	}

	// Depth (and/or stencil) attachment
	if (createinfo.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment)
	{
		if (attachment.hasDepth())
		{
			aspectMask = vk::ImageAspectFlagBits::eDepth;
		}
		if (attachment.hasStencil())
		{
			aspectMask = aspectMask | vk::ImageAspectFlagBits::eStencil;
		}
		attachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		imageLayout = (createinfo.usage & vk::ImageUsageFlagBits::eSampled) ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eDepthStencilAttachmentOptimal;
	}

	assert(aspectMask > 0);

	vk::ImageCreateInfo image;
	image.imageType = vk::ImageType::e2D;
	image.format = createinfo.format;
	image.extent.width = createinfo.width;
	image.extent.height = createinfo.height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = createinfo.layerCount;
	image.samples = vk::SampleCountFlagBits::e1;
	image.tiling = vk::ImageTiling::eOptimal;
	image.usage = createinfo.usage;

	vk::MemoryAllocateInfo memAlloc;
	vk::MemoryRequirements memReqs;

	// Create image for this attachment
	//VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &image, nullptr, &attachment.image));
	vk::Device(vulkanDevice->logicalDevice).createImage(&image, nullptr, &attachment.image);

	//vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, attachment.image, &memReqs);
	vk::Device(vulkanDevice->logicalDevice).getImageMemoryRequirements(attachment.image, &memReqs);

	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAlloc, nullptr, &attachment.memory));
	vk::Device(vulkanDevice->logicalDevice).allocateMemory(&memAlloc, nullptr, &attachment.memory);
	//VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, attachment.image, attachment.memory, 0));
	vk::Device(vulkanDevice->logicalDevice).bindImageMemory(attachment.image, attachment.memory, 0);

	attachment.subresourceRange = {};
	attachment.subresourceRange.aspectMask = aspectMask;
	attachment.subresourceRange.levelCount = 1;
	attachment.subresourceRange.layerCount = createinfo.layerCount;

	vk::ImageViewCreateInfo imageView;
	imageView.viewType = (createinfo.layerCount == 1) ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray;
	imageView.format = createinfo.format;
	imageView.subresourceRange = attachment.subresourceRange;
	//todo: workaround for depth+stencil attachments
	imageView.subresourceRange.aspectMask = (attachment.hasDepth()) ? vk::ImageAspectFlagBits::eDepth : aspectMask;
	imageView.image = attachment.image;
	//VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &imageView, nullptr, &attachment.view));
	vk::Device(vulkanDevice->logicalDevice).createImageView(&imageView, nullptr, &attachment.view);

	// Fill attachment description
	attachment.description = {};
	attachment.description.samples = vk::SampleCountFlagBits::e1;
	attachment.description.loadOp = vk::AttachmentLoadOp::eClear;
	attachment.description.storeOp = (createinfo.usage & vk::ImageUsageFlagBits::eSampled) ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare;
	attachment.description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachment.description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachment.description.format = createinfo.format;
	attachment.description.initialLayout = vk::ImageLayout::eUndefined;
	// Final layout
	if ((createinfo.usage & vk::ImageUsageFlagBits::eSampled))
	{
		// If sampled, final layout is always SHADER_READ
		attachment.description.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	} else
	{
		// If not, final layout depends on attachment type
		if (attachment.hasDepth() || attachment.hasStencil())
		{
			attachment.description.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		} else
		{
			attachment.description.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
		}
	}

	attachments.push_back(attachment);

	return static_cast<uint32_t>(attachments.size() - 1);
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

vk::Result vkx::Framebuffer::createSampler(vk::Filter magFilter, vk::Filter minFilter, vk::SamplerAddressMode adressMode)
{
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
	//return vkCreateSampler(vulkanDevice->logicalDevice, &samplerInfo, nullptr, &sampler);
	vk::Result res = vk::Device(vulkanDevice->logicalDevice).createSampler(&samplerInfo, nullptr, &sampler);
	return res;
}

/**
* Creates a default render pass setup with one sub pass
*
* @return VK_SUCCESS if all resources have been created successfully
*/

vk::Result vkx::Framebuffer::createRenderPass()
{
	std::vector<vk::AttachmentDescription> attachmentDescriptions;
	for (auto& attachment : attachments)
	{
		attachmentDescriptions.push_back(attachment.description);
	};

	// Collect attachment references
	std::vector<vk::AttachmentReference> colorReferences;
	vk::AttachmentReference depthReference;
	bool hasDepth = false;
	bool hasColor = false;

	uint32_t attachmentIndex = 0;

	for (auto& attachment : attachments)
	{
		if (attachment.isDepthStencil())
		{
			// Only one depth attachment allowed
			assert(!hasDepth);
			depthReference.attachment = attachmentIndex;
			depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			hasDepth = true;
		} else
		{
			colorReferences.push_back({ attachmentIndex, vk::ImageLayout::eColorAttachmentOptimal });
			hasColor = true;
		}
		attachmentIndex++;
	};

	// Default render pass setup uses only one subpass
	vk::SubpassDescription subpass;
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
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create render pass
	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies.data();
	//VK_CHECK_RESULT(vkCreateRenderPass(vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &renderPass));
	vk::Device(vulkanDevice->logicalDevice).createRenderPass(&renderPassInfo, nullptr, &renderPass);

	std::vector<vk::ImageView> attachmentViews;
	for (auto attachment : attachments)
	{
		attachmentViews.push_back(attachment.view);
	}

	// Find. max number of layers across attachments
	uint32_t maxLayers = 0;
	for (auto attachment : attachments)
	{
		if (attachment.subresourceRange.layerCount > maxLayers)
		{
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
	vk::Device(vulkanDevice->logicalDevice).createFramebuffer(&framebufferInfo, nullptr, &framebuffer);

	return vk::Result::eSuccess;
}
