/*
* Vulkan Example - imGui (https://github.com/ocornut/imgui)
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <array>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gli/gli.hpp>

#include <imgui.h>
//#include "vk/imgui_impl_glfw_vulkan.h"

//#include "testBuffer.hpp"

#include "vulkanApp.h"
#include "vulkanOffscreenExampleBase.hpp"

#define ENABLE_VALIDATION false






// Options and values to display/toggle from the UI
struct UISettings {
	bool displayModels = true;
	bool displayLogos = true;
	bool displayBackground = true;
	bool animateLight = false;
	float lightSpeed = 0.25f;
	std::array<float, 50> frameTimes = { 0 };
	float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
	float lightTimer = 0.0f;
} uiSettings;

// ----------------------------------------------------------------------------
// ImGUI class
// ----------------------------------------------------------------------------
class ImGUI {
	private:
	// Vulkan resources for rendering the UI
	vk::Sampler sampler;
	//vks::Buffer vertexBuffer;
	//vks::Buffer indexBuffer;
	//vkx::CreateBufferResult vertexBuffer;
	//vkx::CreateBufferResult indexBuffer;
	vkx::TestBuffer vertexBuffer;
	vkx::TestBuffer indexBuffer;

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
	vkx::vulkanApp *example;
	public:
	// UI params are set via push constants
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

	ImGUI(/*vkx::vulkanApp *example*/vkx::Context *context) : /*example(example)*/context(context) {
		//device = example->vulkanDevice;
		//context = example->context
	};

	~ImGUI() {
		// Release all Vulkan resources required for rendering imGui
		vertexBuffer.destroy();
		indexBuffer.destroy();
		//vkDestroyImage(device->logicalDevice, fontImage, nullptr);
		context->device.destroyImage(fontImage, nullptr);
		//vkDestroyImageView(device->logicalDevice, fontView, nullptr);
		context->device.destroyImageView(fontView, nullptr);
		//vkFreeMemory(device->logicalDevice, fontMemory, nullptr);
		context->device.freeMemory(fontMemory, nullptr);
		//vkDestroySampler(device->logicalDevice, sampler, nullptr);
		context->device.destroySampler(sampler, nullptr);
		//vkDestroyPipelineCache(device->logicalDevice, pipelineCache, nullptr);
		context->device.destroyPipelineCache(pipelineCache, nullptr);
		//vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
		context->device.destroyPipeline(pipeline, nullptr);
		//vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
		context->device.destroyPipelineLayout(pipelineLayout, nullptr);
		//vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
		context->device.destroyDescriptorPool(descriptorPool, nullptr);
		//vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout, nullptr);
		context->device.destroyDescriptorSetLayout(descriptorSetLayout, nullptr);
	}

	// Initialize styles, keys, etc.
	void init(float width, float height) {
		// Color scheme
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		// Dimensions
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(width, height);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	}

	// Initialize all Vulkan resources used by the ui
	void initResources(vk::RenderPass renderPass, vk::Queue copyQueue) {
		ImGuiIO& io = ImGui::GetIO();

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
		//VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageInfo, nullptr, &fontImage));
		fontImage = context->device.createImage(imageInfo, nullptr);
		vk::MemoryRequirements memReqs;
		//vkGetImageMemoryRequirements(device->logicalDevice, fontImage, &memReqs);
		memReqs = context->device.getImageMemoryRequirements(fontImage);

		vk::MemoryAllocateInfo memAllocInfo;// = vks::initializers::memoryAllocateInfo();
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = context->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

		

		//VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &fontMemory));
		fontMemory = context->device.allocateMemory(memAllocInfo, nullptr);
		//VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, fontImage, fontMemory, 0));
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
		//vks::Buffer stagingBuffer;
		//vkx::CreateBufferResult stagingBuffer;
		vkx::TestBuffer stagingBuffer;



		//VK_CHECK_RESULT(device->createBuffer(
		//	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//	&stagingBuffer,
		//	uploadSize));



		//context->createBuffer(
		//	vk::BufferUsageFlagBits::eTransferSrc,
		//	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		//	uploadSize,
		//	&stagingBuffer);
		//stagingBuffer.map();
		//memcpy(stagingBuffer.mapped, fontData, uploadSize);
		//stagingBuffer.unmap();

		//stagingBuffer = context->createBuffer(
		//	vk::BufferUsageFlagBits::eTransferSrc,
		//	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		//	uploadSize);

		context->createBuffer(
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			&stagingBuffer,
			uploadSize);



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

		//vkCmdCopyBufferToImage(
		//	copyCmd,
		//	stagingBuffer.buffer,
		//	fontImage,
		//	vk::ImageLayout::eTransferDstOptimal,
		//	1,
		//	&bufferCopyRegion);

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


		

		//device->flushCommandBuffer(copyCmd, copyQueue, true);
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
		//VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));
		sampler = context->device.createSampler(samplerInfo, nullptr);

		// Descriptor pool
		std::vector<vk::DescriptorPoolSize> poolSizes = {
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
		};
		vk::DescriptorPoolCreateInfo descriptorPoolInfo = vkx::descriptorPoolCreateInfo(poolSizes, 2);
		//VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));
		descriptorPool = context->device.createDescriptorPool(descriptorPoolInfo, nullptr);

		// Descriptor set layout
		std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings = {
			vkx::descriptorSetLayoutBinding(vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 0),
		};
		vk::DescriptorSetLayoutCreateInfo descriptorLayout = vkx::descriptorSetLayoutCreateInfo(setLayoutBindings);
		//VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));
		descriptorSetLayout = context->device.createDescriptorSetLayout(descriptorLayout, nullptr);

		// Descriptor set
		vk::DescriptorSetAllocateInfo allocInfo = vkx::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		//VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &descriptorSet));
		context->device.allocateDescriptorSets(&allocInfo, &descriptorSet);
		
		vk::DescriptorImageInfo fontDescriptor = vkx::descriptorImageInfo(
			sampler,
			fontView,
			vk::ImageLayout::eShaderReadOnlyOptimal
		);

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
			vkx::writeDescriptorSet(descriptorSet, vk::DescriptorType::eCombinedImageSampler, 0, &fontDescriptor)
		};
		//vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		context->device.updateDescriptorSets(static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		// Pipeline cache
		vk::PipelineCacheCreateInfo pipelineCacheCreateInfo;
		//VK_CHECK_RESULT(vkCreatePipelineCache(device->logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
		pipelineCache = context->device.createPipelineCache(pipelineCacheCreateInfo, nullptr);

		// Pipeline layout
		// Push constants for UI rendering parameters
		
		vk::PushConstantRange pushConstantRange = vkx::pushConstantRange(vk::ShaderStageFlagBits::eVertex, sizeof(PushConstBlock), 0);
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vkx::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		//VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
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
			vkx::vertexInputAttributeDescription(0, 2, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)),	// Location 0: Color
		};

		vk::PipelineVertexInputStateCreateInfo vertexInputState;// = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		pipelineCreateInfo.pVertexInputState = &vertexInputState;


		shaderStages[0] = context->loadShader(vkx::getAssetPath() + "shaders/vulkanscene/imgui/ui.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context->loadShader(vkx::getAssetPath() + "shaders/vulkanscene/imgui/ui.frag.spv", vk::ShaderStageFlagBits::eFragment);

		//VK_CHECK_RESULT(vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
		pipeline = context->device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo);
	}

	// Starts a new imGui frame and sets up windows and ui elements
	void newFrame(vkx::vulkanApp *example, bool updateFrameGraph) {
		ImGui::NewFrame();

		// Init imGui windows and elements

		ImVec4 clear_color = ImColor(114, 144, 154);
		static float f = 0.0f;
		ImGui::Text(example->title.c_str());
		ImGui::Text(context->deviceProperties.deviceName);
		

		// Update frame time display
		if (updateFrameGraph) {
			std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
			float frameTime = 1000.0f / (example->frameTimer * 1000.0f);
			uiSettings.frameTimes.back() = frameTime;
			if (frameTime < uiSettings.frameTimeMin) {
				uiSettings.frameTimeMin = frameTime;
			}
			if (frameTime > uiSettings.frameTimeMax) {
				uiSettings.frameTimeMax = frameTime;
			}
		}

		ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 80));

		ImGui::Text("Camera");
		//ImGui::InputFloat3("position", &example->camera.position.x, 2);
		//ImGui::InputFloat3("rotation", &example->camera.rotation.x, 2);

		ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Example settings");
		ImGui::Checkbox("Render models", &uiSettings.displayModels);
		ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
		ImGui::Checkbox("Display background", &uiSettings.displayBackground);
		ImGui::Checkbox("Animate light", &uiSettings.animateLight);
		ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow();

		// Render to generate draw buffers
		ImGui::Render();
	}

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

		//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		//vkCmdBindPipeline(commandBuffer, vk::PipelineBindPoint::eGraphics, pipeline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		// Bind vertex and index buffer
		vk::DeviceSize offsets[1] = { 0 };
		//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
		commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer.buffer, offsets);
		//vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, vk::IndexType::eUint16);
		commandBuffer.bindIndexBuffer(indexBuffer.buffer, 0, vk::IndexType::eUint16);

		vk::Viewport viewport = vkx::viewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);
		//vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		commandBuffer.setViewport(0, 1, &viewport);

		// UI scale and translate via push constants
		pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		pushConstBlock.translate = glm::vec2(-1.0f);
		//vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);
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
				//vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
				commandBuffer.setScissor(0, 1, &scissorRect);
				//vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				commandBuffer.drawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}

};

// ----------------------------------------------------------------------------
// VulkanExample
// ----------------------------------------------------------------------------

class VulkanExample : public vkx::vulkanApp {
	public:
	ImGUI *imGui = nullptr;

	//// Vertex layout for the models
	//vks::VertexLayout vertexLayout = vks::VertexLayout({
	//	vks::VERTEX_COMPONENT_POSITION,
	//	vks::VERTEX_COMPONENT_NORMAL,
	//	vks::VERTEX_COMPONENT_COLOR,
	//});

	std::vector<vkx::VertexComponent> vertexLayout =
	{
		vkx::VertexComponent::VERTEX_COMPONENT_POSITION,
		vkx::VertexComponent::VERTEX_COMPONENT_NORMAL,
		vkx::VertexComponent::VERTEX_COMPONENT_COLOR,
	};

	//struct Models {
	//	vkx::Model models;
	//	vkx::Model logos;
	//	vkx::Model background;
	//} models;

	//vks::Buffer uniformBufferVS;
	//vkx::CreateBufferResult uniformBufferVS;// scene data
	vkx::TestBuffer uniformBufferVS;// scene data

	struct UBOVS {
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::vec4 lightPos;
	} uboVS;

	vk::PipelineLayout pipelineLayout;
	vk::Pipeline pipeline;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorSet descriptorSet;

	VulkanExample() : vkx::vulkanApp(ENABLE_VALIDATION) {
		title = "Vulkan Example - ImGui";
		//camera.type = Camera::CameraType::lookat;
		//camera.setPosition(glm::vec3(0.0f, 1.4f, -4.8f));
		//camera.setRotation(glm::vec3(4.5f, -380.0f, 0.0f));
		//camera.setPerspective(45.0f, (float)width / (float)height, 0.1f, 256.0f);
	}

	~VulkanExample() {
		//vkDestroyPipeline(device, pipeline, nullptr);
		device.destroyPipeline(pipeline, nullptr);
		//vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		device.destroyPipelineLayout(pipelineLayout, nullptr);
		//vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		device.destroyDescriptorSetLayout(descriptorSetLayout, nullptr);

		//models.models.destroy();
		//models.background.destroy();
		//models.logos.destroy();

		uniformBufferVS.destroy();

		delete imGui;
	}

	void buildCommandBuffers() {
		vk::CommandBufferBeginInfo cmdBufInfo;// = vks::initializers::commandBufferBeginInfo();

		vk::ClearValue clearValues[2];
		//clearValues[0].color = vkx::clearColor({ { 0.2f, 0.2f, 0.2f, 1.0f } });
		clearValues[0].color = vkx::clearColor({ glm::vec4(0.2f, 0.2f, 0.2f, 1.0f) });
		clearValues[1].depthStencil = { 1.0f, 0 };

		vk::RenderPassBeginInfo renderPassBeginInfo;// = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = 1280;// width;
		renderPassBeginInfo.renderArea.extent.height = 720;// height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		imGui->newFrame(this, (frameCounter == 0));

		imGui->updateBuffers();

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
			// Set target frame buffer
			//renderPassBeginInfo.framebuffer = frameBuffers[i];
			renderPassBeginInfo.framebuffer = framebuffers[i];

			//VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
			drawCmdBuffers[i].begin(&cmdBufInfo);

			//vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			drawCmdBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

			vk::Viewport viewport = vkx::viewport((float)/*width*/1280, (float)/*height*/720, 0.0f, 1.0f);
			//vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
			drawCmdBuffers[i].setViewport(0, 1, &viewport);

			vk::Rect2D scissor = vkx::rect2D(/*width*/1280, /*height*/720, 0, 0);
			//vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
			drawCmdBuffers[i].setScissor(0, 1, &scissor);

			// Render scene
			//vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			drawCmdBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

			//vkCmdBindPipeline(drawCmdBuffers[i], vk::PipelineBindPoint::eGraphics, pipeline);
			drawCmdBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

			

			vk::DeviceSize offsets[1] = { 0 };
			//if (uiSettings.displayBackground) {
			//	vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &models.background.vertices.buffer, offsets);
			//	vkCmdBindIndexBuffer(drawCmdBuffers[i], models.background.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			//	vkCmdDrawIndexed(drawCmdBuffers[i], models.background.indexCount, 1, 0, 0, 0);
			//}

			//if (uiSettings.displayModels) {
			//	vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &models.models.vertices.buffer, offsets);
			//	vkCmdBindIndexBuffer(drawCmdBuffers[i], models.models.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			//	vkCmdDrawIndexed(drawCmdBuffers[i], models.models.indexCount, 1, 0, 0, 0);
			//}

			//if (uiSettings.displayLogos) {
			//	vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &models.logos.vertices.buffer, offsets);
			//	vkCmdBindIndexBuffer(drawCmdBuffers[i], models.logos.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			//	vkCmdDrawIndexed(drawCmdBuffers[i], models.logos.indexCount, 1, 0, 0, 0);
			//}

			// Render imGui
			imGui->drawFrame(drawCmdBuffers[i]);

			//vkCmdEndRenderPass(drawCmdBuffers[i]);
			drawCmdBuffers[i].endRenderPass();

			//VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
			drawCmdBuffers[i].end();
		}
	}

	void setupLayoutsAndDescriptors() {
		// descriptor pool
		std::vector<vk::DescriptorPoolSize> poolSizes = {
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
		};
		vk::DescriptorPoolCreateInfo descriptorPoolInfo = vkx::descriptorPoolCreateInfo(poolSizes, 2);
		//VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
		descriptorPool = device.createDescriptorPool(descriptorPoolInfo, nullptr);


		// Set layout
		std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings = {
			vkx::descriptorSetLayoutBinding(vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 0),
		};
		vk::DescriptorSetLayoutCreateInfo descriptorLayout =
			vkx::descriptorSetLayoutCreateInfo(setLayoutBindings);
		//VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));
		descriptorSetLayout = device.createDescriptorSetLayout(descriptorLayout, nullptr);

		// Pipeline layout
		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkx::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		//VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
		pipelineLayout = device.createPipelineLayout(pPipelineLayoutCreateInfo, nullptr);

		// Descriptor set
		vk::DescriptorSetAllocateInfo allocInfo = vkx::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		//VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		device.allocateDescriptorSets(&allocInfo, &descriptorSet);
		
		std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
			vkx::writeDescriptorSet(descriptorSet, vk::DescriptorType::eUniformBuffer, 0, &uniformBufferVS.descriptor),
		};
		//vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		device.updateDescriptorSets(static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void preparePipelines() {
		// Rendering
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkx::pipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology::eTriangleList, {}, VK_FALSE);

		vk::PipelineRasterizationStateCreateInfo rasterizationState =
			vkx::pipelineRasterizationStateCreateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eFront, vk::FrontFace::eCounterClockwise);

		vk::PipelineColorBlendAttachmentState blendAttachmentState =
			vkx::pipelineColorBlendAttachmentState(/*0xf, VK_FALSE*/);

		vk::PipelineColorBlendStateCreateInfo colorBlendState =
			vkx::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

		vk::PipelineDepthStencilStateCreateInfo depthStencilState =
			vkx::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, vk::CompareOp::eLessOrEqual);

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

		// Load shaders
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

		std::vector<vk::VertexInputBindingDescription> vertexInputBindings = {
			vkx::vertexInputBindingDescription(0, /*vertexLayout.stride()*/vkx::vertexSize(vertexLayout), vk::VertexInputRate::eVertex),
		};

		std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes = {
			vkx::vertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0),					// Location 0: Position		
			vkx::vertexInputAttributeDescription(0, 1, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3),	// Location 1: Normal		
			vkx::vertexInputAttributeDescription(0, 2, vk::Format::eR32G32B32Sfloat, sizeof(float) * 6),	// Location 2: Color		
		};
		vk::PipelineVertexInputStateCreateInfo vertexInputState;// = vkx::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		pipelineCreateInfo.pVertexInputState = &vertexInputState;

		shaderStages[0] = context.loadShader(vkx::getAssetPath() + "shaders/vulkanscene/imgui/scene.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(vkx::getAssetPath() + "shaders/vulkanscene/imgui/scene.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
		pipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers() {

		// Vertex shader uniform buffer block
		//VK_CHECK_RESULT(vulkanDevice->createBuffer(
		//	vk::BufferUsageFlagBits::eUniformBuffer,
		//	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		//	&uniformBufferVS,
		//	sizeof(uboVS),
		//	&uboVS));

		//context.createBuffer(
		//	vk::BufferUsageFlagBits::eUniformBuffer,
		//	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		//	&uniformBufferVS,
		//	&uboVS,
		//	sizeof(uboVS));

		context.createBuffer(
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			&uniformBufferVS,
			sizeof(uboVS),
			&uboVS);



		//uniformBufferVS = context.createUniformBuffer(uboVS);


		updateUniformBuffers();
	}

	void updateUniformBuffers() {
		// Vertex shader		
		uboVS.projection = camera.matrices.projection;
		uboVS.modelview = camera.matrices.view * glm::mat4();

		// Light source
		if (uiSettings.animateLight) {
			uiSettings.lightTimer += frameTimer * uiSettings.lightSpeed;
			uboVS.lightPos.x = sin(glm::radians(uiSettings.lightTimer * 360.0f)) * 15.0f;
			uboVS.lightPos.z = cos(glm::radians(uiSettings.lightTimer * 360.0f)) * 15.0f;
		};

		//VK_CHECK_RESULT(uniformBufferVS.map());
		uniformBufferVS.map();
		memcpy(uniformBufferVS.mapped, &uboVS, sizeof(uboVS));
		uniformBufferVS.unmap();
	}

	void draw() {
		vulkanApp::prepareFrame();
		buildCommandBuffers();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		queue.submit(1, &submitInfo, nullptr);

		//drawCurrentCommandBuffer();

		vulkanApp::submitFrame();
	}

	void loadAssets() {
		//models.models.loadFromFile(ASSET_PATH "models/vulkanscenemodels.dae", vertexLayout, 1.0f, vulkanDevice, queue);
		//models.background.loadFromFile(ASSET_PATH "models/vulkanscenebackground.dae", vertexLayout, 1.0f, vulkanDevice, queue);
		//models.logos.loadFromFile(ASSET_PATH "models/vulkanscenelogos.dae", vertexLayout, 1.0f, vulkanDevice, queue);
	}

	void prepareImGui() {
		imGui = new ImGUI(&context);
		imGui->init((float)/*width*/1280, (float)/*height*/720);
		imGui->initResources(renderPass, queue);
	}

	void prepare() {
		vulkanApp::prepare();
		loadAssets();
		prepareUniformBuffers();
		setupLayoutsAndDescriptors();
		preparePipelines();
		prepareImGui();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render() {

		if (!prepared) {
			return;
		}

		// Update imGui
		ImGuiIO& io = ImGui::GetIO();

		io.DisplaySize = ImVec2((float)/*width*/1280, (float)/*height*/720);
		io.DeltaTime = frameTimer;

		// todo: Android touch/gamepad, different platforms
#if defined(_WIN32)
		//io.MousePos = ImVec2(mousePos.x, mousePos.y);
		io.MousePos = ImVec2(mouse.current.x, mouse.current.y);
		io.MouseDown[0] = (((GetKeyState(VK_LBUTTON) & 0x100) != 0));
		io.MouseDown[1] = (((GetKeyState(VK_RBUTTON) & 0x100) != 0));
#else
#endif

		draw();

		if (uiSettings.animateLight) {
			updateUniformBuffers();
		}
	}

	virtual void viewChanged() {
		updateUniformBuffers();
	}

	void updateDrawCommandBuffer(const vk::CommandBuffer &cmdBuffer) override {
	}

	void updateCommandBuffers() override {
	}

};

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {

	VulkanExample* example = new VulkanExample();
	example->run();
	delete(example);
	return 0;
}