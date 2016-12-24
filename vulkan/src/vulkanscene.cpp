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

// todo: rename this: // important!
// vertexLayout has the same name!
std::vector<vkx::VertexLayout> meshVertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR
};


std::vector<vkx::VertexLayout> skinnedMeshVertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4
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
		vkx::UniformData sceneVS;// scene data
		vkx::UniformData matrixVS;// matrix data
		vkx::UniformData materialVS;// material data
	} uniformData;

	struct {
		glm::mat4 model;// not really needed// todo: remove
		glm::mat4 view;
		glm::mat4 projection;
		
		glm::mat4 normal;

		glm::vec3 lightPos;

		glm::vec3 cameraPos;
	} uboScene;


	struct matrixNode {
		glm::mat4 model;
		glm::mat4 normal;
	};
	std::vector<matrixNode> matrixNodes;


	// rename to materialPropertiesNode// or something
	// possibly move
	// or just don't use
	//struct materialNode {
	//	glm::vec4 ambient;
	//	glm::vec4 diffuse;
	//	glm::vec4 specular;
	//	float opacity;
	//};

	std::vector<vkx::materialProperties> materialNodes;
	


	unsigned int alignedMatrixSize;
	unsigned int alignedMaterialSize;

	float globalP = 0.0f;




	struct {
		vkx::Texture colorMap;
		vkx::Texture floor;
	} textures;

	struct {
		vk::Pipeline meshes;
		vk::Pipeline animated;
		vk::Pipeline blending;
		vk::Pipeline wireframe;
		//vk::Pipeline skybox;
	} pipelines;

	vk::PipelineLayout pipelineLayout;




	struct {
		vk::PipelineLayout meshes;
		vk::PipelineLayout animated;
	} pipelineLayouts;



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
	glm::vec3 lightPos = glm::vec3(1.0f, 2.0f, 0.0f);

	VulkanExample() : vkx::vulkanApp(ENABLE_VALIDATION) {
		// todo: pick better numbers
		// or pick based on screen size
		size.width = 1280;
		size.height = 720;


		camera.setTranslation({ 0.0f, 1.0f, 5.0f });

		matrixNodes.resize(100);
		materialNodes.resize(100);

		//materialNodes[0].test = 0.0f;
		//materialNodes[1].test = 1.0f;

		//matrices[0].model = glm::translate(glm::mat4(), glm::vec3(-5.0f, 0.0f, 0.0f));
		//matrixNodes[1].model = glm::translate(glm::mat4(), glm::vec3(-5.0f, 0.0f, 0.0f));


		// todo: move this somewhere else
		// it doesn't need to be here
		unsigned int alignment = (uint32_t)context.deviceProperties.limits.minUniformBufferOffsetAlignment;


		alignedMatrixSize = (unsigned int)(alignedSize(alignment, sizeof(matrixNode)));

		//unsigned int alignment = (uint32_t)context.deviceProperties.limits.minUniformBufferOffsetAlignment;
		alignedMaterialSize = (unsigned int)(alignedSize(alignment, sizeof(vkx::materialProperties)));

		//camera.matrixNodes.projection = glm::perspectiveRH(glm::radians(60.0f), (float)size.width / (float)size.height, 0.0001f, 256.0f);

		title = "Vulkan Demo Scene";
	}

	~VulkanExample() {
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class


		// todo: fix this all up

		// destroy pipelines
		device.destroyPipeline(pipelines.meshes);

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
		//textures.colorMap = textureLoader->loadCubemap(getAssetPath() + "textures/cubemap_vulkan.ktx", vk::Format::eR8G8B8A8Unorm);
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



		// todo: fix this// important
		//models[0].matrixIndex = 0;
		//models[1].matrixIndex = 1;
		//models[2].matrixIndex = 2;
		for (int i = 0; i < models.size(); ++i) {
			models[i].matrixIndex = i;
		}

		globalP += 0.005f;

		//models[1].change();

		//models[1].setTranslation(glm::vec3(3*cos(globalP), 1.0f, 3*sin(globalP)));
		//models[2].setTranslation(glm::vec3(/*2*cos(globalP)+*/2.0f, 3.0f, 0.0f));
		//models[3].setTranslation(glm::vec3(cos(globalP)-2.0f, 2.0f, 0.0f));
		//models[4].setTranslation(glm::vec3(cos(globalP), 3.0f, 0.0f));
		//models[5].setTranslation(glm::vec3(cos(globalP)-2.0f, 4.0f, 0.0f));


		//uboScene.lightPos = glm::vec4(cos(globalP), 4.0f, cos(globalP), 0.0f);
		uboScene.lightPos = glm::vec3(0.0f, 4.0f, 0.0f);


		//matrixNodes[0].model = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
		//matrixNodes[1].model = glm::translate(glm::mat4(), glm::vec3(sin(globalP), 1.0f, 0.0f));
		
		//updateDescriptorSets();
		//updateUniformBuffers();

		//for (int i = 0; i < meshes.size(); ++i) {
		//	matrixNodes[i].model = meshes[i].transfMatrix;// change to use index // todo
		//}


		// todo: fix
		for (auto &model : models) {
			matrixNodes[model.matrixIndex].model = model.transfMatrix;
			//matrixNodes[model.matrixIndex].
			//glm::inverseTranspose(uboScene.view * uboScene.model);
			//matrixNodes[model.matrixIndex].model = glm::mat4();
		}


		//for (auto &mesh : meshes) {
			//matrixNodes[mesh.matrixIndex].model = mesh.transfMatrix;
		//}
		
		// todo: fix this up
		// really inefficient
		// incredibly inefficient
		// don't do this every frame!!!!

		// make sure there is enough room for the material nodes
		if (materialNodes.size() != this->assetManager.loadedMaterials.size()) {
			if (this->assetManager.loadedMaterials.size() == 0) {
				vkx::materialProperties p;
				p.ambient = glm::vec4();
				p.diffuse = glm::vec4();
				p.specular = glm::vec4();
				p.opacity = 1.0f;
				materialNodes[0] = p;
			} else {
				materialNodes.resize(this->assetManager.loadedMaterials.size());
			}
		}

		for (int i = 0; i < this->assetManager.loadedMaterials.size(); ++i) {
			materialNodes[i] = this->assetManager.loadedMaterials[i].properties;
		}


		

		updateUniformBuffers();






		//for (auto &mesh : meshes) {

		//	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mesh.pipeline);
		//	cmdBuffer.bindVertexBuffers(mesh.vertexBufferBinding, mesh.meshBuffer.vertices.buffer, vk::DeviceSize());
		//	cmdBuffer.bindIndexBuffer(mesh.meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

		//	// move this outside loop
		//	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[0], nullptr);

		//	uint32_t offset = mesh.matrixIndex * alignedMatrixSize;
		//	//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
		//	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, 1, &descriptorSets[1], 1, &offset);

		//	//uint32_t offset2 = mesh.matrixIndex * alignedMaterialSize;
		//	//uint32_t offset2 = mesh.materialIndex * alignedMatrixSize;// change?
		//	//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, 1, &descriptorSets[2], 1, &offset2);

		//	cmdBuffer.drawIndexed(mesh.meshBuffer.indexCount, 1, 0, 0, 0);
		//}

		uint32_t lastMaterialIndex = -1;


		// MODELS:

		// bind mesh pipeline
		// don't have to do this for every mesh
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.meshes);

		// for each model
		// model = group of meshes
		// todo: add skinned / animated model support
		for (auto &model : models) {

			// for each of the model's meshes
			for (auto &mesh : model.meshes) {


				// todo: possibly bind multiple descriptor sets at once?

				cmdBuffer.bindVertexBuffers(mesh.vertexBufferBinding, mesh.meshBuffer.vertices.buffer, vk::DeviceSize());
				cmdBuffer.bindIndexBuffer(mesh.meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

				uint32_t setNum;

				// move this outside loop?
				// bind scene descriptor set
				setNum = 0;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, setNum, descriptorSets[setNum], nullptr);


				//uint32_t offset = mesh.matrixIndex * alignedMatrixSize;

				// this will probably squish a scene to one point
				// change to above
				// possibly move outside loop

				uint32_t offset = model.matrixIndex * alignedMatrixSize;
				//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
				// the third param is the set number!
				setNum = 1;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, setNum, 1, &descriptorSets[setNum], 1, &offset);


				// todo:
				// avoid re binding the same material twice in a row
				// ie: if(lastMaterialIndex != mesh.meshBuffer.materialIndex)

				// get offset of mesh's material using meshbuffer's material index
				// and aligned material size

				//if (lastMaterialIndex != mesh.meshBuffer.materialIndex) {
					lastMaterialIndex = mesh.meshBuffer.materialIndex;
					uint32_t offset2 = mesh.meshBuffer.materialIndex * alignedMaterialSize;
					//uint32_t offset2 = 0 * alignedMaterialSize;
					// the third param is the set number!
					setNum = 2;
					cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, setNum, 1, &descriptorSets[setNum], 1, &offset2);
				//}







				////if (lastMaterialIndex != mesh.meshBuffer.materialIndex) {
				//lastMaterialIndex = mesh.meshBuffer.materialIndex;
				////uint32_t offset2 = mesh.meshBuffer.materialIndex * alignedMaterialSize;
				//uint32_t offset2 = 0 * alignedMaterialSize;
				//// the third param is the set number!
				//setNum = 2;
				//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, setNum, 1, &descriptorSets[setNum], 1, &offset2);

				// must make pipeline layout compatible
				setNum = 3;
				vkx::Material m = this->assetManager.loadedMaterials[mesh.meshBuffer.materialIndex];
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, setNum, m.descriptorSet, nullptr);



				// draw:
				cmdBuffer.drawIndexed(mesh.meshBuffer.indexCount, 1, 0, 0, 0);
			}

		}

	}

	void prepareVertices() {

		struct Vertex {
			float pos[3];
			float normal[3];
			float uv[2];
			float color[3];
		};

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
		
		// scene data
		std::vector<vk::DescriptorPoolSize> poolSizes0 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),// mostly static data
		};

		vk::DescriptorPoolCreateInfo descriptorPool0Info =
			vkx::descriptorPoolCreateInfo(poolSizes0.size(), poolSizes0.data(), 1);


		vk::DescriptorPool descPool0 = device.createDescriptorPool(descriptorPool0Info);
		descriptorPools.push_back(descPool0);




		// matrix data
		std::vector<vk::DescriptorPoolSize> poolSizes1 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),// non-static data
		};

		vk::DescriptorPoolCreateInfo descriptorPool1Info =
			vkx::descriptorPoolCreateInfo(poolSizes1.size(), poolSizes1.data(), 1);

		vk::DescriptorPool descPool1 = device.createDescriptorPool(descriptorPool1Info);
		descriptorPools.push_back(descPool1);



		// material data
		std::vector<vk::DescriptorPoolSize> poolSizes2 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),
		};

		vk::DescriptorPoolCreateInfo descriptorPool2Info =
			vkx::descriptorPoolCreateInfo(poolSizes2.size(), poolSizes2.data(), 1);


		vk::DescriptorPool descPool2 = device.createDescriptorPool(descriptorPool2Info);
		descriptorPools.push_back(descPool2);



		// combined image sampler
		std::vector<vk::DescriptorPoolSize> poolSizes3 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 10000),
		};

		vk::DescriptorPoolCreateInfo descriptorPool3Info =
			vkx::descriptorPoolCreateInfo(poolSizes3.size(), poolSizes3.data(), 10000);


		vk::DescriptorPool descPool3 = device.createDescriptorPool(descriptorPool3Info);
		descriptorPools.push_back(descPool3);

		this->assetManager.materialDescriptorPool = &descriptorPools[3];


		

	}

	void setupDescriptorSetLayout() {


		// descriptor set layout 0
		// scene data

		std::vector<vk::DescriptorSetLayoutBinding> setLayout0Bindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 1
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout0 =
			vkx::descriptorSetLayoutCreateInfo(setLayout0Bindings.data(), setLayout0Bindings.size());

		vk::DescriptorSetLayout setLayout0 = device.createDescriptorSetLayout(descriptorLayout0);

		descriptorSetLayouts.push_back(setLayout0);



		// descriptor set layout 1
		// matrix data

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





		// descriptor set layout 2
		// material data

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



		// descriptor set layout 3
		// combined image sampler

		std::vector<vk::DescriptorSetLayoutBinding> setLayout3Bindings =
		{
			// Binding 0 : Fragment shader color map image sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout3 =
			vkx::descriptorSetLayoutCreateInfo(setLayout3Bindings.data(), setLayout3Bindings.size());

		vk::DescriptorSetLayout setLayout3 = device.createDescriptorSetLayout(descriptorLayout3);

		descriptorSetLayouts.push_back(setLayout3);

		this->assetManager.materialDescriptorSetLayout = &descriptorSetLayouts[3];


		// use all descriptor set layouts
		// to form pipeline layout

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
		// scene data
		vk::DescriptorSetAllocateInfo descriptorSetInfo0 =
			vkx::descriptorSetAllocateInfo(descriptorPools[0], &descriptorSetLayouts[0], 1);

		std::vector<vk::DescriptorSet> descSets0 = device.allocateDescriptorSets(descriptorSetInfo0);
		descriptorSets.push_back(descSets0[0]);// descriptor set 0




		// descriptor set 1
		// matrix data
		vk::DescriptorSetAllocateInfo descriptorSetInfo1 =
			vkx::descriptorSetAllocateInfo(descriptorPools[1], &descriptorSetLayouts[1], 1);

		std::vector<vk::DescriptorSet> descSets1 = device.allocateDescriptorSets(descriptorSetInfo1);
		descriptorSets.push_back(descSets1[0]);// descriptor set 1



		// descriptor set 2
		// material data
		vk::DescriptorSetAllocateInfo descriptorSetInfo2 =
			vkx::descriptorSetAllocateInfo(descriptorPools[2], &descriptorSetLayouts[2], 1);

		std::vector<vk::DescriptorSet> descSets2 = device.allocateDescriptorSets(descriptorSetInfo2);
		descriptorSets.push_back(descSets2[0]);// descriptor set 2



		// descriptor set 3
		// image sampler
		//vk::DescriptorSetAllocateInfo descriptorSetInfo3 =
		//	vkx::descriptorSetAllocateInfo(descriptorPools[3], &descriptorSetLayouts[3], 1);

		//std::vector<vk::DescriptorSet> descSets3 = device.allocateDescriptorSets(descriptorSetInfo3);
		//descriptorSets.push_back(descSets3[0]);// descriptor set 3








		// Cube map image descriptor
		//vk::DescriptorImageInfo texDescriptor =
		//	vkx::descriptorImageInfo(textures.colorMap.sampler, textures.colorMap.view, vk::ImageLayout::eGeneral);

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
			//vkx::writeDescriptorSet(
			//	descriptorSets[0],// descriptor set 0
			//	vk::DescriptorType::eCombinedImageSampler,
			//	1,// binding 1
			//	&texDescriptor),
			
			// set 1
			// vertex shader matrix dynamic buffer
			vkx::writeDescriptorSet(
				descriptorSets[1],// descriptor set 1
				vk::DescriptorType::eUniformBufferDynamic,
				0,// binding 0
				&uniformData.matrixVS.descriptor),


			// set 2
			// fragment shader material dynamic buffer
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
		// todo:
		// understand this:

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


		// Alpha blended pipeline
		// transparency
		rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
		blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcColor;
		blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcColor;
		pipelines.blending = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo)[0];


		// Wire frame rendering pipeline
		rasterizationState.cullMode = vk::CullModeFlagBits::eBack;
		blendAttachmentState.blendEnable = VK_FALSE;
		rasterizationState.polygonMode = vk::PolygonMode::eLine;
		rasterizationState.lineWidth = 1.0f;
		pipelines.wireframe = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo)[0];













		// Assign pipelines
		
		// todo:
		//for (auto &mesh : meshes) {
		//	mesh.pipeline = pipelines.meshes;
		//}

		for (auto &model : models) {
			model.pipeline = pipelines.meshes;
		}

	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers() {
		// Vertex shader uniform buffer block
		uniformData.sceneVS = context.createUniformBuffer(uboScene);
		uniformData.matrixVS = context.createDynamicUniformBuffer(matrixNodes);
		uniformData.materialVS = context.createDynamicUniformBuffer(materialNodes);

		updateUniformBuffers();
	}

	void updateUniformBuffers() {
		uboScene.projection = camera.matrices.projection;
		uboScene.view = camera.matrices.view;
		uboScene.cameraPos = camera.translation;

		//uboVS.model = camera.matrixNodes.skyboxView;

		// ?
		//uboScene.normal = glm::inverseTranspose(uboScene.view * uboScene.model);// fix this// important

		//uboScene.normal = glm::inverseTranspose(uboScene.view);

		//uboScene.lightPos = lightPos;

		uniformData.sceneVS.copy(uboScene);

		uniformData.matrixVS.copy(matrixNodes);

		// seperate this!// important
		uniformData.materialVS.copy(materialNodes);
	}


	void start() {


		vkx::Model planeModel(context, assetManager);
		planeModel.load(getAssetPath() + "models/plane2.dae");
		planeModel.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);


		//vkx::Model otherModel1(context, assetManager);
		//otherModel1.load(getAssetPath() + "models/vulkanscenemodels.dae");
		//otherModel1.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);


		//vkx::Model otherModel1(context, assetManager);
		//otherModel1.load(getAssetPath() + "models/vulkanscenemodels.dae");
		//otherModel1.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//vkx::Model otherModel1(context, assetManager);
		//otherModel1.load(getAssetPath() + "models/sibenik/sibenik.dae");
		//otherModel1.createMeshes(meshVertexLayout, 0.05f, VERTEX_BUFFER_BIND_ID);

		//vkx::Model otherModel2(context, assetManager);
		//otherModel2.load(getAssetPath() + "models/vulkanscenemodels.dae");
		//otherModel2.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//vkx::Model otherModel3(context, assetManager);
		//otherModel3.load(getAssetPath() + "models/myCube.dae");
		//otherModel3.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//vkx::Model otherModel4(context, assetManager);
		//otherModel4.load(getAssetPath() + "models/cube.obj");
		//otherModel4.createMeshes(meshVertexLayout, 0.02f, VERTEX_BUFFER_BIND_ID);

		//vkx::Model otherModel5(context, assetManager);
		//otherModel5.load(getAssetPath() + "models/cube.obj");
		//otherModel5.createMeshes(meshVertexLayout, 0.02f, VERTEX_BUFFER_BIND_ID);



		//meshes.push_back(skyboxMesh);
		//meshes.push_back(planeMesh);
		//meshes.push_back(otherMesh1);

		models.push_back(planeModel);
		//models.push_back(otherModel1);
		//models.push_back(otherModel2);
		//models.push_back(otherModel3);
		//models.push_back(otherModel4);
		//models.push_back(otherModel5);

	}



	void prepare() {
		vulkanApp::prepare();

		// todo: remove this:
		loadTextures();

		
		prepareVertices();

		

		prepareUniformBuffers();

		setupDescriptorSetLayout();
		setupDescriptorPool();
		setupDescriptorSet();

		preparePipelines();

		start();


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

	virtual void getOverlayText(vkx::TextOverlay *textOverlay) {


		textOverlay->addText(title, 5.0f, 5.0f, vkx::TextOverlay::alignLeft);

		std::stringstream ss;
		//ss << std::fixed << std::setprecision(2) << (frameTimer * 1000.0f) << "ms (" << lastFPS << " fps)";
		ss << lastFPS << " FPS";
		textOverlay->addText(ss.str(), 5.0f, 25.0f, vkx::TextOverlay::alignLeft);
		
		ss.str("");
		ss.clear();

		ss << std::fixed << std::setprecision(2) << (frameTimer * 1000.0f) << "ms";
		textOverlay->addText(ss.str(), 5.0f, 45.0f, vkx::TextOverlay::alignLeft);


		ss.str("");
		ss.clear();

		ss << "Frame #: " << frameCounter;
		textOverlay->addText(ss.str(), 5.0f, 65.0f, vkx::TextOverlay::alignLeft);

		ss.str("");
		ss.clear();



		ss << "GPU: ";
		ss << deviceProperties.deviceName;
		textOverlay->addText(ss.str(), 5.0f, 85.0f, vkx::TextOverlay::alignLeft);


		// vertical offset
		float vOffset = 120.0f;

		textOverlay->addText("camera stats:", 5.0f, vOffset, vkx::TextOverlay::alignLeft);
		textOverlay->addText("rotation(q) w: " + std::to_string(camera.rotation.w), 5.0f, vOffset + 20.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("rotation(q) x: " + std::to_string(camera.rotation.x), 5.0f, vOffset + 40.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("rotation(q) y: " + std::to_string(camera.rotation.y), 5.0f, vOffset + 60.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("rotation(q) z: " + std::to_string(camera.rotation.z), 5.0f, vOffset + 80.0f, vkx::TextOverlay::alignLeft);

		textOverlay->addText("pos x: " + std::to_string(camera.translation.x), 5.0f, vOffset + 100.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("pos y: " + std::to_string(camera.translation.y), 5.0f, vOffset + 120.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("pos z: " + std::to_string(camera.translation.z), 5.0f, vOffset + 140.0f, vkx::TextOverlay::alignLeft);
	}

};



VulkanExample *vulkanExample;

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	VulkanExample* example = new VulkanExample();
	example->run();
	delete(example);
	return 0;
}