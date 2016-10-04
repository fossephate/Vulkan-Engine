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
	/*vkDestroySampler(vulkanDevice->logicalDevice, sampler, nullptr);
	vkDestroyImage(vulkanDevice->logicalDevice, image, nullptr);
	vkDestroyImageView(vulkanDevice->logicalDevice, view, nullptr);
	vkFreeMemory(vulkanDevice->logicalDevice, imageMemory, nullptr);
	vkDestroyDescriptorSetLayout(vulkanDevice->logicalDevice, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(vulkanDevice->logicalDevice, descriptorPool, nullptr);
	vkDestroyPipelineLayout(vulkanDevice->logicalDevice, pipelineLayout, nullptr);
	vkDestroyPipelineCache(vulkanDevice->logicalDevice, pipelineCache, nullptr);
	vkDestroyPipeline(vulkanDevice->logicalDevice, pipeline, nullptr);
	vkDestroyRenderPass(vulkanDevice->logicalDevice, renderPass, nullptr);
	vkFreeCommandBuffers(vulkanDevice->logicalDevice, commandPool, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
	vkDestroyCommandPool(vulkanDevice->logicalDevice, commandPool, nullptr);
	vkDestroyFence(vulkanDevice->logicalDevice, fence, nullptr);*/
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
	//cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
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
	vk::Device(vulkanDevice->logicalDevice).getImageMemoryRequirements(image, &memReqs);
	
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &allocInfo, nullptr, &imageMemory));
	vk::Device(vulkanDevice->logicalDevice).allocateMemory(&allocInfo, nullptr, &imageMemory);
	//VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, image, imageMemory, 0));
	vk::Device(vulkanDevice->logicalDevice).bindImageMemory(image, imageMemory, 0);

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
	vk::Device(vulkanDevice->logicalDevice).allocateCommandBuffers(&cmdBufAllocateInfo, &copyCmd);

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
	vk::Device(vulkanDevice->logicalDevice).freeCommandBuffers(commandPool, 1, &copyCmd);

	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.image = image;
	imageViewInfo.viewType = vk::ImageViewType::e2D;
	imageViewInfo.format = imageInfo.format;
	imageViewInfo.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB,	vk::ComponentSwizzle::eA };
	imageViewInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
	//VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &imageViewInfo, nullptr, &view));
	vk::Device(vulkanDevice->logicalDevice).createImageView(&imageViewInfo, nullptr, &view);

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
	vk::Device (vulkanDevice->logicalDevice).createSampler(&samplerInfo, nullptr, &sampler);

	// Descriptor
	// Font uses a separate descriptor pool
	std::array<VkDescriptorPoolSize, 1> poolSizes;
	poolSizes[0] = vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1);

	vk::DescriptorPoolCreateInfo descriptorPoolInfo =
		vkx::descriptorPoolCreateInfo(
			static_cast<uint32_t>(poolSizes.size()),
			poolSizes.data(),
			1);

	//VK_CHECK_RESULT(vkCreateDescriptorPool(vulkanDevice->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

	// Descriptor set layout
	std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings;
	setLayoutBindings[0] = vkx::descriptorSetLayoutBinding(vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 0);

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo =
		vkx::descriptorSetLayoutCreateInfo(
			setLayoutBindings.data(),
			static_cast<uint32_t>(setLayoutBindings.size()));

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(vulkanDevice->logicalDevice, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo =
		vkTools::initializers::pipelineLayoutCreateInfo(
			&descriptorSetLayout,
			1);

	VK_CHECK_RESULT(vkCreatePipelineLayout(vulkanDevice->logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

	// Descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo =
		vkTools::initializers::descriptorSetAllocateInfo(
			descriptorPool,
			&descriptorSetLayout,
			1);

	VK_CHECK_RESULT(vkAllocateDescriptorSets(vulkanDevice->logicalDevice, &descriptorSetAllocInfo, &descriptorSet));

	VkDescriptorImageInfo texDescriptor =
		vkTools::initializers::descriptorImageInfo(
			sampler,
			view,
			VK_IMAGE_LAYOUT_GENERAL);

	std::array<VkWriteDescriptorSet, 1> writeDescriptorSets;
	writeDescriptorSets[0] = vkTools::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texDescriptor);
	vkUpdateDescriptorSets(vulkanDevice->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

	// Pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK_RESULT(vkCreatePipelineCache(vulkanDevice->logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache));

	// Command buffer execution fence
	VkFenceCreateInfo fenceCreateInfo = vkTools::initializers::fenceCreateInfo();
	VK_CHECK_RESULT(vkCreateFence(vulkanDevice->logicalDevice, &fenceCreateInfo, nullptr, &fence));
}




/**
* Prepare a separate pipeline for the font rendering decoupled from the main application
*/

void vkx::VulkanTextOverlay::preparePipeline()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
		vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
			0,
			VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterizationState =
		vkTools::initializers::pipelineRasterizationStateCreateInfo(
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_BACK_BIT,
			VK_FRONT_FACE_CLOCKWISE,
			0);

	// Enable blending
	VkPipelineColorBlendAttachmentState blendAttachmentState =
		vkTools::initializers::pipelineColorBlendAttachmentState(0xf, VK_TRUE);

	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendState =
		vkTools::initializers::pipelineColorBlendStateCreateInfo(
			1,
			&blendAttachmentState);

	VkPipelineDepthStencilStateCreateInfo depthStencilState =
		vkTools::initializers::pipelineDepthStencilStateCreateInfo(
			VK_FALSE,
			VK_FALSE,
			VK_COMPARE_OP_LESS_OR_EQUAL);

	VkPipelineViewportStateCreateInfo viewportState =
		vkTools::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisampleState =
		vkTools::initializers::pipelineMultisampleStateCreateInfo(
			VK_SAMPLE_COUNT_1_BIT,
			0);

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState =
		vkTools::initializers::pipelineDynamicStateCreateInfo(
			dynamicStateEnables.data(),
			static_cast<uint32_t>(dynamicStateEnables.size()),
			0);

	std::array<VkVertexInputBindingDescription, 2> vertexBindings = {};
	vertexBindings[0] = vkTools::initializers::vertexInputBindingDescription(0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX);
	vertexBindings[1] = vkTools::initializers::vertexInputBindingDescription(1, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX);

	std::array<VkVertexInputAttributeDescription, 2> vertexAttribs = {};
	// Position
	vertexAttribs[0] = vkTools::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
	// UV
	vertexAttribs[1] = vkTools::initializers::vertexInputAttributeDescription(1, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2));

	VkPipelineVertexInputStateCreateInfo inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
	inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
	inputState.pVertexBindingDescriptions = vertexBindings.data();
	inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttribs.size());
	inputState.pVertexAttributeDescriptions = vertexAttribs.data();

	VkGraphicsPipelineCreateInfo pipelineCreateInfo =
		vkTools::initializers::pipelineCreateInfo(
			pipelineLayout,
			renderPass,
			0);

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

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(vulkanDevice->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}









/**
* Prepare a separate render pass for rendering the text as an overlay
*/

inline void vkx::VulkanTextOverlay::prepareRenderPass()
{
	VkAttachmentDescription attachments[2] = {};

	// Color attachment
	attachments[0].format = colorFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	// Don't clear the framebuffer (like the renderpass from the example does)
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Depth attachment
	attachments[1].format = depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDependency subpassDependencies[2] = {};

	// Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commmands executed outside of the actual renderpass)
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Transition from initial to final
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.flags = 0;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = NULL;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pResolveAttachments = NULL;
	subpassDescription.pDepthStencilAttachment = &depthReference;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = subpassDependencies;

	VK_CHECK_RESULT(vkCreateRenderPass(vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &renderPass));
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
	VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[1];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

	VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.extent.width = *frameBufferWidth;
	renderPassBeginInfo.renderArea.extent.height = *frameBufferHeight;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;

	for (int32_t i = 0; i < cmdBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = *frameBuffers[i];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffers[i], &cmdBufInfo));

		if (vkDebug::DebugMarker::active)
		{
			vkDebug::DebugMarker::beginRegion(cmdBuffers[i], "Text overlay", glm::vec4(1.0f, 0.94f, 0.3f, 1.0f));
		}

		vkCmdBeginRenderPass(cmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkTools::initializers::viewport((float)*frameBufferWidth, (float)*frameBufferHeight, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkTools::initializers::rect2D(*frameBufferWidth, *frameBufferHeight, 0, 0);
		vkCmdSetScissor(cmdBuffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		VkDeviceSize offsets = 0;
		vkCmdBindVertexBuffers(cmdBuffers[i], 0, 1, &vertexBuffer.buffer, &offsets);
		vkCmdBindVertexBuffers(cmdBuffers[i], 1, 1, &vertexBuffer.buffer, &offsets);
		for (uint32_t j = 0; j < numLetters; j++)
		{
			vkCmdDraw(cmdBuffers[i], 4, 1, j * 4, 0);
		}

		vkCmdEndRenderPass(cmdBuffers[i]);

		if (vkDebug::DebugMarker::active)
		{
			vkDebug::DebugMarker::endRegion(cmdBuffers[i]);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffers[i]));
	}
}

/**
* Submit the text command buffers to a queue
*/

void vkx::VulkanTextOverlay::submit(VkQueue queue, uint32_t bufferindex, VkSubmitInfo submitInfo)
{
	if (!visible)
	{
		return;
	}

	submitInfo.pCommandBuffers = &cmdBuffers[bufferindex];
	submitInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));

	VK_CHECK_RESULT(vkWaitForFences(vulkanDevice->logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX));
	VK_CHECK_RESULT(vkResetFences(vulkanDevice->logicalDevice, 1, &fence));
}

/**
* Reallocate command buffers for the text overlay
* @note Frees the existing command buffers
*/

void vkx::VulkanTextOverlay::reallocateCommandBuffers()
{
	vkFreeCommandBuffers(vulkanDevice->logicalDevice, commandPool, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vkTools::initializers::commandBufferAllocateInfo(
			commandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			static_cast<uint32_t>(cmdBuffers.size()));

	VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice->logicalDevice, &cmdBufAllocateInfo, cmdBuffers.data()));
}
