/*
* Vulkan Demo Scene
*
* Don't take this a an example, it's more of a personal playground
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* Note : Different license than the other examples!
*
* This code is licensed under the Mozilla Public License Version 2.0 (http://opensource.org/licenses/MPL-2.0)
*/

#include "vulkanApp.h"

std::vector<vkx::VertexLayout> vertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR
};

class VulkanExample : public vkx::vulkanApp {
public:

	vk::PipelineVertexInputStateCreateInfo inputState;
	std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
	std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	vk::Pipeline pipeline;
	vkx::MeshBuffer mesh;

	struct {
		vkx::UniformData meshVS;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 normal;
		glm::mat4 view;
		glm::vec4 lightPos;
	} uboVS;

	struct {
		vk::Pipeline models;
	} pipelines;

	vk::PipelineLayout pipelineLayout;
	vk::DescriptorSet descriptorSet;
	vk::DescriptorSetLayout descriptorSetLayout;

	glm::vec4 lightPos = glm::vec4(0.0f, -3.5f, 0.0f, 0.0f);

	VulkanExample() : vkx::vulkanApp(ENABLE_VALIDATION) {
		size.width = 1280;
		size.height = 720;
		camera.setTranslation({ -5.0, -5.0f, 3.0f });
		//camera.setRotation({ 0.0f, 30.0f, 0.0f });

		camera.matrices.projection = glm::perspective(glm::radians(60.0f), (float)size.width / (float)size.height, 0.001f, 256.0f);

		rotationSpeed = 0.5f;
		title = "Tardis Console - by Bradley Austin Davis";
	}

	~VulkanExample() {
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		device.destroyPipeline(pipelines.models);
		device.destroyPipelineLayout(pipelineLayout);
		device.destroyDescriptorSetLayout(descriptorSetLayout);
		uniformData.meshVS.destroy();
		mesh.destroy();
	}

	void updateDrawCommandBuffer(const vk::CommandBuffer& cmdBuffer) {
		cmdBuffer.setViewport(0, vkx::viewport(size));
		cmdBuffer.setScissor(0, vkx::rect2D(size));
		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.models);
		cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, mesh.vertices.buffer, { 0 });
		cmdBuffer.bindIndexBuffer(mesh.indices.buffer, 0, vk::IndexType::eUint32);
		cmdBuffer.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
	}

	void prepareVertices() {
		struct Vertex {
			float pos[3];
			float normal[3];
			float color[3];
		};

		auto assetPath = getAssetPath() + "models/console.fbx";
		vkx::MeshLoader loader;
		if (!loader.load(assetPath)) {
			throw std::runtime_error("Failed to load model");
		}
		mesh = loader.createBuffers(context, vertexLayout, 0.01f);

		// Binding description
		bindingDescriptions.resize(1);
		bindingDescriptions[0] =
			vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Vertex), vk::VertexInputRate::eVertex);

		// Attribute descriptions
		// Location 0 : Position
		attributeDescriptions.resize(3);
		attributeDescriptions[0] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos));
		// Location 1 : Normal
		attributeDescriptions[1] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal));
		// Location 3 : Color
		attributeDescriptions[2] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));

		inputState.vertexBindingDescriptionCount = bindingDescriptions.size();
		inputState.pVertexBindingDescriptions = bindingDescriptions.data();
		inputState.vertexAttributeDescriptionCount = attributeDescriptions.size();
		inputState.pVertexAttributeDescriptions = attributeDescriptions.data();
	}

	void setupDescriptorPool() {
		// Example uses one ubo and one image sampler
		std::vector<vk::DescriptorPoolSize> poolSizes =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
		};

		vk::DescriptorPoolCreateInfo descriptorPoolInfo =
			vkx::descriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 2);

		descriptorPool = device.createDescriptorPool(descriptorPoolInfo);
	}

	void setupDescriptorSetLayout() {
		std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout =
			vkx::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), setLayoutBindings.size());
		descriptorSetLayout = device.createDescriptorSetLayout(descriptorLayout);


		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkx::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		pipelineLayout = device.createPipelineLayout(pPipelineLayoutCreateInfo);
	}

	void setupDescriptorSet() {
		vk::DescriptorSetAllocateInfo allocInfo =
			vkx::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

		descriptorSet = device.allocateDescriptorSets(allocInfo)[0];

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				descriptorSet,
				vk::DescriptorType::eUniformBuffer,
				0,
				&uniformData.meshVS.descriptor),
		};

		device.updateDescriptorSets(writeDescriptorSets, nullptr);
	}

	void preparePipelines() {
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;

		vk::PipelineRasterizationStateCreateInfo rasterizationState =
			vkx::pipelineRasterizationStateCreateInfo(
				vk::PolygonMode::eFill,
				vk::CullModeFlagBits::eBack,
				vk::FrontFace::eClockwise);

		vk::PipelineColorBlendAttachmentState blendAttachmentState;
		blendAttachmentState.colorWriteMask = vkx::fullColorWriteMask();

		vk::PipelineColorBlendStateCreateInfo colorBlendState;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;

		vk::PipelineViewportStateCreateInfo viewportState;
		viewportState.scissorCount = 1;
		viewportState.viewportCount = 1;

		vk::PipelineMultisampleStateCreateInfo multisampleState;

		std::vector<vk::DynamicState> dynamicStateEnables = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};
		vk::PipelineDynamicStateCreateInfo dynamicState =
			vkx::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());

		// vk::Pipeline for the meshes (armadillo, bunny, etc.)
		// Load shaders
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/tardisconsole/mesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/tardisconsole/mesh.frag.spv", vk::ShaderStageFlagBits::eFragment);

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo =
			vkx::pipelineCreateInfo(pipelineLayout, renderPass);

		pipelineCreateInfo.pVertexInputState = &inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		pipelines.models = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers() {
		// Vertex shader uniform buffer block
		uniformData.meshVS = context.createUniformBuffer(uboVS);
		updateUniformBuffers();
	}

	void updateUniformBuffers() {
		uboVS.projection = getProjection();

		uboVS.view = camera.matrices.view;
		//uboVS.model = glm::scale(glm::mat4(), vec3(10.0));
		uboVS.normal = glm::inverseTranspose(uboVS.view * uboVS.model);
		uboVS.lightPos = lightPos;
		uniformData.meshVS.copy(uboVS);
	}

	void prepare() {
		vulkanApp::prepare();
		prepareVertices();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		updateDrawCommandBuffers();
		prepared = true;
	}

	virtual void render() {
		if (!prepared)
			return;
		draw();
	}

	virtual void viewChanged() {
		updateUniformBuffers();
	}

};

//RUN_EXAMPLE(VulkanExample)
VulkanExample *vulkanExample;

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	VulkanExample* example = new VulkanExample();
	example->run();
	delete(example);
	return 0;
}