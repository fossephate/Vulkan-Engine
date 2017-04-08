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

#include "vulkanImgui.hpp"

#define ENABLE_VALIDATION false






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

	vkx::CreateBufferResult uniformBufferVS;// scene data
	//vkx::TestBuffer uniformBufferVS;// scene data

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
		device.destroyPipeline(pipeline, nullptr);
		device.destroyPipelineLayout(pipelineLayout, nullptr);
		device.destroyDescriptorSetLayout(descriptorSetLayout, nullptr);

		//models.models.destroy();
		//models.background.destroy();
		//models.logos.destroy();

		uniformBufferVS.destroy();

		delete imGui;
	}

	void buildCommandBuffers() {
		vk::CommandBufferBeginInfo cmdBufInfo;

		vk::ClearValue clearValues[2];
		//clearValues[0].color = vkx::clearColor({ { 0.2f, 0.2f, 0.2f, 1.0f } });
		clearValues[0].color = vkx::clearColor({ glm::vec4(0.2f, 0.2f, 0.2f, 1.0f) });
		clearValues[1].depthStencil = { 1.0f, 0 };

		vk::RenderPassBeginInfo renderPassBeginInfo;
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
			renderPassBeginInfo.framebuffer = framebuffers[i];

			drawCmdBuffers[i].begin(&cmdBufInfo);

			drawCmdBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

			vk::Viewport viewport = vkx::viewport((float)settings.windowSize.width, (float)settings.windowSize.height, 0.0f, 1.0f);
			drawCmdBuffers[i].setViewport(0, 1, &viewport);

			vk::Rect2D scissor = vkx::rect2D(settings.windowSize.width, settings.windowSize.height, 0, 0);
			drawCmdBuffers[i].setScissor(0, 1, &scissor);

			// Render scene
			drawCmdBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

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

			drawCmdBuffers[i].endRenderPass();
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
		descriptorPool = device.createDescriptorPool(descriptorPoolInfo, nullptr);


		// Set layout
		std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings = {
			vkx::descriptorSetLayoutBinding(vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 0),
		};
		vk::DescriptorSetLayoutCreateInfo descriptorLayout = vkx::descriptorSetLayoutCreateInfo(setLayoutBindings);
		descriptorSetLayout = device.createDescriptorSetLayout(descriptorLayout, nullptr);

		// Pipeline layout
		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkx::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		pipelineLayout = device.createPipelineLayout(pPipelineLayoutCreateInfo, nullptr);

		// Descriptor set
		vk::DescriptorSetAllocateInfo allocInfo = vkx::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		device.allocateDescriptorSets(&allocInfo, &descriptorSet);
		
		std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
			vkx::writeDescriptorSet(descriptorSet, vk::DescriptorType::eUniformBuffer, 0, &uniformBufferVS.descriptor),
		};
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
			vkx::vertexInputBindingDescription(0, vkx::vertexSize(vertexLayout), vk::VertexInputRate::eVertex),
		};

		std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes = {
			vkx::vertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0),					// Location 0: Position		
			vkx::vertexInputAttributeDescription(0, 1, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3),	// Location 1: Normal		
			vkx::vertexInputAttributeDescription(0, 2, vk::Format::eR32G32B32Sfloat, sizeof(float) * 6),	// Location 2: Color		
		};
		vk::PipelineVertexInputStateCreateInfo vertexInputState;
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		pipelineCreateInfo.pVertexInputState = &vertexInputState;

		shaderStages[0] = context.loadShader(vkx::getAssetPath() + "shaders/vulkanscene/imgui/scene.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(vkx::getAssetPath() + "shaders/vulkanscene/imgui/scene.frag.spv", vk::ShaderStageFlagBits::eFragment);
		pipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers() {

		// Vertex shader uniform buffer block

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
		imGui->init((float)settings.windowSize.width, (float)settings.windowSize.height);
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
		ImGuiIO &io = ImGui::GetIO();

		io.DisplaySize = ImVec2((float)settings.windowSize.width, (float)settings.windowSize.height);
		io.DeltaTime = frameTimer*1000;

		// todo: Android touch/gamepad, different platforms
#if defined(_WIN32)
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