#pragma once

//float rand0t1() {
//	std::random_device rd;
//	std::mt19937 gen(rd());
//	std::uniform_real_distribution<> dis(0.0f, 1.0f);
//	float rnd = dis(gen);
//	return rnd;
//}

#include <vulkan/vulkan.hpp>
#include "vulkanTools.h"
#include "vulkanContext.h"
#include "imgui.h"
//#include "vulkanApp.h"


//// Options and values to display/toggle from the UI
//struct UISettings {
//	bool displayModels = true;
//	bool displayLogos = true;
//	bool displayBackground = true;
//	bool animateLight = false;
//	float lightSpeed = 0.25f;
//	std::array<float, 50> frameTimes = { 0 };
//
//	//std::array<float, 50> FPSValues = { 0 };
//
//	//float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
//
//	float frameTimeMin = 0.0f;
//	float frameTimeMax = 300.0f;
//
//	float lightTimer = 0.0f;
//} uiSettings;

// ----------------------------------------------------------------------------
// ImGUI class
// ----------------------------------------------------------------------------
class ImGUI {
	private:
	// Vulkan resources for rendering the UI
	vk::Sampler sampler;
	vkx::CreateBufferResult vertexBuffer;
	vkx::CreateBufferResult indexBuffer;

	int32_t vertexCount = 0;
	int32_t indexCount = 0;
	vk::DeviceMemory fontMemory;// = VK_NULL_HANDLE;
	vk::Image fontImage;// = VK_NULL_HANDLE;
	vk::ImageView fontView;// = VK_NULL_HANDLE;
	vk::PipelineCache pipelineCache;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline pipeline;
	vk::DescriptorPool descriptorPool;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorSet descriptorSet;
	//vks::VulkanDevice *device;
	vkx::Context *context;
	public:
	// UI params are set via push constants
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

	ImGUI(vkx::Context *context) : context(context) {

	};

	~ImGUI() {
		// Release all Vulkan resources required for rendering imGui
		vertexBuffer.destroy();
		indexBuffer.destroy();
		context->device.destroyImage(fontImage, nullptr);
		context->device.destroyImageView(fontView, nullptr);
		context->device.freeMemory(fontMemory, nullptr);
		context->device.destroySampler(sampler, nullptr);
		context->device.destroyPipelineCache(pipelineCache, nullptr);
		context->device.destroyPipeline(pipeline, nullptr);
		context->device.destroyPipelineLayout(pipelineLayout, nullptr);
		context->device.destroyDescriptorPool(descriptorPool, nullptr);
		context->device.destroyDescriptorSetLayout(descriptorSetLayout, nullptr);
	}

	// Initialize styles, keys, etc.
	void init(float width, float height) {
		// Color scheme
		ImGuiStyle &style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		// Dimensions
		ImGuiIO &io = ImGui::GetIO();
		io.DisplaySize = ImVec2(width, height);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	}

	// Initialize all Vulkan resources used by the ui
	void initResources(vk::RenderPass renderPass, vk::Queue copyQueue) {
		ImGuiIO &io = ImGui::GetIO();

		// Create font texture
		unsigned char* fontData;
		int texWidth, texHeight;
		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		vk::DeviceSize uploadSize = texWidth*texHeight * 4 * sizeof(char);

		// Create target image for copy
		vk::ImageCreateInfo imageInfo;// = vks::initializers::imageCreateInfo();
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.format = vk::Format::eR8G8B8A8Unorm;
		imageInfo.extent.width = texWidth;
		imageInfo.extent.height = texHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = vk::SampleCountFlagBits::e1;
		imageInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
		imageInfo.sharingMode = vk::SharingMode::eExclusive;
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;
		fontImage = context->device.createImage(imageInfo, nullptr);
		vk::MemoryRequirements memReqs;
		memReqs = context->device.getImageMemoryRequirements(fontImage);

		vk::MemoryAllocateInfo memAllocInfo;// = vks::initializers::memoryAllocateInfo();
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = context->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

		fontMemory = context->device.allocateMemory(memAllocInfo, nullptr);
		context->device.bindImageMemory(fontImage, fontMemory, 0);

		// Image view
		vk::ImageViewCreateInfo viewInfo;// = vks::initializers::imageViewCreateInfo();
		viewInfo.image = fontImage;
		viewInfo.viewType = vk::ImageViewType::e2D;
		viewInfo.format = vk::Format::eR8G8B8A8Unorm;
		viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		//VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &fontView));
		fontView = context->device.createImageView(viewInfo, nullptr);

		// Staging buffers for font data upload
		vkx::CreateBufferResult stagingBuffer;



		//VK_CHECK_RESULT(device->createBuffer(
		//	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//	&stagingBuffer,
		//	uploadSize));


		stagingBuffer = context->createBuffer(
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			uploadSize);

		//context->createBuffer(
		//	vk::BufferUsageFlagBits::eTransferSrc,
		//	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		//	&stagingBuffer,
		//	uploadSize);

		stagingBuffer.map();
		memcpy(stagingBuffer.mapped, fontData, uploadSize);
		stagingBuffer.unmap();



		// Copy buffer data to font image
		vk::CommandBuffer copyCmd = context->createCommandBuffer(vk::CommandBufferLevel::ePrimary, true);

		// Prepare for transfer
		vkx::setImageLayout(
			copyCmd,
			fontImage,
			vk::ImageAspectFlagBits::eColor,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			vk::PipelineStageFlagBits::eHost,
			vk::PipelineStageFlagBits::eTransfer);

		// Copy
		vk::BufferImageCopy bufferCopyRegion;
		bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = texWidth;
		bufferCopyRegion.imageExtent.height = texHeight;
		bufferCopyRegion.imageExtent.depth = 1;

		copyCmd.copyBufferToImage(
			stagingBuffer.buffer,
			fontImage,
			vk::ImageLayout::eTransferDstOptimal,
			1,
			&bufferCopyRegion);



		// Prepare for shader read
		vkx::setImageLayout(
			copyCmd,
			fontImage,
			vk::ImageAspectFlagBits::eColor,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader);

		context->flushCommandBuffer(copyCmd, copyQueue, true);

		stagingBuffer.destroy();

		// Font texture Sampler
		vk::SamplerCreateInfo samplerInfo;// = vks::initializers::samplerCreateInfo();
		samplerInfo.magFilter = vk::Filter::eLinear;
		samplerInfo.minFilter = vk::Filter::eLinear;
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
		sampler = context->device.createSampler(samplerInfo, nullptr);

		// Descriptor pool
		std::vector<vk::DescriptorPoolSize> poolSizes = {
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
		};
		vk::DescriptorPoolCreateInfo descriptorPoolInfo = vkx::descriptorPoolCreateInfo(poolSizes, 2);
		descriptorPool = context->device.createDescriptorPool(descriptorPoolInfo, nullptr);

		// Descriptor set layout
		std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings = {
			vkx::descriptorSetLayoutBinding(vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 0),
		};
		vk::DescriptorSetLayoutCreateInfo descriptorLayout = vkx::descriptorSetLayoutCreateInfo(setLayoutBindings);
		descriptorSetLayout = context->device.createDescriptorSetLayout(descriptorLayout, nullptr);

		// Descriptor set
		vk::DescriptorSetAllocateInfo allocInfo = vkx::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		context->device.allocateDescriptorSets(&allocInfo, &descriptorSet);

		vk::DescriptorImageInfo fontDescriptor = vkx::descriptorImageInfo(
			sampler,
			fontView,
			vk::ImageLayout::eShaderReadOnlyOptimal
		);

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
			vkx::writeDescriptorSet(descriptorSet, vk::DescriptorType::eCombinedImageSampler, 0, &fontDescriptor)
		};
		context->device.updateDescriptorSets(static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		// Pipeline cache
		vk::PipelineCacheCreateInfo pipelineCacheCreateInfo;
		pipelineCache = context->device.createPipelineCache(pipelineCacheCreateInfo, nullptr);

		// Pipeline layout
		// Push constants for UI rendering parameters

		vk::PushConstantRange pushConstantRange = vkx::pushConstantRange(vk::ShaderStageFlagBits::eVertex, sizeof(PushConstBlock), 0);
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vkx::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		pipelineLayout = context->device.createPipelineLayout(pipelineLayoutCreateInfo, nullptr);

		// Setup graphics pipeline for UI rendering
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkx::pipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology::eTriangleList, {}, VK_FALSE);

		vk::PipelineRasterizationStateCreateInfo rasterizationState =
			vkx::pipelineRasterizationStateCreateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise);

		// Enable blending
		vk::PipelineColorBlendAttachmentState blendAttachmentState;
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		blendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
		blendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		blendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		blendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;

		vk::PipelineColorBlendStateCreateInfo colorBlendState =
			vkx::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

		vk::PipelineDepthStencilStateCreateInfo depthStencilState =
			vkx::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, vk::CompareOp::eLessOrEqual);

		vk::PipelineViewportStateCreateInfo viewportState =
			vkx::pipelineViewportStateCreateInfo(1, 1, {});

		vk::PipelineMultisampleStateCreateInfo multisampleState =
			vkx::pipelineMultisampleStateCreateInfo(vk::SampleCountFlagBits::e1);

		std::vector<vk::DynamicState> dynamicStateEnables = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};

		vk::PipelineDynamicStateCreateInfo dynamicState =
			vkx::pipelineDynamicStateCreateInfo(dynamicStateEnables);

		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo = vkx::pipelineCreateInfo(pipelineLayout, renderPass);

		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		// Vertex bindings an attributes based on ImGui vertex definition
		std::vector<vk::VertexInputBindingDescription> vertexInputBindings = {
			vkx::vertexInputBindingDescription(0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex),
		};
		std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes = {
			vkx::vertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)),	// Location 0: Position
			vkx::vertexInputAttributeDescription(0, 1, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)),	// Location 1: UV
			vkx::vertexInputAttributeDescription(0, 2, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)),	// Location 2: Color
		};

		vk::PipelineVertexInputStateCreateInfo vertexInputState;// = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		pipelineCreateInfo.pVertexInputState = &vertexInputState;


		shaderStages[0] = context->loadShader(vkx::getAssetPath() + "shaders/vulkanscene/imgui/ui.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context->loadShader(vkx::getAssetPath() + "shaders/vulkanscene/imgui/ui.frag.spv", vk::ShaderStageFlagBits::eFragment);

		pipeline = context->device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo);
	}

	//// Starts a new imGui frame and sets up windows and ui elements
	//void newFrame(vkx::vulkanApp *example, bool updateFrameGraph) {
	//	ImGui::NewFrame();

	//	// Init imGui windows and elements

	//	ImVec4 clear_color = ImColor(114, 144, 154);
	//	static float f = 0.0f;
	//	ImGui::Text(example->title.c_str());
	//	ImGui::Text(context->deviceProperties.deviceName);


	//	// Update frame time display
	//	if (updateFrameGraph) {
	//		std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
	//		float frameTime = 1000.0f / (example->deltaTime * 1000.0f);
	//		//frameTime = rand0t1() * 100;
	//		uiSettings.frameTimes.back() = frameTime;

	//		if (frameTime < uiSettings.frameTimeMin) {
	//			uiSettings.frameTimeMin = frameTime;
	//		}
	//		if (frameTime > uiSettings.frameTimeMax && frameTime < 9000) {
	//			uiSettings.frameTimeMax = frameTime;
	//		}
	//	}

	//	ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 80));
	//	//ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 200));

	//	ImGui::Text("Camera");
	//	ImGui::InputFloat3("position", &example->camera.transform.translation.x, 2);
	//	//ImGui::InputFloat3("rotation", &example->camera.transform.orientation.x, 3);

	//	ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiSetCond_FirstUseEver);
	//	ImGui::Begin("Example settings");
	//	ImGui::Checkbox("Render models", &uiSettings.displayModels);
	//	ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
	//	ImGui::Checkbox("Display background", &uiSettings.displayBackground);
	//	ImGui::Checkbox("Animate light", &uiSettings.animateLight);
	//	ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
	//	ImGui::End();

	//	ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
	//	ImGui::ShowTestWindow();

	//	// Render to generate draw buffers
	//	ImGui::Render();
	//}

	// Update vertex and index buffer containing the imGui elements when required
	void updateBuffers() {
		ImDrawData* imDrawData = ImGui::GetDrawData();

		// Note: Alignment is done inside buffer creation
		vk::DeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
		vk::DeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

		// Update buffers only if vertex or index count has been changed compared to current buffer size

		// Vertex buffer		
		if (((VkBuffer)vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
			vertexBuffer.unmap();
			vertexBuffer.destroy();
			//VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vertexBuffer, vertexBufferSize));// last 2 parameters are reversed
			context->createBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible, &vertexBuffer, vertexBufferSize);
			//vertexBuffer = context->createBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible, vertexBufferSize);

			vertexCount = imDrawData->TotalVtxCount;
			vertexBuffer.unmap();
			vertexBuffer.map();
		}

		// Index buffer
		vk::DeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
		if (((VkBuffer)indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {
			indexBuffer.unmap();
			indexBuffer.destroy();
			//VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &indexBuffer, indexBufferSize));
			context->createBuffer(vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eHostVisible, &indexBuffer, indexBufferSize);
			//indexBuffer = context->createBuffer(vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eHostVisible, indexBufferSize);

			indexCount = imDrawData->TotalIdxCount;
			indexBuffer.map();
		}

		// Upload data
		ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer.mapped;
		ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer.mapped;

		for (int n = 0; n < imDrawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		// Flush to make writes visible to GPU
		vertexBuffer.flush();
		indexBuffer.flush();
	}

	// Draw current imGui frame into a command buffer
	void drawFrame(vk::CommandBuffer commandBuffer) {
		ImGuiIO& io = ImGui::GetIO();

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		// Bind vertex and index buffer
		vk::DeviceSize offsets[1] = { 0 };
		commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer.buffer, offsets);
		//vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, vk::IndexType::eUint16);
		commandBuffer.bindIndexBuffer(indexBuffer.buffer, 0, vk::IndexType::eUint16);

		vk::Viewport viewport = vkx::viewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);
		commandBuffer.setViewport(0, 1, &viewport);

		// UI scale and translate via push constants
		pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		pushConstBlock.translate = glm::vec2(-1.0f);
		commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstBlock), &pushConstBlock);

		// Render commands
		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;
		for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
				vk::Rect2D scissorRect;
				scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
				scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
				scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
				commandBuffer.setScissor(0, 1, &scissorRect);
				commandBuffer.drawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}

};