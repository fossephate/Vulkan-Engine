/*
* Vulkan Example - Animated gears using multiple uniform buffers
*
* See readme.md for details
*
* Copyright (C) 2015 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanShapes.h"

vkx::ShapesRenderer::ShapesRenderer(const vkx::Context & context, bool stereo) : Parent(context), stereo(stereo) {
	srand((uint32_t)time(NULL));
}

vkx::ShapesRenderer::~ShapesRenderer() {
	queue.waitIdle();
	device.waitIdle();
	context.device.freeCommandBuffers(cmdPool, cmdBuffer);
	context.device.destroyPipeline(pipelines.solid);
	context.device.destroyPipelineLayout(pipelineLayout);
	context.device.destroyDescriptorSetLayout(descriptorSetLayout);
	uniformData.vsScene.destroy();
}

void vkx::ShapesRenderer::buildCommandBuffer() {
	if (!cmdBuffer) {
		vk::CommandBufferAllocateInfo cmdBufAllocateInfo;
		cmdBufAllocateInfo.commandPool = cmdPool;
		cmdBufAllocateInfo.commandBufferCount = 1;
		cmdBufAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
		cmdBuffer = context.device.allocateCommandBuffers(cmdBufAllocateInfo)[0];
	}

	cmdBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

	vk::CommandBufferBeginInfo cmdBufInfo;
	cmdBufInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
	cmdBuffer.begin(cmdBufInfo);

	vk::ClearValue clearValues[2];
	clearValues[0].color = vkx::clearColor(glm::vec4(0.2f, 0.2f, 0.2f, 1));
	clearValues[1].depthStencil = { 1.0f, 0 };

	vkx::setImageLayout(cmdBuffer, framebuffer.colors[0].image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
	vkx::setImageLayout(cmdBuffer, framebuffer.depth.image, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::RenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.extent.width = framebufferSize.x;
	renderPassBeginInfo.renderArea.extent.height = framebufferSize.y;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = framebuffer.framebuffer;
	cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
	cmdBuffer.setScissor(0, vkx::rect2D(framebufferSize));
	if (stereo) {
		auto viewport = vkx::viewport(framebufferSize);
		viewport.width /= 2.0f;
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.solid);
		// Binding point 0 : Mesh vertex buffer
		cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshes.buffer, { 0 });
		// Binding point 1 : Instance data buffer
		cmdBuffer.bindVertexBuffers(INSTANCE_BUFFER_BIND_ID, instanceBuffer.buffer, { 0 });
		for (uint32_t i = 0; i < 2; ++i) {
			cmdBuffer.setViewport(0, viewport);
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, { (uint32_t)i * (uint32_t)uniformData.vsScene.alignment });
			cmdBuffer.drawIndirect(indirectBuffer.buffer, 0, SHAPES_COUNT, sizeof(vk::DrawIndirectCommand));
			viewport.x += viewport.width;
		}
	} else {
		cmdBuffer.setViewport(0, vkx::viewport(framebufferSize));
		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.solid);
		// Binding point 0 : Mesh vertex buffer
		cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshes.buffer, { 0 });
		// Binding point 1 : Instance data buffer
		cmdBuffer.bindVertexBuffers(INSTANCE_BUFFER_BIND_ID, instanceBuffer.buffer, { 0 });
		cmdBuffer.drawIndirect(indirectBuffer.buffer, 0, SHAPES_COUNT, sizeof(vk::DrawIndirectCommand));
	}
	cmdBuffer.endRenderPass();
	cmdBuffer.end();
}

void vkx::ShapesRenderer::loadShapes() {
	std::vector<Vertex> vertexData;
	size_t vertexCount = 0;
	appendShape<>(geometry::tetrahedron(), vertexData);
	appendShape<>(geometry::octahedron(), vertexData);
	appendShape<>(geometry::cube(), vertexData);
	appendShape<>(geometry::dodecahedron(), vertexData);
	appendShape<>(geometry::icosahedron(), vertexData);
	for (auto& vertex : vertexData) {
		vertex.position *= 0.2f;
	}
	meshes = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexData);
}

void vkx::ShapesRenderer::setupDescriptorPool() {
	// Example uses one ubo
	std::vector<vk::DescriptorPoolSize> poolSizes =
	{
		vkx::descriptorPoolSize(uniformType, 1),
	};

	vk::DescriptorPoolCreateInfo descriptorPoolInfo =
		vkx::descriptorPoolCreateInfo((uint32_t)poolSizes.size(), poolSizes.data(), 1);

	descriptorPool = context.device.createDescriptorPool(descriptorPoolInfo);
}

void vkx::ShapesRenderer::setupDescriptorSetLayout() {
	// Binding 0 : Vertex shader uniform buffer
	std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings =
	{
		vkx::descriptorSetLayoutBinding(uniformType, vk::ShaderStageFlagBits::eVertex, 0),
	};

	descriptorSetLayout = context.device.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount((uint32_t)setLayoutBindings.size())
		.setPBindings(setLayoutBindings.data()));

	pipelineLayout = context.device.createPipelineLayout(
		vk::PipelineLayoutCreateInfo()
		.setPSetLayouts(&descriptorSetLayout)
		.setSetLayoutCount(1));
}

void vkx::ShapesRenderer::setupDescriptorSet() {
	vk::DescriptorSetAllocateInfo allocInfo =
		vkx::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

	descriptorSet = context.device.allocateDescriptorSets(allocInfo)[0];

	// Binding 0 : Vertex shader uniform buffer
	vk::WriteDescriptorSet writeDescriptorSet;
	writeDescriptorSet.dstSet = descriptorSet;
	writeDescriptorSet.descriptorType = uniformType;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.pBufferInfo = &uniformData.vsScene.descriptor;
	writeDescriptorSet.descriptorCount = 1;

	context.device.updateDescriptorSets(writeDescriptorSet, nullptr);
}

void vkx::ShapesRenderer::preparePipelines() {
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState =
		vkx::pipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology::eTriangleList);

	vk::PipelineRasterizationStateCreateInfo rasterizationState =
		vkx::pipelineRasterizationStateCreateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise);

	vk::PipelineColorBlendAttachmentState blendAttachmentState =
		vkx::pipelineColorBlendAttachmentState();

	vk::PipelineColorBlendStateCreateInfo colorBlendState =
		vkx::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

	vk::PipelineDepthStencilStateCreateInfo depthStencilState =
		vkx::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, vk::CompareOp::eLessOrEqual);

	vk::PipelineViewportStateCreateInfo viewportState =
		vkx::pipelineViewportStateCreateInfo(1, 1);

	vk::PipelineMultisampleStateCreateInfo multisampleState =
		vkx::pipelineMultisampleStateCreateInfo(vk::SampleCountFlagBits::e1);

	std::vector<vk::DynamicState> dynamicStateEnables = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};
	vk::PipelineDynamicStateCreateInfo dynamicState =
		vkx::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), (uint32_t)dynamicStateEnables.size());

	// Instacing pipeline
	// Load shaders
	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
	{
		vkx::shader::initGlsl();
		shaderStages[0] = context.loadGlslShader(getAssetPath() + "shaders/indirect/indirect.vert", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadGlslShader(getAssetPath() + "shaders/indirect/indirect.frag", vk::ShaderStageFlagBits::eFragment);
		vkx::shader::finalizeGlsl();
	}

	std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
	std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

	// Binding description
	bindingDescriptions.resize(2);

	// Mesh vertex buffer (description) at binding point 0
	bindingDescriptions[0] =
		vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Vertex), vk::VertexInputRate::eVertex);
	bindingDescriptions[1] =
		vkx::vertexInputBindingDescription(INSTANCE_BUFFER_BIND_ID, sizeof(InstanceData), vk::VertexInputRate::eInstance);

	// Attribute descriptions
	// Describes memory layout and shader positions
	attributeDescriptions.clear();

	// Per-Vertex attributes
	// Location 0 : Position
	attributeDescriptions.push_back(
		vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)));
	// Location 1 : Color
	attributeDescriptions.push_back(
		vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)));
	// Location 2 : Normal
	attributeDescriptions.push_back(
		vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)));

	// Instanced attributes
	// Location 4 : Position
	attributeDescriptions.push_back(
		vkx::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 4, vk::Format::eR32G32B32Sfloat, offsetof(InstanceData, pos)));
	// Location 5 : Rotation
	attributeDescriptions.push_back(
		vkx::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 5, vk::Format::eR32G32B32A32Sfloat, offsetof(InstanceData, rot)));
	// Location 6 : Scale
	attributeDescriptions.push_back(
		vkx::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 6, vk::Format::eR32Sfloat, offsetof(InstanceData, scale)));

	vk::GraphicsPipelineCreateInfo pipelineCreateInfo =
		vkx::pipelineCreateInfo(pipelineLayout, renderPass);

	vk::PipelineVertexInputStateCreateInfo vertexInputState;
	vertexInputState.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
	vertexInputState.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputState.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
	vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.stageCount = (uint32_t)shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();

	pipelines.solid = context.device.createGraphicsPipelines(context.pipelineCache, pipelineCreateInfo, nullptr)[0];
}

void vkx::ShapesRenderer::prepareIndirectData() {
	std::vector<vk::DrawIndirectCommand> indirectData;
	indirectData.resize(SHAPES_COUNT);
	for (auto i = 0; i < SHAPES_COUNT; ++i) {
		auto& drawIndirectCommand = indirectData[i];
		const auto& shapeData = shapes[i];
		drawIndirectCommand.firstInstance = i * INSTANCES_PER_SHAPE;
		drawIndirectCommand.instanceCount = INSTANCES_PER_SHAPE;
		drawIndirectCommand.firstVertex = (uint32_t)shapeData.baseVertex;
		drawIndirectCommand.vertexCount = (uint32_t)shapeData.vertices;
	}
	indirectBuffer = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndirectBuffer, indirectData);
}

void vkx::ShapesRenderer::prepareInstanceData() {
	std::vector<InstanceData> instanceData;
	instanceData.resize(INSTANCE_COUNT);

	std::mt19937 rndGenerator((uint32_t)time(nullptr));
	std::uniform_real_distribution<float> uniformDist(0.0, 1.0);
	std::exponential_distribution<float> expDist(1);

	for (auto i = 0; i < INSTANCE_COUNT; i++) {
		auto& instance = instanceData[i];
		instance.rot = glm::vec3(M_PI * uniformDist(rndGenerator), M_PI * uniformDist(rndGenerator), M_PI * uniformDist(rndGenerator));
		float theta = 2 * (float)M_PI * uniformDist(rndGenerator);
		float phi = acos(1 - 2 * uniformDist(rndGenerator));
		instance.scale = 0.1f + expDist(rndGenerator) * 3.0f;
		instance.pos = glm::normalize(glm::vec3(sin(phi) * cos(theta), sin(theta), cos(phi)));
		instance.pos *= instance.scale * (1.0f + expDist(rndGenerator) / 2.0f) * 4.0f;
	}

	instanceBuffer = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, instanceData);
}

void vkx::ShapesRenderer::prepareUniformBuffers() {
	uniformData.vsScene = context.createUniformBuffer(uboVS, 2);
}

void vkx::ShapesRenderer::prepare() {
	depthFormat = vkx::getSupportedDepthFormat(context.physicalDevice);
	OffscreenRenderer::prepare();
	loadShapes();
	prepareInstanceData();
	prepareIndirectData();
	prepareUniformBuffers();
	setupDescriptorSetLayout();
	preparePipelines();
	setupDescriptorPool();
	setupDescriptorSet();
	buildCommandBuffer();
}

void vkx::ShapesRenderer::update(float deltaTime, const glm::mat4 & projection, const glm::mat4 & view) {
	update(deltaTime, { projection, projection }, { view, view });
}

void vkx::ShapesRenderer::update(float deltaTime, const std::array<glm::mat4, 2>& projections, const std::array<glm::mat4, 2>& views) {
	uboVS.time += deltaTime * 0.05f;
	uboVS.projection = projections[0];
	uboVS.view = views[0];
	uniformData.vsScene.copy(uboVS);

	uboVS.projection = projections[1];
	uboVS.view = views[1];
	uniformData.vsScene.copy(uboVS, uniformData.vsScene.alignment);
	#if 0
	frameTimer = deltaTime;
	if (!paused) {
		accumulator += frameTimer;
		if (accumulator < duration) {
			zoom = easings::inOutQuint(accumulator, duration, zoomStart, zoomDelta);
			updateUniformBuffer(true);
		} else {
			updateUniformBuffer(false);
		}

		if (accumulator >= interval) {
			accumulator = 0;
			zoomStart = zoom;
			if (zoom < -2) {
				zoomDelta = 135;
			} else {
				zoomDelta = -135;
			}
		}
	}
	#endif
}

void vkx::ShapesRenderer::render() {
	std::vector<vk::Semaphore> waitSemaphores;
	std::vector<vk::PipelineStageFlags> waitStages;
	waitSemaphores.push_back(semaphores.renderStart);
	waitStages.push_back(vk::PipelineStageFlagBits::eBottomOfPipe);
	std::vector<vk::Semaphore> signalSemaphores;
	signalSemaphores.push_back(semaphores.renderComplete);

	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.signalSemaphoreCount = (uint32_t)signalSemaphores.size();
	submitInfo.pSignalSemaphores = signalSemaphores.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	context.queue.submit(submitInfo, VK_NULL_HANDLE);
}
