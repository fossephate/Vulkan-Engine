#include "vulkanTextOverlay.h"

/**
* Default constructor
*
* @param vulkanDevice Pointer to a valid VulkanDevice
*/

vkx::VulkanTextOverlay::VulkanTextOverlay(vkx::VulkanDevice * vulkanDevice, vk::Queue queue, std::vector<vk::Framebuffer>& framebuffers, vk::Format colorformat, vk::Format depthformat, uint32_t * framebufferwidth, uint32_t * framebufferheight, std::vector<vk::PipelineShaderStageCreateInfo> shaderstages)
{
	this->vulkanDevice = vulkanDevice;
	this->queue = queue;
	this->colorFormat = colorformat;
	this->depthFormat = depthformat;

	this->frameBuffers.resize(framebuffers.size());
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		this->frameBuffers[i] = &framebuffers[i];
	}

	this->shaderStages = shaderstages;

	this->frameBufferWidth = framebufferwidth;
	this->frameBufferHeight = framebufferheight;

	cmdBuffers.resize(framebuffers.size());
	prepareResources();
	prepareRenderPass();
	preparePipeline();
}

/**
* Default destructor, frees up all Vulkan resources acquired by the text overlay
*/

vkx::VulkanTextOverlay::~VulkanTextOverlay()
{
	// Free up all Vulkan resources requested by the text overlay
	vertexBuffer.destroy();
	vk::Device &ld = vk::Device (vulkanDevice->logicalDevice);
	ld.destroySampler(sampler, nullptr);
	ld.destroyImage(image, nullptr);
	ld.freeMemory(imageMemory, nullptr);
	ld.destroyDescriptorSetLayout(descriptorSetLayout, nullptr);
	ld.destroyDescriptorPool(descriptorPool, nullptr);
	ld.destroyPipelineLayout(pipelineLayout, nullptr);
	ld.destroyPipelineCache(pipelineCache, nullptr);
	ld.destroyPipeline(pipeline, nullptr);
	ld.destroyRenderPass(renderPass, nullptr);
	ld.freeCommandBuffers(commandPool, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
	ld.destroyCommandPool(commandPool, nullptr);
	ld.destroyFence(fence, nullptr);
}

/**
* Prepare all vulkan resources required to render the font
* The text overlay uses separate resources for descriptors (pool, sets, layouts), pipelines and command buffers
*/

void vkx::VulkanTextOverlay::prepareResources()
{
	static unsigned char font24pixels[STB_FONT_HEIGHT][STB_FONT_WIDTH];
	STB_FONT_NAME(stbFontData, font24pixels, STB_FONT_HEIGHT);

	// Command buffer

	// Pool
	vk::CommandPoolCreateInfo cmdPoolInfo;
	cmdPoolInfo.queueFamilyIndex = vulkanDevice->queueFamilyIndices.graphics;
	cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	//VK_CHECK_RESULT(vkCreateCommandPool(vulkanDevice->logicalDevice, &cmdPoolInfo, nullptr, &commandPool));
	vk::Device(vulkanDevice->logicalDevice).createCommandPool(&cmdPoolInfo, nullptr, &commandPool);

	vk::CommandBufferAllocateInfo cmdBufAllocateInfo =
		vkx::commandBufferAllocateInfo(
			commandPool,
			vk::CommandBufferLevel::ePrimary,
			(uint32_t)cmdBuffers.size());

	//VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice->logicalDevice, &cmdBufAllocateInfo, cmdBuffers.data()));
	vk::Device(vulkanDevice->logicalDevice).allocateCommandBuffers(&cmdBufAllocateInfo, cmdBuffers.data());

	// Vertex buffer
	/*VK_CHECK_RESULT(vulkanDevice->createBuffer(
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		&vertexBuffer,
		MAX_CHAR_COUNT * sizeof(glm::vec4)));*/
	vulkanDevice->createBuffer(
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		&vertexBuffer,
		MAX_CHAR_COUNT * sizeof(glm::vec4));

	// Map persistent
	vertexBuffer.map();

	// Font texture
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.format = vk::Format::eR8Unorm;
	imageInfo.extent.width = STB_FONT_WIDTH;
	imageInfo.extent.height = STB_FONT_HEIGHT;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.initialLayout = vk::ImageLayout::ePreinitialized;
	//VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &imageInfo, nullptr, &image));
	vk::Device(vulkanDevice->logicalDevice).createImage(&imageInfo, nullptr, &image);

	vk::MemoryRequirements memReqs;
	vk::MemoryAllocateInfo allocInfo;
	//vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, image, &memReqs);
	vulkanDevice->logicalDevice.getImageMemoryRequirements(image, &memReqs);
	
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &allocInfo, nullptr, &imageMemory));
	vulkanDevice->logicalDevice.allocateMemory(&allocInfo, nullptr, &imageMemory);
	//VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, image, imageMemory, 0));
	vulkanDevice->logicalDevice.bindImageMemory(image, imageMemory, 0);

	// Staging
	vkx::Buffer stagingBuffer;

	/*VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		allocInfo.allocationSize));*/

	vulkanDevice->createBuffer(
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		&stagingBuffer,
		allocInfo.allocationSize);

	stagingBuffer.map();
	memcpy(stagingBuffer.mapped, &font24pixels[0][0], STB_FONT_WIDTH * STB_FONT_HEIGHT);	// Only one channel, so data size = W * H (*R8)
	stagingBuffer.unmap();

	// Copy to image
	vk::CommandBuffer copyCmd;
	cmdBufAllocateInfo.commandBufferCount = 1;
	//VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice->logicalDevice, &cmdBufAllocateInfo, &copyCmd));
	vulkanDevice->logicalDevice.allocateCommandBuffers(&cmdBufAllocateInfo, &copyCmd);

	vk::CommandBufferBeginInfo cmdBufInfo;
	//VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
	copyCmd.begin(&cmdBufInfo);

	// Prepare for transfer
	vkx::setImageLayout(
		copyCmd,
		image,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageLayout::ePreinitialized,
		vk::ImageLayout::eTransferDstOptimal);

	vk::BufferImageCopy bufferCopyRegion;
	bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = STB_FONT_WIDTH;
	bufferCopyRegion.imageExtent.height = STB_FONT_HEIGHT;
	bufferCopyRegion.imageExtent.depth = 1;

	/*vkCmdCopyBufferToImage(
		copyCmd,
		stagingBuffer.buffer,
		image,
		vk::ImageLayout::eTransferDstOptimal,
		1,
		&bufferCopyRegion
	);*/
	copyCmd.copyBufferToImage(
		stagingBuffer.buffer,
		image,
		vk::ImageLayout::eTransferDstOptimal,
		1,
		&bufferCopyRegion);

	// Prepare for shader read
	vkx::setImageLayout(
		copyCmd,
		image,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal);

	//VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));
	copyCmd.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;

	//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	queue.submit(1, &submitInfo, VK_NULL_HANDLE);
	//VK_CHECK_RESULT(vkQueueWaitIdle(queue));
	queue.waitIdle();

	stagingBuffer.destroy();

	//vkFreeCommandBuffers(vulkanDevice->logicalDevice, commandPool, 1, &copyCmd);
	vulkanDevice->logicalDevice.freeCommandBuffers(commandPool, 1, &copyCmd);

	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.image = image;
	imageViewInfo.viewType = vk::ImageViewType::e2D;
	imageViewInfo.format = imageInfo.format;
	imageViewInfo.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB,	vk::ComponentSwizzle::eA };
	imageViewInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
	//VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &imageViewInfo, nullptr, &view));
	vulkanDevice->logicalDevice.createImageView(&imageViewInfo, nullptr, &view);

	// Sampler
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.compareOp = vk::CompareOp::eNever;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueWhite;
	//VK_CHECK_RESULT(vkCreateSampler(vulkanDevice->logicalDevice, &samplerInfo, nullptr, &sampler));
	vulkanDevice->logicalDevice.createSampler(&samplerInfo, nullptr, &sampler);

	// Descriptor
	// Font uses a separate descriptor pool
	std::array<vk::DescriptorPoolSize, 1> poolSizes;
	poolSizes[0] = vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1);

	vk::DescriptorPoolCreateInfo descriptorPoolInfo =
		vkx::descriptorPoolCreateInfo(
			static_cast<uint32_t>(poolSizes.size()),
			poolSizes.data(),
			1);

	//VK_CHECK_RESULT(vkCreateDescriptorPool(vulkanDevice->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));
	vulkanDevice->logicalDevice.createDescriptorPool(&descriptorPoolInfo, nullptr, &descriptorPool);

	// Descriptor set layout
	std::array<vk::DescriptorSetLayoutBinding, 1> setLayoutBindings;
	setLayoutBindings[0] = vkx::descriptorSetLayoutBinding(vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 0);

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo =
		vkx::descriptorSetLayoutCreateInfo(
			setLayoutBindings.data(),
			static_cast<uint32_t>(setLayoutBindings.size()));

	//VK_CHECK_RESULT(vkCreateDescriptorSetLayout(vulkanDevice->logicalDevice, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));
	vulkanDevice->logicalDevice.createDescriptorSetLayout(&descriptorSetLayoutInfo, nullptr, &descriptorSetLayout);

	// Pipeline layout
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo =
		vkx::pipelineLayoutCreateInfo(
			&descriptorSetLayout,
			1);

	//VK_CHECK_RESULT(vkCreatePipelineLayout(vulkanDevice->logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));
	vulkanDevice->logicalDevice.createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout);

	// Descriptor set
	vk::DescriptorSetAllocateInfo descriptorSetAllocInfo =
		vkx::descriptorSetAllocateInfo(
			descriptorPool,
			&descriptorSetLayout,
			1);

	//VK_CHECK_RESULT(vkAllocateDescriptorSets(vulkanDevice->logicalDevice, &descriptorSetAllocInfo, &descriptorSet));
	vulkanDevice->logicalDevice.allocateDescriptorSets(&descriptorSetAllocInfo, &descriptorSet);

	vk::DescriptorImageInfo texDescriptor =
		vkx::descriptorImageInfo(
			sampler,
			view,
			vk::ImageLayout::eGeneral);

	std::array<vk::WriteDescriptorSet, 1> writeDescriptorSets;
	writeDescriptorSets[0] = vkx::writeDescriptorSet(descriptorSet, vk::DescriptorType::eCombinedImageSampler, 0, &texDescriptor);
	//vkUpdateDescriptorSets(vulkanDevice->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
	vulkanDevice->logicalDevice.updateDescriptorSets(static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

	// Pipeline cache
	vk::PipelineCacheCreateInfo pipelineCacheCreateInfo;
	//VK_CHECK_RESULT(vkCreatePipelineCache(vulkanDevice->logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
	vulkanDevice->logicalDevice.createPipelineCache(&pipelineCacheCreateInfo, nullptr, &pipelineCache);

	// Command buffer execution fence
	vk::FenceCreateInfo fenceCreateInfo;
	//VK_CHECK_RESULT(vkCreateFence(vulkanDevice->logicalDevice, &fenceCreateInfo, nullptr, &fence));
	vulkanDevice->logicalDevice.createFence(&fenceCreateInfo, nullptr, &fence);
}




/**
* Prepare a separate pipeline for the font rendering decoupled from the main application
*/

void vkx::VulkanTextOverlay::preparePipeline()
{
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState =
		vkx::pipelineInputAssemblyStateCreateInfo(
			vk::PrimitiveTopology::eTriangleStrip,
			vk::PipelineInputAssemblyStateCreateFlags(),
			VK_FALSE);

	vk::PipelineRasterizationStateCreateInfo rasterizationState =
		vkx::pipelineRasterizationStateCreateInfo(
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eBack,
			vk::FrontFace::eClockwise,
			vk::PipelineRasterizationStateCreateFlags());

	// Enable blending
	vk::PipelineColorBlendAttachmentState blendAttachmentState;// = vkx::pipelineColorBlendAttachmentState(0xf, VK_TRUE);
	blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eA;//0xf not an option
	blendAttachmentState.blendEnable = VK_TRUE;
	

	blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eOne;
	blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOne;
	blendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	blendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	blendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOne;
	blendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo colorBlendState =
		vkx::pipelineColorBlendStateCreateInfo(
			1,
			&blendAttachmentState);

	vk::PipelineDepthStencilStateCreateInfo depthStencilState =
		vkx::pipelineDepthStencilStateCreateInfo(
			VK_FALSE,
			VK_FALSE,
			vk::CompareOp::eLessOrEqual);

	vk::PipelineViewportStateCreateInfo viewportState =
		vkx::pipelineViewportStateCreateInfo(1, 1, vk::PipelineViewportStateCreateFlags());

	vk::PipelineMultisampleStateCreateInfo multisampleState =
		vkx::pipelineMultisampleStateCreateInfo(
			vk::SampleCountFlagBits::e1,
			vk::PipelineMultisampleStateCreateFlags());

	std::vector<vk::DynamicState> dynamicStateEnables = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	vk::PipelineDynamicStateCreateInfo dynamicState =
		vkx::pipelineDynamicStateCreateInfo(
			dynamicStateEnables.data(),
			static_cast<uint32_t>(dynamicStateEnables.size()),
			vk::PipelineDynamicStateCreateFlags());

	std::array<vk::VertexInputBindingDescription, 2> vertexBindings = {};
	vertexBindings[0] = vkx::vertexInputBindingDescription(0, sizeof(glm::vec4), vk::VertexInputRate::eVertex);
	vertexBindings[1] = vkx::vertexInputBindingDescription(1, sizeof(glm::vec4), vk::VertexInputRate::eVertex);

	std::array<vk::VertexInputAttributeDescription, 2> vertexAttribs = {};
	// Position
	vertexAttribs[0] = vkx::vertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, 0);
	// UV
	vertexAttribs[1] = vkx::vertexInputAttributeDescription(1, 1, vk::Format::eR32G32Sfloat, sizeof(glm::vec2));

	vk::PipelineVertexInputStateCreateInfo inputState = vkx::pipelineVertexInputStateCreateInfo();
	inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
	inputState.pVertexBindingDescriptions = vertexBindings.data();
	inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttribs.size());
	inputState.pVertexAttributeDescriptions = vertexAttribs.data();

	vk::GraphicsPipelineCreateInfo pipelineCreateInfo =
		vkx::pipelineCreateInfo(
			pipelineLayout,
			renderPass,
			vk::PipelineCreateFlags());

	pipelineCreateInfo.pVertexInputState = &inputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();

	//VK_CHECK_RESULT(vkCreateGraphicsPipelines(vulkanDevice->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	vulkanDevice->logicalDevice.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);//?????
}









/**
* Prepare a separate render pass for rendering the text as an overlay
*/

void vkx::VulkanTextOverlay::prepareRenderPass()
{
	vk::AttachmentDescription attachments[2];

	// Color attachment
	attachments[0].format = colorFormat;
	attachments[0].samples = vk::SampleCountFlagBits::e1;
	// Don't clear the framebuffer (like the renderpass from the example does)
	attachments[0].loadOp = vk::AttachmentLoadOp::eLoad;
	attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
	attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[0].initialLayout = vk::ImageLayout::eUndefined;
	attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

	// Depth attachment
	attachments[1].format = depthFormat;
	attachments[1].samples = vk::SampleCountFlagBits::e1;
	attachments[1].loadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[1].storeOp = vk::AttachmentStoreOp::eDontCare;
	attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[1].initialLayout = vk::ImageLayout::eUndefined;
	attachments[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference colorReference;
	colorReference.attachment = 0;
	colorReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference depthReference;
	depthReference.attachment = 1;
	depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;;

	vk::SubpassDependency subpassDependencies[2];

	// Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commmands executed outside of the actual renderpass)
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	subpassDependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	subpassDependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
	subpassDependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	subpassDependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	// Transition from initial to final
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	subpassDependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	subpassDependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	subpassDependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
	subpassDependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	vk::SubpassDescription subpassDescription;
	subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDescription.flags = vk::SubpassDescriptionFlags();
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = NULL;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pResolveAttachments = NULL;
	subpassDescription.pDepthStencilAttachment = &depthReference;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = NULL;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.pNext = NULL;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = subpassDependencies;

	//VK_CHECK_RESULT(vkCreateRenderPass(vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &renderPass));
	vulkanDevice->logicalDevice.createRenderPass(&renderPassInfo, nullptr, &renderPass);
}




/**
* Maps the buffer, resets letter count
*/

void vkx::VulkanTextOverlay::beginTextUpdate()
{
	mappedLocal = (glm::vec4*)vertexBuffer.mapped;
	numLetters = 0;
}






/**
* Add text to the current buffer
*
* @param text Text to add
* @param x x position of the text to add in window coordinate space
* @param y y position of the text to add in window coordinate space
* @param align Alignment for the new text (left, right, center)
*/

void vkx::VulkanTextOverlay::addText(std::string text, float x, float y, TextAlign align)
{
	assert(vertexBuffer.mapped != nullptr);

	const float charW = 1.5f / *frameBufferWidth;
	const float charH = 1.5f / *frameBufferHeight;

	float fbW = (float)*frameBufferWidth;
	float fbH = (float)*frameBufferHeight;
	x = (x / fbW * 2.0f) - 1.0f;
	y = (y / fbH * 2.0f) - 1.0f;

	// Calculate text width
	float textWidth = 0;
	for (auto letter : text)
	{
		stb_fontchar *charData = &stbFontData[(uint32_t)letter - STB_FIRST_CHAR];
		textWidth += charData->advance * charW;
	}

	switch (align)
	{
	case alignRight:
		x -= textWidth;
		break;
	case alignCenter:
		x -= textWidth / 2.0f;
		break;
	case alignLeft:
		break;
	}

	// Generate a uv mapped quad per char in the new text
	for (auto letter : text)
	{
		stb_fontchar *charData = &stbFontData[(uint32_t)letter - STB_FIRST_CHAR];

		mappedLocal->x = (x + (float)charData->x0 * charW);
		mappedLocal->y = (y + (float)charData->y0 * charH);
		mappedLocal->z = charData->s0;
		mappedLocal->w = charData->t0;
		mappedLocal++;

		mappedLocal->x = (x + (float)charData->x1 * charW);
		mappedLocal->y = (y + (float)charData->y0 * charH);
		mappedLocal->z = charData->s1;
		mappedLocal->w = charData->t0;
		mappedLocal++;

		mappedLocal->x = (x + (float)charData->x0 * charW);
		mappedLocal->y = (y + (float)charData->y1 * charH);
		mappedLocal->z = charData->s0;
		mappedLocal->w = charData->t1;
		mappedLocal++;

		mappedLocal->x = (x + (float)charData->x1 * charW);
		mappedLocal->y = (y + (float)charData->y1 * charH);
		mappedLocal->z = charData->s1;
		mappedLocal->w = charData->t1;
		mappedLocal++;

		x += charData->advance * charW;

		numLetters++;
	}
}





/**
* Unmap buffer and update command buffers
*/

void vkx::VulkanTextOverlay::endTextUpdate()
{
	updateCommandBuffers();
}




/**
* Update the command buffers to reflect text changes
*/

void vkx::VulkanTextOverlay::updateCommandBuffers()
{
	vk::CommandBufferBeginInfo cmdBufInfo;

	vk::ClearValue clearValues[1];
	//clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[0].color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f};

	vk::RenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.extent.width = *frameBufferWidth;
	renderPassBeginInfo.renderArea.extent.height = *frameBufferHeight;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;

	for (int32_t i = 0; i < cmdBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = *frameBuffers[i];

		//VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffers[i], &cmdBufInfo));
		cmdBuffers[i].begin(&cmdBufInfo);

		if (vkDebug::DebugMarker::active)
		{
			vkDebug::DebugMarker::beginRegion(cmdBuffers[i], "Text overlay", glm::vec4(1.0f, 0.94f, 0.3f, 1.0f));
		}

		//vkCmdBeginRenderPass(cmdBuffers[i], &renderPassBeginInfo, vk::SubpassContents::eInline);
		cmdBuffers[i].beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);

		vk::Viewport viewport = vkx::viewport((float)*frameBufferWidth, (float)*frameBufferHeight, 0.0f, 1.0f);
		//vkCmdSetViewport(cmdBuffers[i], 0, 1, &viewport);
		cmdBuffers[i].setViewport(0, 1, &viewport);

		vk::Rect2D scissor = vkx::rect2D(*frameBufferWidth, *frameBufferHeight, 0, 0);
		//vkCmdSetScissor(cmdBuffers[i], 0, 1, &scissor);
		cmdBuffers[i].setScissor(0, 1, &scissor);

		//vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		cmdBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		//vkCmdBindDescriptorSets(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
		cmdBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		vk::DeviceSize offsets = 0;
		//vkCmdBindVertexBuffers(cmdBuffers[i], 0, 1, &vertexBuffer.buffer, &offsets);
		cmdBuffers[i].bindVertexBuffers(0, 1, &vertexBuffer.buffer, &offsets);
		//vkCmdBindVertexBuffers(cmdBuffers[i], 1, 1, &vertexBuffer.buffer, &offsets);
		cmdBuffers[i].bindVertexBuffers(1, 1, &vertexBuffer.buffer, &offsets);
		for (uint32_t j = 0; j < numLetters; j++)
		{
			//vkCmdDraw(cmdBuffers[i], 4, 1, j * 4, 0);
			cmdBuffers[i].draw(4, 1, j * 4, 0);
		}

		//vkCmdEndRenderPass(cmdBuffers[i]);
		cmdBuffers[i].endRenderPass();

		if (vkDebug::DebugMarker::active)
		{
			vkDebug::DebugMarker::endRegion(cmdBuffers[i]);
		}

		//VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffers[i]));
		cmdBuffers[i].end();
	}
}

/**
* Submit the text command buffers to a queue
*/

void vkx::VulkanTextOverlay::submit(vk::Queue queue, uint32_t bufferindex, vk::SubmitInfo submitInfo)
{
	if (!visible) {
		return;
	}

	submitInfo.pCommandBuffers = &cmdBuffers[bufferindex];
	submitInfo.commandBufferCount = 1;

	//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
	queue.submit(1, &submitInfo, fence);

	//VK_CHECK_RESULT(vkWaitForFences(vulkanDevice->logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX));
	vk::Device(vulkanDevice->logicalDevice).waitForFences(1, &fence, VK_TRUE, UINT64_MAX);
	//VK_CHECK_RESULT(vkResetFences(vulkanDevice->logicalDevice, 1, &fence));
	vk::Device(vulkanDevice->logicalDevice).resetFences(1, &fence);
}

/**
* Reallocate command buffers for the text overlay
* @note Frees the existing command buffers
*/

void vkx::VulkanTextOverlay::reallocateCommandBuffers()
{
	//vkFreeCommandBuffers(vulkanDevice->logicalDevice, commandPool, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
	vk::Device(vulkanDevice->logicalDevice).freeCommandBuffers(commandPool, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());

	vk::CommandBufferAllocateInfo cmdBufAllocateInfo =
		vkx::commandBufferAllocateInfo(
			commandPool,
			vk::CommandBufferLevel::ePrimary,
			static_cast<uint32_t>(cmdBuffers.size()));

	//VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice->logicalDevice, &cmdBufAllocateInfo, cmdBuffers.data()));
	vk::Device(vulkanDevice->logicalDevice).allocateCommandBuffers(&cmdBufAllocateInfo, cmdBuffers.data());
}
