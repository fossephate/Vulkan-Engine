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
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR
};

inline size_t alignedSize(size_t align, size_t sz) {
	return ((sz + align - 1) / align)*align;
}






class VulkanExample : public vkx::vulkanApp {

public:



	vk::PipelineVertexInputStateCreateInfo inputState;
	std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
	std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

	std::vector<vkx::Mesh> meshes;
	std::vector<vkx::Model> models;

	vkx::Mesh skyboxMesh;

	struct {
		vkx::UniformData sceneVS;
		vkx::UniformData matrixVS;
		vkx::UniformData materialVS;
	} uniformData;

	struct {
		glm::mat4 model;// not really needed
		glm::mat4 view;
		glm::mat4 projection;
		
		glm::mat4 normal;

		glm::vec4 lightPos;
	} uboScene;


	struct matrixNode {
		glm::mat4 model;
	};
	std::vector<matrixNode> matrices;



	struct materialNode {
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		glm::vec4 emissive;
		float opacity;
	};

	std::vector<materialNode> materials;

	// Shader properites for a material
	// Will be passed to the shaders using push constant
	struct SceneMaterialProperites {
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		float opacity;
	};

	// Stores info on the materials used in the scene
	struct SceneMaterial {
		std::string name;
		// Material properties
		SceneMaterialProperites properties;
		// The example only uses a diffuse channel
		vkx::Texture diffuse;
		// The material's descriptor contains the material descriptors
		vk::DescriptorSet descriptorSet;
		// Pointer to the pipeline used by this material
		vk::Pipeline *pipeline;
	};
	


	unsigned int alignedMatrixSize;
	unsigned int alignedMaterialSize;

	float globalP = 0.0f;




	//struct {
	//	vkx::Texture skybox;
	//} textures;

	struct {
		vkx::Texture colorMap;
		vkx::Texture floor;
	} textures;

	struct {
		vk::Pipeline meshes;
		vk::Pipeline skybox;
	} pipelines;

	vk::PipelineLayout pipelineLayout;
	//vk::DescriptorSet descriptorSet;
	//vk::DescriptorSetLayout descriptorSetLayout;

	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
	std::vector<vk::DescriptorSet> descriptorSets;
	std::vector<vk::DescriptorPool> descriptorPools;

	//descriptorSetLayouts.resize(2);
	//descriptorSets.resize(2);
	//descriptorPools.resize(2);

	//vk::DescriptorSetLayout descriptorSetLayouts[2];
	//vk::DescriptorSet descriptorSets[2];
	//vk::DescriptorPool descriptorPools[2];

	//vk::DescriptorSetLayout descriptorSetLayout;
	//vk::DescriptorSet descriptorSet;
	


	//glm::vec4 lightPos = glm::vec4(2.0f, 2.0f, 5.0f, 0.0f);
	glm::vec4 lightPos = glm::vec4(1.0f, 2.0f, 0.0f, 0.0f);

	VulkanExample() : vkx::vulkanApp(ENABLE_VALIDATION) {
		size.width = 1280;
		size.height = 720;


		camera.setTranslation({ 0.0f, 1.0f, 5.0f });

		matrices.resize(3);
		materials.resize(3);

		//materials[0].test = 0.0f;
		//materials[1].test = 1.0f;

		//matrices[0].model = glm::translate(glm::mat4(), glm::vec3(-5.0f, 0.0f, 0.0f));
		//matrices[1].model = glm::translate(glm::mat4(), glm::vec3(-5.0f, 0.0f, 0.0f));

		unsigned int alignment = (uint32_t)context.deviceProperties.limits.minUniformBufferOffsetAlignment;
		alignedMatrixSize = (unsigned int)(alignedSize(alignment, sizeof(matrixNode)));

		//unsigned int alignment = (uint32_t)context.deviceProperties.limits.minUniformBufferOffsetAlignment;
		alignedMaterialSize = (unsigned int)(alignedSize(alignment, sizeof(materialNode)));

		//camera.matrices.projection = glm::perspectiveRH(glm::radians(60.0f), (float)size.width / (float)size.height, 0.0001f, 256.0f);

		title = "Vulkan Demo Scene";
	}

	~VulkanExample() {
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class


		// todo: fix this all up

		// destroy pipelines
		device.destroyPipeline(pipelines.meshes);
		//device.destroyPipeline(pipelines.skybox);

		device.destroyPipelineLayout(pipelineLayout);
		//device.destroyDescriptorSetLayout(descriptorSetLayout);

		uniformData.sceneVS.destroy();

		for (auto &mesh : meshes) {
			device.destroyBuffer(mesh.meshBuffer.vertices.buffer);
			device.freeMemory(mesh.meshBuffer.vertices.memory);

			device.destroyBuffer(mesh.meshBuffer.indices.buffer);
			device.freeMemory(mesh.meshBuffer.indices.memory);
		}

		//textures.skybox.destroy();

	}

	void loadTextures() {
		textures.colorMap = textureLoader->loadCubemap(getAssetPath() + "textures/cubemap_vulkan.ktx", vk::Format::eR8G8B8A8Unorm);
		//textures.skybox = textureLoader->loadCubemap(getAssetPath() + "textures/cubemap_vulkan.ktx", vk::Format::eR8G8B8A8Unorm);
	}

	void updateDrawCommandBuffer(const vk::CommandBuffer &cmdBuffer) {
		cmdBuffer.setViewport(0, vkx::viewport(size));
		cmdBuffer.setScissor(0, vkx::rect2D(size));


		//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
		//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[0], nullptr);

		//https://github.com/nvpro-samples/gl_vk_threaded_cadscene/blob/master/doc/vulkan_uniforms.md


		// left:
		//vk::Viewport viewport = vkx::viewport((float)size.width / 3, (float)size.height, 0.0f, 1.0f);
		//cmdBuffer.setViewport(0, viewport);
		// center
		//viewport.x += viewport.width;
		//cmdBuffer.setViewport(0, viewport);

		//uboVS.model = glm::translate(uboVS.model, glm::vec3(0.0f, 5.0f, 0.0f));

		

		//meshes[1].model = glm::translate(glm::mat4(), glm::vec3(0.0f, 10.0f, 0.0f));

		//meshes[1].model = glm::translate(glm::mat4(), glm::vec3(5.0f, 5.0f, 0.0f));

		//uboMatrixData.model = meshes[1].model;
		//updateUniformBuffers();

		
		//uboVS.model = meshes[1].model;

		//meshes[1].orientation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		//int i = 1;

		meshes[0].matrixIndex = 0;
		meshes[1].matrixIndex = 1;

		globalP += 0.005f;


		meshes[1].setTranslation(glm::vec3(sin(globalP), 1.0f, 0.0f));

		//matrices[0].model = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
		//matrices[1].model = glm::translate(glm::mat4(), glm::vec3(sin(globalP), 1.0f, 0.0f));
		
		//updateDescriptorSets();
		//updateUniformBuffers();

		for (int i = 0; i < meshes.size(); ++i) {
			matrices[i].model = meshes[i].transfMatrix;// change to use index // todo
		}



		updateUniformBuffers();





		for (auto &mesh : meshes) {

			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mesh.pipeline);
			cmdBuffer.bindVertexBuffers(mesh.vertexBufferBinding, mesh.meshBuffer.vertices.buffer, vk::DeviceSize());
			cmdBuffer.bindIndexBuffer(mesh.meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);


			// move this outside loop
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[0], nullptr);


			uint32_t offset = mesh.matrixIndex * alignedMatrixSize;
			//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, 1, &descriptorSets[1], 1, &offset);


			//uint32_t offset2 = mesh.matrixIndex * alignedMaterialSize;
			//uint32_t offset2 = mesh.materialIndex * alignedMatrixSize;// change?
			//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, 1, &descriptorSets[2], 1, &offset2);

			//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[0], nullptr);
			//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[1], nullptr);



			cmdBuffer.drawIndexed(mesh.meshBuffer.indexCount, 1, 0, 0, 0);


			//mesh.drawIndexed(cmdBuffer);
			//i += 1;
		}

		//uboScene.model = glm::mat4();

	}

	void prepareVertices() {

		struct Vertex {
			float pos[3];
			float normal[3];
			float uv[2];
			float color[3];
		};

		// re-usable? meshloader class// definitely not reusable// important
		//vkx::MeshLoader* loader = new vkx::MeshLoader();

		//loader->load(getAssetPath() + "models/xyplane.dae");
		//vkx::Mesh planeMesh = loader->createMeshFromBuffers(context, vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);


		//loader = new vkx::MeshLoader();

		//loader->load(getAssetPath() + "models/vulkanscenemodels.dae");
		//vkx::Mesh otherMesh1 = loader->createMeshFromBuffers(context, vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		vkx::Mesh planeMesh(context);
		planeMesh.load(getAssetPath() + "models/plane2.dae");
		planeMesh.createBuffers(vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);


		//vkx::Mesh otherMesh1(context);
		//////otherMesh1.load(getAssetPath() + "models/vulkanscenemodels.dae");
		//otherMesh1.load(getAssetPath() + "models/vulkanscenemodels.dae");
		//otherMesh1.createBuffers(vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		vkx::Mesh otherMesh1(context);
		otherMesh1.load(getAssetPath() + "models/vulkanscenemodels.dae");
		//otherMesh1.load(getAssetPath() + "models/rock01.dae");
		otherMesh1.createBuffers(vertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		vkx::Mesh otherMesh2(context);
		otherMesh2.load(getAssetPath() + "models/torus.obj");
		otherMesh2.createBuffers(vertexLayout, 0.02f, VERTEX_BUFFER_BIND_ID);

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
		meshes.push_back(otherMesh2);


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
		

		std::vector<vk::DescriptorPoolSize> poolSizes1 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),// static data
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1),// will eventually be material data
		};

		vk::DescriptorPoolCreateInfo descriptorPool1Info =
			vkx::descriptorPoolCreateInfo(poolSizes1.size(), poolSizes1.data(), 2);


		vk::DescriptorPool descPool1 = device.createDescriptorPool(descriptorPool1Info);
		descriptorPools.push_back(descPool1);





		std::vector<vk::DescriptorPoolSize> poolSizes2 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),// non-static data
		};

		vk::DescriptorPoolCreateInfo descriptorPool2Info =
			vkx::descriptorPoolCreateInfo(poolSizes2.size(), poolSizes2.data(), 1);

		vk::DescriptorPool descPool2 = device.createDescriptorPool(descriptorPool2Info);
		descriptorPools.push_back(descPool2);




		std::vector<vk::DescriptorPoolSize> poolSizes3 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),// will eventually be material data
		};

		vk::DescriptorPoolCreateInfo descriptorPool3Info =
			vkx::descriptorPoolCreateInfo(poolSizes3.size(), poolSizes3.data(), 1);


		vk::DescriptorPool descPool3 = device.createDescriptorPool(descriptorPool3Info);
		descriptorPools.push_back(descPool3);


		

	}

	void setupDescriptorSetLayout() {


		// descriptor set layout 0

		std::vector<vk::DescriptorSetLayoutBinding> setLayout0Bindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 1

			// Binding 1 : Fragment shader color map image sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1)// binding 1
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout0 =
			vkx::descriptorSetLayoutCreateInfo(setLayout0Bindings.data(), setLayout0Bindings.size());

		vk::DescriptorSetLayout setLayout0 = device.createDescriptorSetLayout(descriptorLayout0);

		descriptorSetLayouts.push_back(setLayout0);



		// descriptor set layout 1

		std::vector<vk::DescriptorSetLayoutBinding> setLayout1Bindings =
		{
			// Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout1 =
			vkx::descriptorSetLayoutCreateInfo(setLayout1Bindings.data(), setLayout1Bindings.size());

		vk::DescriptorSetLayout setLayout1 = device.createDescriptorSetLayout(descriptorLayout1);

		descriptorSetLayouts.push_back(setLayout1);





		// descriptor set layout 3

		std::vector<vk::DescriptorSetLayoutBinding> setLayout2Bindings =
		{
			// Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout2 =
			vkx::descriptorSetLayoutCreateInfo(setLayout2Bindings.data(), setLayout2Bindings.size());

		vk::DescriptorSetLayout setLayout2 = device.createDescriptorSetLayout(descriptorLayout2);

		descriptorSetLayouts.push_back(setLayout2);






		// use all descriptor set layouts


		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkx::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());

		pipelineLayout = device.createPipelineLayout(pPipelineLayoutCreateInfo);

	}

	void setupDescriptorSet() {

		/*for (int i = 0; i < descriptorPools.size(); ++i) {
			vk::DescriptorSetAllocateInfo allocInfo =
				vkx::descriptorSetAllocateInfo(descriptorPools[i], &descriptorSetLayouts[i], 1);

			vk::DescriptorSet descSet = device.allocateDescriptorSets(allocInfo)[0];
			descriptorSets.push_back(descSet);
		}*/

		//std::vector<vk::DescriptorSetAllocateInfo> descAllocInfos = {
		//	vkx::descriptorSetAllocateInfo(descriptorPools[0], &descriptorSetLayouts[0], 1),
		//	vkx::descriptorSetAllocateInfo(descriptorPools[1], &descriptorSetLayouts[1], 1),
		//};

		// descriptor set 0

		vk::DescriptorSetAllocateInfo descriptorSetInfo0 =
			vkx::descriptorSetAllocateInfo(descriptorPools[0], &descriptorSetLayouts[0], 1);

		std::vector<vk::DescriptorSet> descSets0 = device.allocateDescriptorSets(descriptorSetInfo0);
		descriptorSets.push_back(descSets0[0]);// descriptor set 1




		// descriptor set 1

		vk::DescriptorSetAllocateInfo descriptorSetInfo1 =
			vkx::descriptorSetAllocateInfo(descriptorPools[1], &descriptorSetLayouts[1], 1);

		std::vector<vk::DescriptorSet> descSets1 = device.allocateDescriptorSets(descriptorSetInfo1);
		descriptorSets.push_back(descSets1[0]);// descriptor set 1



		// descriptor set 2
		vk::DescriptorSetAllocateInfo descriptorSetInfo2 =
			vkx::descriptorSetAllocateInfo(descriptorPools[2], &descriptorSetLayouts[2], 1);

		std::vector<vk::DescriptorSet> descSets2 = device.allocateDescriptorSets(descriptorSetInfo2);
		descriptorSets.push_back(descSets2[0]);// descriptor set 2











		//vk::DescriptorSetAllocateInfo allocInfo1 =
		//	vkx::descriptorSetAllocateInfo(descriptorPools[0], descriptorSetLayouts.data(), 1);

		//vk::DescriptorSet descSet1 = device.allocateDescriptorSets(allocInfo1)[0];
		//descriptorSets.push_back(descSet1);

		//vk::DescriptorSetAllocateInfo allocInfo2 =
		//	vkx::descriptorSetAllocateInfo(descriptorPools[1], descriptorSetLayouts.data(), 1);

		//vk::DescriptorSet descSet2 = device.allocateDescriptorSets(allocInfo2)[0];
		//descriptorSets.push_back(descSet2);


		//vk::DescriptorSetAllocateInfo allocInfo =
			//vkx::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

		//descriptorSet = device.allocateDescriptorSets(allocInfo)[0];

		// Cube map image descriptor
		vk::DescriptorImageInfo texDescriptor =
			vkx::descriptorImageInfo(textures.colorMap.sampler, textures.colorMap.view, vk::ImageLayout::eGeneral);

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets =
		{
			// set 0
			// Binding 0 : Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				descriptorSets[0],// descriptor set 0
				vk::DescriptorType::eUniformBuffer,
				0,// binding 0
				&uniformData.sceneVS.descriptor),
			// Binding 1 : Fragment shader image sampler
			vkx::writeDescriptorSet(
				descriptorSets[0],// descriptor set 0
				vk::DescriptorType::eCombinedImageSampler,
				1,// binding 1
				&texDescriptor),
			
			// set 1
			// vertex shader matrix dynamic buffer
			vkx::writeDescriptorSet(
				descriptorSets[1],// descriptor set 1
				vk::DescriptorType::eUniformBufferDynamic,
				0,// binding 0
				&uniformData.matrixVS.descriptor),


			// set 2
			// vertex shader material dynamic buffer
			vkx::writeDescriptorSet(
				descriptorSets[2],// descriptor set 2
				vk::DescriptorType::eUniformBufferDynamic,
				0,// binding 0
				&uniformData.materialVS.descriptor)

		};

		device.updateDescriptorSets(writeDescriptorSets, nullptr);

		//updateDescriptorSets();
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

	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers() {
		// Vertex shader uniform buffer block
		uniformData.sceneVS = context.createUniformBuffer(uboScene);
		//uniformData.dynamicVS = context.createUniformBuffer(matrices.data(), matrices.size()+1);
		//uniformData.dynamicVS = context.createUniformBuffer(&matrices.data(), matrices.size());
		//uniformData.dynamicVS = context.createUniformBuffer(matrices.data(), matrices.size());
		uniformData.matrixVS = context.createDynamicUniformBuffer(matrices);
		uniformData.materialVS = context.createDynamicUniformBuffer(materials);

		updateUniformBuffers();
	}

	void updateUniformBuffers() {
		uboScene.projection = camera.matrices.projection;
		uboScene.view = camera.matrices.view;

		//uboVS.model = camera.matrices.skyboxView;

		// ?
		//uboScene.normal = glm::inverseTranspose(uboScene.view * uboScene.model);// fix this// important

		//uboScene.normal = glm::inverseTranspose(uboScene.view);

		uboScene.lightPos = lightPos;

		uniformData.sceneVS.copy(uboScene);

		uniformData.matrixVS.copy(matrices);

		// seperate this!// important
		//uniformData.materialVS.copy(materials);
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