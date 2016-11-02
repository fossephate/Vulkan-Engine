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

//static std::vector<std::string> names{ "logos", "background", "models", "skybox" };

std::vector<vkx::VertexLayout> vertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR
};




class VulkanExample : public vkx::vulkanApp {

public:



	vk::PipelineVertexInputStateCreateInfo inputState;
	std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
	std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

	std::vector<vkx::Mesh> meshes;

	vkx::Mesh skyboxMesh;

	struct {
		vkx::UniformData meshVS;
		vkx::UniformData dynamic;
	} uniformData;

	struct {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;

		glm::mat4 normal;

		glm::vec4 lightPos;
	} uboVS;

	struct {
		glm::mat4 transform;
	} matrixData;

	struct {
		vkx::Texture skybox;
	} textures;

	struct {
		vk::Pipeline meshes;
		vk::Pipeline skybox;
	} pipelines;

	vk::PipelineLayout pipelineLayout;
	//vk::DescriptorSet descriptorSet;
	//vk::DescriptorSetLayout descriptorSetLayout;

	//std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
	//std::vector<vk::DescriptorSet> descriptorSets;
	//std::vector<vk::DescriptorPool> descriptorPools;

	vk::DescriptorSetLayout descriptorSetLayouts[2];
	vk::DescriptorSet descriptorSets[2];
	vk::DescriptorPool descriptorPools[2];
	


	//glm::vec4 lightPos = glm::vec4(2.0f, 2.0f, 5.0f, 0.0f);
	glm::vec4 lightPos = glm::vec4(1.0f, 2.0f, 0.0f, 0.0f);

	VulkanExample() : vkx::vulkanApp(ENABLE_VALIDATION) {
		size.width = 1280;
		size.height = 720;


		camera.setTranslation({ -1.0f, 1.0f, 3.0f });

		

		//camera.matrices.projection = glm::perspectiveRH(glm::radians(60.0f), (float)size.width / (float)size.height, 0.0001f, 256.0f);

		title = "Vulkan Demo Scene";
	}

	~VulkanExample() {
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		// destroy pipelines
		device.destroyPipeline(pipelines.meshes);
		device.destroyPipeline(pipelines.skybox);

		device.destroyPipelineLayout(pipelineLayout);
		//device.destroyDescriptorSetLayout(descriptorSetLayout);

		uniformData.meshVS.destroy();

		for (auto& mesh : meshes) {
			device.destroyBuffer(mesh.meshBuffer.vertices.buffer);
			device.freeMemory(mesh.meshBuffer.vertices.memory);

			device.destroyBuffer(mesh.meshBuffer.indices.buffer);
			device.freeMemory(mesh.meshBuffer.indices.memory);
		}

		textures.skybox.destroy();

	}

	void loadTextures() {
		textures.skybox = textureLoader->loadCubemap(getAssetPath() + "textures/cubemap_vulkan.ktx", vk::Format::eR8G8B8A8Unorm);
	}

	void updateDrawCommandBuffer(const vk::CommandBuffer& cmdBuffer) {
		cmdBuffer.setViewport(0, vkx::viewport(size));
		cmdBuffer.setScissor(0, vkx::rect2D(size));


		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[0], nullptr);

		//https://github.com/nvpro-samples/gl_vk_threaded_cadscene/blob/master/doc/vulkan_uniforms.md


		// left:
		//vk::Viewport viewport = vkx::viewport((float)size.width / 3, (float)size.height, 0.0f, 1.0f);
		//cmdBuffer.setViewport(0, viewport);
		// center
		//viewport.x += viewport.width;
		//cmdBuffer.setViewport(0, viewport);

		//uboVS.model = glm::translate(uboVS.model, glm::vec3(0.0f, 5.0f, 0.0f));

		

		//meshes[1].model = glm::translate(glm::mat4(), glm::vec3(0.0f, 10.0f, 0.0f));

		meshes[1].model = glm::translate(glm::mat4(), glm::vec3(5.0f, 0.0f, 0.0f));
		//uboVS.model = meshes[1].model;

		//meshes[1].orientation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		int i = 1;

		for (auto &mesh : meshes) {


			//uboVS.model = mesh.model;
			//uboVS.model = glm::mat4_cast(mesh.orientation);
			//updateUniformBuffers();

			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mesh.pipeline);
			cmdBuffer.bindVertexBuffers(mesh.vertexBufferBinding, mesh.meshBuffer.vertices.buffer, vk::DeviceSize());
			cmdBuffer.bindIndexBuffer(mesh.meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[0], nullptr);


			//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[1], nullptr);



			cmdBuffer.drawIndexed(mesh.meshBuffer.indexCount, 1, 0, 0, 0);


			//mesh.drawIndexed(cmdBuffer);
			i += 1;
		}

		uboVS.model = glm::mat4();

	}

	void prepareVertices() {

		struct Vertex {
			float pos[3];
			float normal[3];
			float uv[2];
			float color[3];
		};

		// re-usable? meshloader class// definitely not reusable// important
		vkx::MeshLoader* loader = new vkx::MeshLoader();

		loader->load(getAssetPath() + "models/xyplane.dae");
		vkx::Mesh planeMesh = loader->createMeshFromBuffers(context, vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);


		loader = new vkx::MeshLoader();

		loader->load(getAssetPath() + "models/vulkanscenemodels.dae");
		vkx::Mesh otherMesh1 = loader->createMeshFromBuffers(context, vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//loader = new vkx::MeshLoader();

		//loader->load(getAssetPath() + "models/cube.obj");
		//vkx::Mesh otherMesh2 = loader->createMeshFromBuffers(context, vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//loader->load(getAssetPath() + "models/cube.obj");
		//skyboxMesh = loader->createMeshFromBuffers(context, vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);


		// better:
		//vkx::Mesh planeMesh;
		//planeMesh.load(getAssetPath() + "models/xyplane.dae");
		//planeMesh.createBuffers(context, vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//vkx::Mesh otherMesh1;
		//otherMesh1.load(getAssetPath() + "models/vulkanscenemodels.dae");
		//otherMesh1.createBuffers(context, vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);




		//meshes.push_back(skyboxMesh);
		meshes.push_back(planeMesh);
		meshes.push_back(otherMesh1);
		//meshes.push_back(otherMesh2);


		// Binding description
		bindingDescriptions.resize(1);
		bindingDescriptions[0] =
			vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Vertex), vk::VertexInputRate::eVertex);

		// Attribute descriptions
		attributeDescriptions.resize(4);
		// Location 0 : Position
		attributeDescriptions[0] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);
		// Location 1 : Normal
		attributeDescriptions[1] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3);
		// Location 2 : Texture coordinates
		attributeDescriptions[2] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32Sfloat, sizeof(float) * 6);
		// Location 3 : Color
		attributeDescriptions[3] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat, sizeof(float) * 8);

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
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1),
			//vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1)
		};

		vk::DescriptorPoolCreateInfo descriptorPoolInfo =
			vkx::descriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 2);

		descriptorPool = device.createDescriptorPool(descriptorPoolInfo);





		//vk::DescriptorPool descPool;
		////descriptorSets.push_back(vk::DescriptorSet());
		////descriptorSets.push_back(vk::DescriptorSet());
		////descriptorSets.push_back(vk::DescriptorSet());

		//uint32_t maxcounts[2];
		//maxcounts[0] = 1;
		//maxcounts[1] = 1;




		//for (int i = 0; i < 1; i++) {

		//	
		//	vk::DescriptorPool descrPool;// where the descriptor pool will be stored

		//	vk::DescriptorPoolCreateInfo descrPoolInfo;
		//	descrPoolInfo.maxSets = maxcounts[i];
		//	//descrPoolInfo.maxSets = 1;
		//	descrPoolInfo.poolSizeCount = 1;
		//	
		//	descrPoolInfo.pPoolSizes = &poolSizes[i];

		//	// scene pool
		//	descrPool = device.createDescriptorPool(descrPoolInfo, nullptr);

		//	descriptorPools[i] = descrPool;

		//	//vk::DescriptorSetLayout setLayouts[] = { descriptorSetLayouts[0], descriptorSetLayouts[1] };

		//	//device.allocateDescriptorSets(a)

		//	vk::DescriptorSetAllocateInfo allocInfo = vkx::descriptorSetAllocateInfo(descrPool, descriptorSetLayouts + i, 1);

		//	device.allocateDescriptorSets(&allocInfo, &descriptorSets[i]);


		//	//for (uint32_t n = 0; n < maxcounts[i]; n++) {

		//	//	//vk::DescriptorSetAllocateInfo allocInfo;
		//	//	//allocInfo.descriptorPool = descrPool;
		//	//	//allocInfo.descriptorSetCount = 1;
		//	//	//allocInfo.pSetLayouts = descriptorSetLayouts + i;

		//	//	//vkx::descriptorSetAllocateInfo(descriptorPool, setLayout

		//	//	// do one at a time, as we don't have layouts in maxcounts-many pointer array
		//	//	//result = vkAllocateDescriptorSets(m_device, &allocInfo, setstores[i] + n);
		//	//	//device.allocateDescriptorSets(&allocInfo, (&descriptorSets[i] + n));

		//	//	//assert(result == VK_SUCCESS);

		//	//}
		//}



	}






	void setupDescriptorSetLayout() {
		//std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings =
		//{
		//	// Binding 0 : Vertex shader uniform buffer
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBuffer,
		//		vk::ShaderStageFlagBits::eVertex,
		//		0),
		//	// Binding 1 : Fragment shader color map image sampler
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eCombinedImageSampler,
		//		vk::ShaderStageFlagBits::eFragment,
		//		1),
		//	//// Binding 2 : vertex test uniform
		//	//vkx::descriptorSetLayoutBinding(
		//	//	vk::DescriptorType::eUniformBufferDynamic,
		//	//	vk::ShaderStageFlagBits::eVertex,
		//	//	1),


		//};

		//vk::DescriptorSetLayoutCreateInfo descriptorSetEntry =
		//	vkx::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), setLayoutBindings.size());

		//descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetEntry);


		//vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
		//	vkx::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);

		//pipelineLayout = device.createPipelineLayout(pPipelineLayoutCreateInfo);


		// bindings for descriptor set layout 1
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayout1Bindings =// rename to descriptorSetLayout1Bindings?
		{
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0

			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1)// binding 1
		};


		// bindings for descriptor set layout 2
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayout2Bindings =
		{
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,// dynamic
				vk::ShaderStageFlagBits::eVertex,// only vertex
				0)// binding 0
		};


		// create a descriptorSetLayout for each set of layout bindings
		//std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

		// create info
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayout1Info =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayout1Bindings.data()+0, descriptorSetLayout1Bindings.size());

		//vk::DescriptorSetLayout descriptorSetLayout1 = device.createDescriptorSetLayout(descriptorSetLayout1Info);
		//descriptorSetLayouts.push_back(descriptorSetLayout1);
		descriptorSetLayouts[0] = device.createDescriptorSetLayout(descriptorSetLayout1Info);

		

		// create info
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayout2Info =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayout2Bindings.data(), descriptorSetLayout2Bindings.size());

		//vk::DescriptorSetLayout descriptorSetLayout2 = device.createDescriptorSetLayout(descriptorSetLayout2Info);
		//descriptorSetLayouts.push_back(descriptorSetLayout2);

		//descriptorSetLayouts[1] = device.createDescriptorSetLayout(descriptorSetLayout2Info);


		vk::DescriptorSetLayout setLayouts[] = { descriptorSetLayouts[0]/*, descriptorSetLayouts[1]*/ };

		#define NV_ARRAYSIZE(arr) (sizeof(arr)/sizeof(arr[0]))


		//vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
		//	vkx::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);

		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo;
		pPipelineLayoutCreateInfo.setLayoutCount = NV_ARRAYSIZE(setLayouts);
		pPipelineLayoutCreateInfo.pSetLayouts = setLayouts;

		pipelineLayout = device.createPipelineLayout(pPipelineLayoutCreateInfo);




	}




	void setupDescriptorSet() {


		/*vk::DescriptorSetAllocateInfo allocInfo =
			vkx::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

		descriptorSet = device.allocateDescriptorSets(allocInfo)[0];*/


		//// Cube map image descriptor
		//vk::DescriptorImageInfo texDescriptorCubeMap =
		//	vkx::descriptorImageInfo(textures.skybox.sampler, textures.skybox.view, vk::ImageLayout::eGeneral);

		//std::vector<vk::WriteDescriptorSet> writeDescriptorSets =
		//{
		//	// Binding 0 : Vertex shader uniform buffer
		//	vkx::writeDescriptorSet(
		//		descriptorSet,
		//		vk::DescriptorType::eUniformBuffer,
		//		0,
		//		&uniformData.meshVS.descriptor),
		//	// Binding 1 : Fragment shader image sampler
		//	vkx::writeDescriptorSet(
		//		descriptorSet,
		//		vk::DescriptorType::eCombinedImageSampler,
		//		1,
		//		&texDescriptorCubeMap)
		//};

		//device.updateDescriptorSets(writeDescriptorSets, nullptr);




		vk::DescriptorImageInfo texDescriptorCubeMap =
			vkx::descriptorImageInfo(textures.skybox.sampler, textures.skybox.view, vk::ImageLayout::eGeneral);

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets{

			// Binding 0 : Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				descriptorSets[0],// destination descriptor set
				vk::DescriptorType::eUniformBuffer,
				0,
				&uniformData.meshVS.descriptor),


			// Binding 1 : Fragment shader image sampler
			vkx::writeDescriptorSet(
				descriptorSets[0],// destination descriptor set
				vk::DescriptorType::eCombinedImageSampler,
				1,
				&texDescriptorCubeMap),

			// Binding 0 : Vertex shader dynamic uniform buffer
			//vkx::writeDescriptorSet(
			//	descriptorSets[1],// destination descriptor set
			//	vk::DescriptorType::eUniformBufferDynamic,
			//	0,
			//	&uniformData.dynamic.descriptor),
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
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/mesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/mesh.frag.spv", vk::ShaderStageFlagBits::eFragment);

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

		pipelines.meshes = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];


		// vk::Pipeline for the logos
		//shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/logo.vert.spv", vk::ShaderStageFlagBits::eVertex);
		//shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/logo.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//pipelines.logos = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];


		// vk::Pipeline for the sky box
		//rasterizationState.cullMode = vk::CullModeFlagBits::eFront; // Inverted culling
		//depthStencilState.depthWriteEnable = VK_FALSE; // No depth writes
		//shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/skybox.vert.spv", vk::ShaderStageFlagBits::eVertex);
		//shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/skybox.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//pipelines.skybox = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];


		// Assign pipelines

		for (auto &mesh : meshes) {
			mesh.pipeline = pipelines.meshes;
		}
		//meshes[0].pipeline = pipelines.meshes;
		//meshes[1].pipeline = pipelines.meshes;
		//meshes[2].pipeline = pipelines.meshes;

		//skyboxMesh.pipeline = pipelines.skybox;
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers() {
		// Vertex shader uniform buffer block
		uniformData.meshVS = context.createUniformBuffer(uboVS);
		updateUniformBuffers();
	}

	void updateUniformBuffers() {
		uboVS.projection = camera.matrices.projection;
		uboVS.view = camera.matrices.view;

		//uboVS.model = camera.matrices.skyboxView;

		uboVS.normal = glm::inverseTranspose(uboVS.view * uboVS.model);

		uboVS.lightPos = lightPos;
		uniformData.meshVS.copy(uboVS);

		//uniformData.dynamic.copy(matrixData);
	}

	void prepare() {
		vulkanApp::prepare();
		loadTextures();
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
		if (!prepared) {
			return;
		}
		draw();
	}

	virtual void viewChanged() {
		updateTextOverlay(); //disable this
		updateUniformBuffers();
	}

	virtual void getOverlayText(vkx::TextOverlay *textOverlay)
	{
		textOverlay->addText("camera stats:", 5.0f, 70.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("rotation(q) w: " + std::to_string(camera.rotation.w), 5.0f, 90.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("rotation(q) x: " + std::to_string(camera.rotation.x), 5.0f, 110.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("rotation(q) y: " + std::to_string(camera.rotation.y), 5.0f, 130.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("rotation(q) z: " + std::to_string(camera.rotation.z), 5.0f, 150.0f, vkx::TextOverlay::alignLeft);

		textOverlay->addText("pos x: " + std::to_string(camera.translation.x), 5.0f, 170.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("pos y: " + std::to_string(camera.translation.y), 5.0f, 190.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("pos z: " + std::to_string(camera.translation.z), 5.0f, 210.0f, vkx::TextOverlay::alignLeft);
	}

};



VulkanExample *vulkanExample;

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	VulkanExample* example = new VulkanExample();
	example->run();
	delete(example);
	return 0;
}