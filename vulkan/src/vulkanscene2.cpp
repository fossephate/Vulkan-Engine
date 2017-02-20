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
#include "vulkanOffscreenExampleBase.hpp"




// Maximum number of bones per mesh
// Must not be higher than same const in skinning shader
#define MAX_BONES 64
// Maximum number of bones per vertex
#define MAX_BONES_PER_VERTEX 4
// Maximum number of skinned meshes (by 65k uniform limit)
#define MAX_SKINNED_MESHES 10
// Texture properties
#define TEX_DIM 1024





std::vector<vkx::VertexLayout> meshVertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4
};


std::vector<vkx::VertexLayout> skinnedMeshVertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4
};


std::vector<vkx::VertexLayout> deferredVertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL
};



inline size_t alignedSize(size_t align, size_t sz) {
	return ((sz + align - 1) / align)*align;
}



inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from) {
	glm::mat4 to;
	to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
	to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
	to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
	to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;
	return to;
}



// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void* alignedAlloc(size_t size, size_t alignment) {
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else 
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void alignedFree(void* data) {
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else 
	free(data);
#endif
}





class VulkanExample : public vkx::OffscreenExampleBase {

public:


	std::vector<std::shared_ptr<vkx::Mesh>> meshes;
	std::vector<std::shared_ptr<vkx::Model>> models;
	std::vector<std::shared_ptr<vkx::SkinnedMesh>> skinnedMeshes;

	std::vector<std::shared_ptr<vkx::Mesh>> meshesDeferred;
	std::vector<std::shared_ptr<vkx::Model>> modelsDeferred;
	std::vector<std::shared_ptr<vkx::SkinnedMesh>> skinnedMeshesDeferred;

	std::vector<std::shared_ptr<vkx::PhysicsObject>> physicsObjects;

	struct {
		vkx::MeshBuffer example;
		vkx::MeshBuffer quad;
	} meshBuffers;


	struct {
		vkx::CreateBufferResult sceneVS;// scene data
		vkx::CreateBufferResult matrixVS;// matrix data
		vkx::CreateBufferResult materialVS;// material data
		vkx::CreateBufferResult bonesVS;// bone data for all skinned meshes // max of 1000 skinned meshes w/64 bones/mesh
	} uniformData;


	// static scene uniform buffer
	struct {
		//glm::mat4 model;// todo: remove
		glm::mat4 view;
		glm::mat4 projection;

		glm::vec4 lightPos;
		glm::vec4 cameraPos;
		glm::mat4 bones[MAX_BONES*MAX_SKINNED_MESHES];// todo: remove this from here
	} uboScene;

	// todo: fix this
	struct MatrixNode {
		glm::mat4 model;
		glm::mat4 boneIndex;
		glm::mat4 g1;
		glm::mat4 g2;
	};

	std::vector<MatrixNode> matrixNodes;

	// material properties not defined here
	std::vector<vkx::MaterialProperties> materialNodes;


	// bone data uniform buffer
	struct {
		glm::mat4 bones[1];
	} uboBoneData;



	unsigned int alignedMatrixSize;
	unsigned int alignedMaterialSize;

	size_t dynamicAlignment;

	float globalP = 0.0f;

	bool debugDisplay = false;

	//glm::vec3 lightPos = glm::vec3(1.0f, -2.0f, 2.0f);
	glm::vec4 lightPos = glm::vec4(1.0f, -2.0f, 2.0f, 1.0f);

	std::string consoleLog;



	// todo: remove this:
	struct {
		vkx::Texture colorMap;
		vkx::Texture floor;
	} textures;




	//struct {
	//	vk::Pipeline meshes;
	//	vk::Pipeline skinnedMeshes;
	//	vk::Pipeline blending;
	//	vk::Pipeline wireframe;
	//} pipelines;

	//struct {

	//	vk::Pipeline meshes;
	//	vk::Pipeline skinnedMeshes;

	//	vk::Pipeline deferred;
	//	vk::Pipeline offscreen;
	//	vk::Pipeline debug;
	//} pipelinesDeferred;



	//struct {
	//	//vk::PipelineLayout basic;

	//	vk::PipelineLayout deferred;

	//	vk::PipelineLayout offscreen;
	//} pipelineLayouts;



	//std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
	//std::vector<vk::DescriptorSet> descriptorSets;
	//std::vector<vk::DescriptorPool> descriptorPools;


	struct {
		vk::PipelineVertexInputStateCreateInfo inputState;
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vk::PipelineVertexInputStateCreateInfo inputState;
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	} verticesDeferred;




	struct {
		glm::mat4 projection;
		glm::mat4 model;
	} uboVS;

	struct {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;

		//glm::mat4 test;
	} uboOffscreenVS;

	struct Light {
		glm::vec4 position;
		glm::vec4 color;
		float radius;
		float quadraticFalloff;
		float linearFalloff;
		float _pad;
	};

	struct {
		Light lights[50];
		glm::vec4 viewPos;
	} uboFSLights;


	struct {
		vkx::UniformData vsFullScreen;
		vkx::UniformData vsOffscreen;
		vkx::UniformData fsLights;

		vkx::UniformData matrixVS;
	} uniformDataDeferred;


	//vk::DescriptorPool descriptorPoolDeferred;
	//vk::DescriptorSetLayout descriptorSetLayoutDeferred;

	//struct {
	//	vk::DescriptorSet basic;

	//	vk::DescriptorSet offscreen;
	//} descriptorSetsDeferred;


	struct Resources {

		vkx::PipelineLayoutList* pipelineLayouts;
		vkx::PipelineList *pipelines;
		vkx::DescriptorSetLayoutList *descriptorSetLayouts;
		vkx::DescriptorSetList *descriptorSets;
		vkx::DescriptorPoolList *descriptorPools;

	} rscs;





	vk::CommandBuffer offscreenCmdBuffer;


	uint32_t lastMaterialIndex = -1;
	std::string lastMaterialName;

	










	VulkanExample() : /*vkx::vulkanApp*/ vkx::OffscreenExampleBase(ENABLE_VALIDATION)/*, offscreen(context)*/ {
		// todo: pick better numbers
		// or pick based on screen size
		size.width = 1280;
		size.height = 720;



		rscs.pipelineLayouts = new vkx::PipelineLayoutList(device);
		rscs.pipelines = new vkx::PipelineList(device);

		rscs.descriptorPools = new vkx::DescriptorPoolList(device);
		rscs.descriptorSetLayouts = new vkx::DescriptorSetLayoutList(device);
		rscs.descriptorSets = new vkx::DescriptorSetList(device);



		camera.setTranslation({ -0.0f, -16.0f, 3.0f });
		glm::quat initialOrientation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		camera.setRotation(initialOrientation);



		matrixNodes.resize(1000);
		materialNodes.resize(1000);


		// todo: move this somewhere else
		// it doesn't need to be here
		unsigned int alignment = (uint32_t)context.deviceProperties.limits.minUniformBufferOffsetAlignment;
		size_t uboAlignment = context.deviceProperties.limits.minUniformBufferOffsetAlignment;


		//dynamicAlignment = (sizeof(glm::mat4) / uboAlignment) * uboAlignment + ((sizeof(glm::mat4) % uboAlignment) > 0 ? uboAlignment : 0);
		//dynamicAlignment = (sizeof(MatrixNode) / uboAlignment) * uboAlignment + ((sizeof(MatrixNode) % uboAlignment) > 0 ? uboAlignment : 0);


		// todo: fix
		//size_t bufferSize = 100 * dynamicAlignment;
		//MatrixNode2.model = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);
		//modelMatrices = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);
		//modelMatrices = (MatrixNode*)alignedAlloc(bufferSize, dynamicAlignment);


		alignedMatrixSize = (unsigned int)(alignedSize(alignment, sizeof(MatrixNode)));
		alignedMaterialSize = (unsigned int)(alignedSize(alignment, sizeof(vkx::MaterialProperties)));

		//camera.matrixNodes.projection = glm::perspectiveRH(glm::radians(60.0f), (float)size.width / (float)size.height, 0.0001f, 256.0f);

		title = "Vulkan Demo Scene";
	}

	~VulkanExample() {
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class


		// todo: fix this all up


		//offscreen.destroy();

		//// destroy pipelines
		//device.destroyPipeline(pipelines.meshes);
		//device.destroyPipeline(pipelines.skinnedMeshes);


		//device.destroyPipeline(pipelinesDeferred.deferred);
		//device.destroyPipeline(pipelinesDeferred.offscreen);
		//device.destroyPipeline(pipelinesDeferred.debug);



		//// destroy pipeline layouts
		//device.destroyPipelineLayout(pipelineLayouts.basic);

		//device.destroyPipelineLayout(pipelineLayouts.deferred);
		//device.destroyPipelineLayout(pipelineLayouts.offscreen);


		//device.destroyDescriptorSetLayout(descriptorSetLayout);

		// destroy uniform buffers
		uniformData.sceneVS.destroy();

		// destroy offscreen uniform buffers
		uniformDataDeferred.vsOffscreen.destroy();
		uniformDataDeferred.vsFullScreen.destroy();
		uniformDataDeferred.fsLights.destroy();

		// destroy offscreen command buffer
		device.freeCommandBuffers(cmdPool, offscreenCmdBuffer);



		for (auto &mesh : meshes) {
			mesh->destroy();
		}

		for (auto &model : models) {
			model->destroy();
		}


		for (auto &skinnedMesh : skinnedMeshes) {
			skinnedMesh->destroy();
		}

		for (auto &physicsObject : physicsObjects) {
			physicsObject->destroy();
		}

		//textures.colorMap.destroy();

		uniformDataDeferred.vsOffscreen.destroy();
		uniformDataDeferred.vsFullScreen.destroy();
		uniformDataDeferred.fsLights.destroy();

		//// Destroy and free mesh resources 
		//skinnedMesh->meshBuffer.destroy();
		//delete(skinnedMesh->meshLoader);
		//delete(skinnedMesh);
		//textures.skybox.destroy();

	}







	void prepareVertexDescriptions() {


		struct skinnedMeshVertex {

			glm::vec3 pos;
			glm::vec2 uv;
			glm::vec3 color;
			glm::vec3 normal;


			// Max. four bones per vertex
			float boneWeights[4];
			uint32_t boneIDs[4];
		};

		uint32_t s1 = sizeof(skinnedMeshVertex);
		uint32_t s2 = vkx::vertexSize(skinnedMeshVertexLayout);


		/* not deferred */

		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			//vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(skinnedMeshVertex), vk::VertexInputRate::eVertex);
			vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, vkx::vertexSize(skinnedMeshVertexLayout), vk::VertexInputRate::eVertex);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(6);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);
		// Location 1 : (UV) Texture coordinates
		vertices.attributeDescriptions[1] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32Sfloat, sizeof(float) * 3);
		// Location 2 : Color
		vertices.attributeDescriptions[2] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32B32Sfloat, sizeof(float) * 5);
		// Location 3 : Normal
		vertices.attributeDescriptions[3] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat, sizeof(float) * 8);
		// Location 4 : Bone weights
		vertices.attributeDescriptions[4] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 11);
		// Location 5 : Bone IDs
		vertices.attributeDescriptions[5] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 5, vk::Format::eR32G32B32A32Sint, sizeof(float) * 15);



		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();

		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();







		/* deferred */

		// Binding description
		verticesDeferred.bindingDescriptions.resize(1);
		verticesDeferred.bindingDescriptions[0] =
			vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, vkx::vertexSize(deferredVertexLayout), vk::VertexInputRate::eVertex);
			//vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(meshVertex), vk::VertexInputRate::eVertex);

		// Attribute descriptions
		// Describes memory layout and shader positions
		verticesDeferred.attributeDescriptions.resize(4);
		// Location 0 : Position
		verticesDeferred.attributeDescriptions[0] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);
		// Location 1 : (UV) Texture coordinates
		verticesDeferred.attributeDescriptions[1] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32Sfloat, sizeof(float) * 3);
		// Location 2 : Color
		verticesDeferred.attributeDescriptions[2] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32B32Sfloat, sizeof(float) * 5);
		// Location 3 : Normal
		verticesDeferred.attributeDescriptions[3] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat, sizeof(float) * 8);
		//// Location 4 : Bone weights
		//verticesDeferred.attributeDescriptions[4] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 11);
		//// Location 5 : Bone IDs
		//verticesDeferred.attributeDescriptions[5] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 5, vk::Format::eR32G32B32A32Sint, sizeof(float) * 15);



		verticesDeferred.inputState.vertexBindingDescriptionCount = verticesDeferred.bindingDescriptions.size();
		verticesDeferred.inputState.pVertexBindingDescriptions = verticesDeferred.bindingDescriptions.data();

		verticesDeferred.inputState.vertexAttributeDescriptionCount = verticesDeferred.attributeDescriptions.size();
		verticesDeferred.inputState.pVertexAttributeDescriptions = verticesDeferred.attributeDescriptions.data();

	}






	void prepareDescriptorPools() {

		// scene data
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes0 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),// mostly static data
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo0 =
			vkx::descriptorPoolCreateInfo(descriptorPoolSizes0.size(), descriptorPoolSizes0.data(), 1);

		rscs.descriptorPools->add("forward.scene", descriptorPoolCreateInfo0);




		// matrix data
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes1 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),// non-static data
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo1 =
			vkx::descriptorPoolCreateInfo(descriptorPoolSizes1.size(), descriptorPoolSizes1.data(), 1);

		rscs.descriptorPools->add("forward.matrix", descriptorPoolCreateInfo1);





		// material data
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes2 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo2 =
			vkx::descriptorPoolCreateInfo(descriptorPoolSizes2.size(), descriptorPoolSizes2.data(), 1);


		rscs.descriptorPools->add("forward.material", descriptorPoolCreateInfo2);





		//// bone data
		//std::vector<vk::DescriptorPoolSize> descriptorPoolSizes3 =
		//{
		//	vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),// static bone data
		//};

		//vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo3 =
		//	vkx::descriptorPoolCreateInfo(descriptorPoolSizes3.size(), descriptorPoolSizes3.data(), 1);

		//vk::DescriptorPool descriptorPool3 = device.createDescriptorPool(descriptorPoolCreateInfo3);
		//descriptorPools.push_back(descriptorPool3);



		// combined image sampler
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes4 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 10000),
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo4 =
			vkx::descriptorPoolCreateInfo(descriptorPoolSizes4.size(), descriptorPoolSizes4.data(), 10000);

		rscs.descriptorPools->add("forward.textures", descriptorPoolCreateInfo4);
		this->assetManager.materialDescriptorPool = rscs.descriptorPools->getPtr("forward.textures");









		// later:
		// deferred:
		// scene data
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes5 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),// mostly static data
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo5 =
			vkx::descriptorPoolCreateInfo(descriptorPoolSizes5.size(), descriptorPoolSizes5.data(), 1);
		rscs.descriptorPools->add("deferred.scene", descriptorPoolCreateInfo5);


		// matrix data
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes6 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),// non-static data
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo6 =
			vkx::descriptorPoolCreateInfo(descriptorPoolSizes6.size(), descriptorPoolSizes6.data(), 1);
		rscs.descriptorPools->add("deferred.matrix", descriptorPoolCreateInfo6);



		// combined image sampler
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes7 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo7 =
			vkx::descriptorPoolCreateInfo(descriptorPoolSizes7.size(), descriptorPoolSizes7.data(), 1000);
		rscs.descriptorPools->add("deferred.textures", descriptorPoolCreateInfo7);



		std::vector<vk::DescriptorPoolSize> descriptorPoolSizesDeferred =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 8),
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 8)
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfoDeferred =
			vkx::descriptorPoolCreateInfo(descriptorPoolSizesDeferred.size(), descriptorPoolSizesDeferred.data(), 2);

		rscs.descriptorPools->add("deferred.deferred", descriptorPoolCreateInfoDeferred);

	}





	void prepareDescriptorSetLayouts() {


		// descriptor set layout 0
		// scene data
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings0 =
		{
			// Set 0: Binding 0 : Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo0 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings0.data(), descriptorSetLayoutBindings0.size());

		rscs.descriptorSetLayouts->add("forward.scene", descriptorSetLayoutCreateInfo0);



		// descriptor set layout 1
		// matrix data
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings1 =
		{
			// Set 1: Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo1 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings1.data(), descriptorSetLayoutBindings1.size());

		rscs.descriptorSetLayouts->add("forward.matrix", descriptorSetLayoutCreateInfo1);





		// descriptor set layout 2
		// material data
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings2 =
		{
			// Set 2: Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo2 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings2.data(), descriptorSetLayoutBindings2.size());

		rscs.descriptorSetLayouts->add("forward.material", descriptorSetLayoutCreateInfo2);




		// // descriptor set layout 3
		// // bone data
		//std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings3 =
		//{
		//	// Binding 0 : Vertex shader uniform buffer
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBuffer,
		//		vk::ShaderStageFlagBits::eVertex,
		//		0),// binding 0
		//};

		//vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo3 =
		//	vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings3.data(), descriptorSetLayoutBindings3.size());pip

		//vk::DescriptorSetLayout descriptorSetLayout3 = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo3);
		//descriptorSetLayouts.push_back(descriptorSetLayout3);






		// descriptor set layout 4
		// combined image sampler
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings4 =
		{
			// Set 4: Binding 0 : Fragment shader color map image sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),// binding 0
			// todo: add specular / bump map here
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo4 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings4.data(), descriptorSetLayoutBindings4.size());

		rscs.descriptorSetLayouts->add("forward.textures", descriptorSetLayoutCreateInfo4);
		this->assetManager.materialDescriptorSetLayout = rscs.descriptorSetLayouts->getPtr("forward.textures");















		// use all descriptor set layouts
		// to form pipeline layout

		// todo: if possible find a better way to do this
		// index / use ordered map and get ptr? ordered by name so probably wouldn't work as intended

		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

		vk::DescriptorSetLayout descriptorSetLayout0 = rscs.descriptorSetLayouts->get("forward.scene");
		vk::DescriptorSetLayout descriptorSetLayout1 = rscs.descriptorSetLayouts->get("forward.matrix");
		vk::DescriptorSetLayout descriptorSetLayout2 = rscs.descriptorSetLayouts->get("forward.material");
		vk::DescriptorSetLayout descriptorSetLayout3 = rscs.descriptorSetLayouts->get("forward.textures");
		descriptorSetLayouts.push_back(descriptorSetLayout0);
		descriptorSetLayouts.push_back(descriptorSetLayout1);
		descriptorSetLayouts.push_back(descriptorSetLayout2);
		descriptorSetLayouts.push_back(descriptorSetLayout3);


		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkx::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());

		//pipelineLayouts.basic = device.createPipelineLayout(pPipelineLayoutCreateInfo);
		//vk::PipelineLayout t = device.createPipelineLayout(pPipelineLayoutCreateInfo);
		rscs.pipelineLayouts->add("forward.basic", pPipelineLayoutCreateInfo);























		/* DEFERRED / OFFSCREEN PASSES */










		// descriptor set layout 0
		// scene data
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings5 =
		{
			// Set 0: Binding 0 : Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo5 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings5.data(), descriptorSetLayoutBindings5.size());

		rscs.descriptorSetLayouts->add("deferred.scene", descriptorSetLayoutCreateInfo5);






		// descriptor set layout 1
		// matrix data
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings6 =
		{
			// Set 1: Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo6 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings6.data(), descriptorSetLayoutBindings6.size());

		rscs.descriptorSetLayouts->add("deferred.matrix", descriptorSetLayoutCreateInfo6);






		//// descriptor set layout 2
		//// material data
		//std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings2 =
		//{
		//	// Set 2: Binding 0 : Vertex shader dynamic uniform buffer
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBufferDynamic,
		//		vk::ShaderStageFlagBits::eVertex,
		//		0),// binding 0
		//};

		//vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo2 =
		//	vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings2.data(), descriptorSetLayoutBindings2.size());

		//rscs.descriptorSetLayouts->add("deferred.material", descriptorSetLayoutCreateInfo2);




		// descriptor set layout 3
		// combined image sampler
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings7 =
		{
			// Set 4: Binding 0 : Fragment shader color map image sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),// binding 0
				   // todo: add specular / bump map here
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo7 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings7.data(), descriptorSetLayoutBindings7.size());

		rscs.descriptorSetLayouts->add("deferred.textures", descriptorSetLayoutCreateInfo7);
		







		/* deferred ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
		// todo:
		// this set layout could just be set = 5 for the above layout
		// rather than being an entire pipeline layout

		// descriptor set layout for full screen quad // deferred pass

		// Deferred shading layout
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindingsDeferred =
		{
			// Set 0: Binding 0: Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),
			// Set 0: Binding 1: Position texture target / Scene colormap
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1),
			// Set 0: Binding 2: Normals texture target
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				2),
			// Set 0: Binding 3: Albedo texture target
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				3),
			// todo: seperate this, it doesn't need to be updated with the textures
			// Set 0: Binding 4: Fragment shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eFragment,
				4),
		};


		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfoDeferred =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindingsDeferred.data(), descriptorSetLayoutBindingsDeferred.size());

		//descriptorSetLayoutDeferred = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfoDeferred);
		rscs.descriptorSetLayouts->add("deferred.deferred", descriptorSetLayoutCreateInfoDeferred);


























		std::vector<vk::DescriptorSetLayout> descriptorSetLayoutsDeferred;

		vk::DescriptorSetLayout descriptorSetLayout4 = rscs.descriptorSetLayouts->get("deferred.scene");
		vk::DescriptorSetLayout descriptorSetLayout5 = rscs.descriptorSetLayouts->get("deferred.matrix");
		//vk::DescriptorSetLayout descriptorSetLayout6 = rscs.descriptorSetLayouts->get("deferred.material");
		vk::DescriptorSetLayout descriptorSetLayout7 = rscs.descriptorSetLayouts->get("deferred.textures");
		
		vk::DescriptorSetLayout descriptorSetLayout8 = rscs.descriptorSetLayouts->get("deferred.deferred");


		descriptorSetLayoutsDeferred.push_back(descriptorSetLayout4);
		descriptorSetLayoutsDeferred.push_back(descriptorSetLayout5);
		//descriptorSetLayoutsDeferred.push_back(descriptorSetLayout6);
		descriptorSetLayoutsDeferred.push_back(descriptorSetLayout7);
		descriptorSetLayoutsDeferred.push_back(descriptorSetLayout8);
		//descriptorSetLayouts.push_back(descriptorSetLayout7);




		// use all descriptor set layouts
		// to form pipeline layout

		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfoDeferred = vkx::pipelineLayoutCreateInfo(descriptorSetLayoutsDeferred.data(), descriptorSetLayoutsDeferred.size());

		// todo:
		// deferred render pass, I should combine this with the above pipeline layout

		rscs.pipelineLayouts->add("deferred.deferred", pPipelineLayoutCreateInfoDeferred);


		// Offscreen (scene) rendering pipeline layout
		// important! used for offscreen render pass
		rscs.pipelineLayouts->add("deferred.offscreen", pPipelineLayoutCreateInfoDeferred);



	}

	void prepareDescriptorSets() {


		// create descriptor sets with descriptor pools and set layouts


		// descriptor set 0
		// scene data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo0 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("forward.scene"), &rscs.descriptorSetLayouts->get("forward.scene"), 1);
		rscs.descriptorSets->add("forward.scene", descriptorSetAllocateInfo0);

		// descriptor set 1
		// matrix data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo1 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("forward.matrix"), &rscs.descriptorSetLayouts->get("forward.matrix"), 1);
		rscs.descriptorSets->add("forward.matrix", descriptorSetAllocateInfo1);

		// descriptor set 2
		// material data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo2 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("forward.material"), &rscs.descriptorSetLayouts->get("forward.material"), 1);
		rscs.descriptorSets->add("forward.material", descriptorSetAllocateInfo2);


		//// descriptor set 3
		//// bone data
		//vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo3 =
		//	vkx::descriptorSetAllocateInfo(descriptorPools[3], &descriptorSetLayouts[3], 1);

		//std::vector<vk::DescriptorSet> descriptorSets3 = device.allocateDescriptorSets(descriptorSetAllocateInfo3);
		//descriptorSets.push_back(descriptorSets3[0]);// descriptor set 4

		// descriptor set 4
		// image sampler
		//vk::DescriptorSetAllocateInfo descriptorSetInfo4 =
		//	vkx::descriptorSetAllocateInfo(descriptorPool4[4], &descriptorSetLayouts[4], 1);

		//std::vector<vk::DescriptorSet> descriptorSets4 = device.allocateDescriptorSets(descriptorSetInfo4);
		//descriptorSets.push_back(descriptorSets4[0]);// descriptor set 4




		std::vector<vk::WriteDescriptorSet> writeDescriptorSets =
		{
			// Set 0: Binding 0: scene uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("forward.scene"),// descriptor set 0
				vk::DescriptorType::eUniformBuffer,
				0,// binding 0
				&uniformData.sceneVS.descriptor),

			// Set 1: Binding 0: vertex shader matrix dynamic buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("forward.matrix"),// descriptor set 1
				vk::DescriptorType::eUniformBufferDynamic,
				0,// binding 0
				&uniformData.matrixVS.descriptor),

			// Set 2: Binding 0: fragment shader material dynamic buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("forward.material"),// descriptor set 2
				vk::DescriptorType::eUniformBufferDynamic,
				0,// binding 0
				&uniformData.materialVS.descriptor),


			//// Set 4?: Binding 0: static bone data buffer
			//vkx::writeDescriptorSet(
			//	descriptorSets[3],// descriptor set 3
			//	vk::DescriptorType::eUniformBuffer,// static
			//	0,// binding 0
			//	&uniformData.bonesVS.descriptor)

			// set 4 is set later (textures)

		};


		device.updateDescriptorSets(writeDescriptorSets, nullptr);






		// descriptor set 0
		// scene data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo5 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("deferred.scene"), &rscs.descriptorSetLayouts->get("deferred.scene"), 1);
		rscs.descriptorSets->add("deferred.scene", descriptorSetAllocateInfo5);

		// descriptor set 1
		// matrix data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo6 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("deferred.matrix"), &rscs.descriptorSetLayouts->get("deferred.matrix"), 1);
		rscs.descriptorSets->add("deferred.matrix", descriptorSetAllocateInfo6);

		// descriptor set 2
		// textures data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo7 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("deferred.textures"), &rscs.descriptorSetLayouts->get("deferred.textures"), 1);
		rscs.descriptorSets->add("deferred.textures", descriptorSetAllocateInfo7);


		// descriptor set 3
		// offscreen textures data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo8 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("deferred.deferred"), &rscs.descriptorSetLayouts->get("deferred.deferred"), 1);
		rscs.descriptorSets->add("deferred.deferred", descriptorSetAllocateInfo8);



		//// Textured quad descriptor set
		//vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo7 =
		//	vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("deferred.deferred"), &rscs.descriptorSetLayouts->get("deferred.deferred"), 1);

		//// deferred descriptor set = 0, binding = 0
		//rscs.descriptorSets->add("deferred.basic", descriptorSetAllocateInfo7);
		






		// vk::Image descriptor for the offscreen texture targets
		// not vk::ImageLayout::eGeneral, eShaderReadOnlyOptimal is important
		vk::DescriptorImageInfo texDescriptorPosition =
			vkx::descriptorImageInfo(offscreen.framebuffers[0].colors[0].sampler, offscreen.framebuffers[0].colors[0].view, vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::DescriptorImageInfo texDescriptorNormal =
			vkx::descriptorImageInfo(offscreen.framebuffers[0].colors[1].sampler, offscreen.framebuffers[0].colors[1].view, vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::DescriptorImageInfo texDescriptorAlbedo =
			vkx::descriptorImageInfo(offscreen.framebuffers[0].colors[2].sampler, offscreen.framebuffers[0].colors[2].view, vk::ImageLayout::eShaderReadOnlyOptimal);

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets2 =
		{

			//// Set 0: Binding 0: scene uniform buffer
			//vkx::writeDescriptorSet(
			//	rscs.descriptorSets->get("deferred.scene"),
			//	vk::DescriptorType::eUniformBuffer,
			//	0,// binding 0
			//	&uniformData.sceneVS.descriptor),

			//// Set 1: Binding 0: vertex shader matrix dynamic buffer
			//vkx::writeDescriptorSet(
			//	rscs.descriptorSets->get("deferred.matrix"),
			//	vk::DescriptorType::eUniformBufferDynamic,
			//	0,// binding 0
			//	&uniformData.matrixVS.descriptor),


			// set 3: Binding 0 : Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.deferred"),
				vk::DescriptorType::eUniformBuffer,
				0,
				&uniformDataDeferred.vsFullScreen.descriptor),
			// set 3: Binding 1 : Position texture target
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.deferred"),
				vk::DescriptorType::eCombinedImageSampler,
				1,
				&texDescriptorPosition),
			// set 3: Binding 2 : Normals texture target
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.deferred"),
				vk::DescriptorType::eCombinedImageSampler,
				2,
				&texDescriptorNormal),
			// set 3: Binding 3 : Albedo texture target
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.deferred"),
				vk::DescriptorType::eCombinedImageSampler,
				3,
				&texDescriptorAlbedo),
			// set 3: Binding 4 : Fragment shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.deferred"),
				vk::DescriptorType::eUniformBuffer,
				4,
				&uniformDataDeferred.fsLights.descriptor),

		};

		device.updateDescriptorSets(writeDescriptorSets2, nullptr);









		// offscreen descriptor set
		// todo: combine with above

		std::vector<vk::WriteDescriptorSet> offscreenWriteDescriptorSets =
		{
			// Set 0: Binding 0: Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.scene"),
				vk::DescriptorType::eUniformBuffer,
				0,
				&uniformDataDeferred.vsOffscreen.descriptor),


			// Set 1: Binding 0: Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.matrix"),
				vk::DescriptorType::eUniformBufferDynamic,
				0,
				&uniformDataDeferred.matrixVS.descriptor),

			// Set 2: Binding 0: Scene color map
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.textures"),
				vk::DescriptorType::eCombinedImageSampler,
				0,
				&textures.colorMap.descriptor),




		};
		device.updateDescriptorSets(offscreenWriteDescriptorSets, nullptr);




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

		vk::PipelineMultisampleStateCreateInfo multisampleState = vkx::pipelineMultisampleStateCreateInfo(vk::SampleCountFlagBits::e1);// added 1/20/17

		std::vector<vk::DynamicState> dynamicStateEnables = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};
		vk::PipelineDynamicStateCreateInfo dynamicState =
			vkx::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());




		// vk::Pipeline for the meshes (armadillo, bunny, etc.)
		// Load shaders
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;


		vk::GraphicsPipelineCreateInfo pipelineCreateInfo = vkx::pipelineCreateInfo(rscs.pipelineLayouts->get("forward.basic"), renderPass);

		pipelineCreateInfo.pVertexInputState = &vertices.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();



		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/forward/mesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/forward/mesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		
		
		////pipelines.meshes = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		////rscs.pipelines->addGraphicsPipeline("forward.meshes", pipelineCache, pipelineCreateInfo);
		
		
		vk::Pipeline meshPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		//rscs.pipelines->add("forward.meshes", meshPipeline);
		rscs.pipelines->resources["forward.meshes"] = meshPipeline;





		// skinned meshes:
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/forward/skinnedMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/forward/skinnedMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//pipelines.skinnedMeshes = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		//rscs.pipelines->addGraphicsPipeline("forward.skinnedMeshes", pipelineCache, pipelineCreateInfo);
		vk::Pipeline skinnedMeshPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		//rscs.pipelines->add("forward.skinnedMeshes", skinnedMeshPipeline);
		rscs.pipelines->resources["forward.skinnedMeshes"] = skinnedMeshPipeline;




		// vk::Pipeline for the sky box
		//rasterizationState.cullMode = vk::CullModeFlagBits::eFront; // Inverted culling
		//depthStencilState.depthWriteEnable = VK_FALSE; // No depth writes
		//shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/skybox.vert.spv", vk::ShaderStageFlagBits::eVertex);
		//shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/skybox.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//pipelines.skybox = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];


		//// Alpha blended pipeline
		//// transparency
		//rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
		//blendAttachmentState.blendEnable = VK_TRUE;
		//blendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
		//blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcColor;
		//blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcColor;
		//pipelines.blending = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo)[0];


		//// Wire frame rendering pipeline
		//rasterizationState.cullMode = vk::CullModeFlagBits::eBack;
		//blendAttachmentState.blendEnable = VK_FALSE;
		//rasterizationState.polygonMode = vk::PolygonMode::eLine;
		//rasterizationState.lineWidth = 1.0f;
		//pipelines.wireframe = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo)[0];

	}


	void prepareDeferredPipelines() {
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkx::pipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology::eTriangleList);

		vk::PipelineRasterizationStateCreateInfo rasterizationState =
			vkx::pipelineRasterizationStateCreateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise);

		vk::PipelineColorBlendAttachmentState blendAttachmentState =
			vkx::pipelineColorBlendAttachmentState();

		vk::PipelineColorBlendStateCreateInfo colorBlendState =
			vkx::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

		vk::PipelineDepthStencilStateCreateInfo depthStencilState =
			vkx::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, vk::CompareOp::eLessOrEqual);

		vk::PipelineViewportStateCreateInfo viewportState =
			vkx::pipelineViewportStateCreateInfo(1, 1);

		vk::PipelineMultisampleStateCreateInfo multisampleState;

		std::vector<vk::DynamicState> dynamicStateEnables = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};
		vk::PipelineDynamicStateCreateInfo dynamicState;
		dynamicState.dynamicStateCount = dynamicStateEnables.size();
		dynamicState.pDynamicStates = dynamicStateEnables.data();

		// Final fullscreen pass pipeline
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo = vkx::pipelineCreateInfo(rscs.pipelineLayouts->get("deferred.deferred"), renderPass);
		pipelineCreateInfo.pVertexInputState = &verticesDeferred.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();


		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/deferred.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/deferred.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//pipelinesDeferred.deferred = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		//rscs.pipelines->addGraphicsPipeline("forward.skinnedMeshes", pipelineCache, pipelineCreateInfo);
		vk::Pipeline deferredPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->resources["deferred.deferred"] = deferredPipeline;


		// Debug display pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/debug.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/debug.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//pipelinesDeferred.debug = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		vk::Pipeline debugPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->resources["deferred.debug"] = debugPipeline;



		// OFFSCREEN PIPELINES:

		// Offscreen pipeline
		//shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrt.vert.spv", vk::ShaderStageFlagBits::eVertex);
		//shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrt.frag.spv", vk::ShaderStageFlagBits::eFragment);

		// Separate render pass
		pipelineCreateInfo.renderPass = offscreen.renderPass;

		// Separate layout
		pipelineCreateInfo.layout = rscs.pipelineLayouts->get("deferred.offscreen");

		// Blend attachment states required for all color attachments
		// This is important, as color write mask will otherwise be 0x0 and you
		// won't see anything rendered to the attachment
		std::array<vk::PipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
			vkx::pipelineColorBlendAttachmentState(),
			vkx::pipelineColorBlendAttachmentState(),
			vkx::pipelineColorBlendAttachmentState()
		};

		colorBlendState.attachmentCount = blendAttachmentStates.size();
		colorBlendState.pAttachments = blendAttachmentStates.data();

		//pipelinesDeferred.offscreen = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];


		// Offscreen pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrtMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrtMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//pipelinesDeferred.meshes = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		vk::Pipeline deferredMeshPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->resources["deferred.meshes"] = deferredMeshPipeline;

	}





	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers() {
		// Vertex shader uniform buffer block
		uniformData.sceneVS = context.createUniformBuffer(uboScene);
		uniformData.matrixVS = context.createDynamicUniformBuffer(matrixNodes);
		uniformData.materialVS = context.createDynamicUniformBuffer(materialNodes);
		uniformData.bonesVS = context.createUniformBuffer(uboBoneData);

		//uniformData.matrixVS = context.createDynamicUniformBufferManual(modelMatrices, 100);




		//uniformData.matrixVS = context.createDynamicUniformBuffer(matrixNodes);

		updateSceneBuffer();// update scene ubo
		updateMatrixBuffer();// update matrix ubo
		updateMaterialBuffer();// update material ubo
		// todo: update bonedata ubo

	}


	void updateSceneBuffer() {

		camera.updateViewMatrix();

		uboScene.view = camera.matrices.view;
		uboScene.projection = camera.matrices.projection;
		//uboScene.cameraPos = camera.transform.translation;
		uboScene.cameraPos = glm::vec4(camera.transform.translation, 0.0f);
		uniformData.sceneVS.copy(uboScene);
	}

	void updateMatrixBuffer() {
		// todo:
		uniformData.matrixVS.copy(matrixNodes);
		//uniformData.matrixVS.copy(modelMatrices);

		//memcpy(uniformData.matrixVS.mapped, modelMatrices, uniformData.matrixVS.size);

		// not needed bc host coherent flag set
		//// Flush to make changes visible to the host 
		//VkMappedMemoryRange memoryRange = vkTools::initializers::mappedMemoryRange();
		//memoryRange.memory = uniformBuffers.dynamic.memory;
		//memoryRange.size = sizeof(uboDataDynamic);
		//vkFlushMappedMemoryRanges(device, 1, &memoryRange);
	}

	void updateMaterialBuffer() {

		if (materialNodes.size() != this->assetManager.materials.loadedMaterials.size()) {
			if (this->assetManager.materials.loadedMaterials.size() == 0) {
				vkx::MaterialProperties p;
				p.ambient = glm::vec4();
				p.diffuse = glm::vec4();
				p.specular = glm::vec4();
				p.opacity = 1.0f;
				materialNodes[0] = p;
			} else {
				materialNodes.resize(this->assetManager.materials.loadedMaterials.size());
			}
		}



		for (auto &iterator : this->assetManager.materials.loadedMaterials) {
			uint32_t index = iterator.second.index;
			materialNodes[index] = iterator.second.properties;
		}


		// todo: don't update the whole buffer each time
		// use map memory range and flush
		// uniform data must not set local host coherent bit?
		// makes changes visible to host
		uniformData.materialVS.copy(materialNodes);
	}



	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffersDeferred() {
		// Fullscreen vertex shader
		uniformDataDeferred.vsFullScreen = context.createUniformBuffer(uboVS);
		// Deferred vertex shader
		uniformDataDeferred.vsOffscreen = context.createUniformBuffer(uboOffscreenVS);
		// Deferred fragment shader
		uniformDataDeferred.fsLights = context.createUniformBuffer(uboFSLights);

		uniformDataDeferred.matrixVS = context.createDynamicUniformBuffer(matrixNodes);

		// Update
		updateUniformBuffersScreen();
		updateSceneBufferDeferred();
		updateMatrixBufferDeferred();
		updateUniformBufferDeferredLights();
	}

	void updateUniformBuffersScreen() {
		if (debugDisplay) {
			uboVS.projection = glm::ortho(0.0f, 2.0f, 0.0f, 2.0f, -1.0f, 1.0f);
		} else {
			uboVS.projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
		}
		uboVS.model = glm::mat4();
		uniformDataDeferred.vsFullScreen.copy(uboVS);
	}

	void updateSceneBufferDeferred() {
		camera.updateViewMatrix();
		uboOffscreenVS.projection = camera.matrices.projection;
		uboOffscreenVS.view = camera.matrices.view;
		uniformDataDeferred.vsOffscreen.copy(uboOffscreenVS);
	}

	void updateMatrixBufferDeferred() {
		uniformDataDeferred.matrixVS.copy(matrixNodes);
	}

	// Update fragment shader light position uniform block
	void updateUniformBufferDeferredLights() {
		// White light from above
		uboFSLights.lights[0].position = glm::vec4(0.0f, 4.0f*sin(globalP), 3.0f, 0.0f);
		uboFSLights.lights[0].color = glm::vec4(1.5f);
		uboFSLights.lights[0].radius = 15.0f;
		uboFSLights.lights[0].linearFalloff = 0.3f;
		uboFSLights.lights[0].quadraticFalloff = 0.4f;
		// Red light
		uboFSLights.lights[1].position = glm::vec4(2.0f*cos(globalP) - 2.0f, 0.0f, 0.0f, 0.0f);
		uboFSLights.lights[1].color = glm::vec4(1.5f, 0.0f, 0.0f, 0.0f);
		uboFSLights.lights[1].radius = 15.0f;
		uboFSLights.lights[1].linearFalloff = 0.4f;
		uboFSLights.lights[1].quadraticFalloff = 0.3f;
		// Blue light
		uboFSLights.lights[2].position = glm::vec4(2.0f, 1.0f, 0.0f, 0.0f);
		uboFSLights.lights[2].color = glm::vec4(0.0f, 0.0f, 2.5f, 0.0f);
		uboFSLights.lights[2].radius = 10.0f;
		uboFSLights.lights[2].linearFalloff = 0.45f;
		uboFSLights.lights[2].quadraticFalloff = 0.35f;
		// Belt glow
		uboFSLights.lights[3].position = glm::vec4(0.0f, 0.7f, 0.5f, 0.0f);
		uboFSLights.lights[3].color = glm::vec4(2.5f, 2.5f, 0.0f, 0.0f);
		uboFSLights.lights[3].radius = 5.0f;
		uboFSLights.lights[3].linearFalloff = 8.0f;
		uboFSLights.lights[3].quadraticFalloff = 6.0f;
		// Green light
		uboFSLights.lights[4].position = glm::vec4(3.0f, 2.0f, 1.0f, 0.0f);
		uboFSLights.lights[4].color = glm::vec4(0.0f, 1.5f, 0.0f, 0.0f);
		uboFSLights.lights[4].radius = 10.0f;
		uboFSLights.lights[4].linearFalloff = 0.8f;
		uboFSLights.lights[4].quadraticFalloff = 0.6f;

		for (int i = 5; i < 10; ++i) {

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> dis(0, 0.8f);
			float rnd = dis(gen);
			float rnd1 = dis(gen);
			float rnd2 = dis(gen);
			float rnd3 = dis(gen);

			uboFSLights.lights[i].position = glm::vec4(sin(globalP)*i, cos(globalP)*i, 3.0f*sin(globalP+i), 0.0f);
			uboFSLights.lights[i].color = glm::vec4(sin(globalP)+(i*2), cos(globalP)+i, sin(globalP)*i, 0.0f) * glm::vec4(1.5f);
			uboFSLights.lights[i].radius = 15.0f;
			uboFSLights.lights[i].linearFalloff = 0.3f;
			uboFSLights.lights[i].quadraticFalloff = 0.4f;

		}



		// Current view position
		//uboFragmentLights.viewPos = glm::vec4(0.0f, 0.0f, -camera.transform.translation.z, 0.0f);
		//uboFragmentLights.viewPos = glm::vec4(-camera.transform.translation.x, -camera.transform.translation.y, -camera.transform.translation.z, 0.0f);
		uboFSLights.viewPos = glm::vec4(camera.transform.translation, 0.0f) * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);

		uniformDataDeferred.fsLights.copy(uboFSLights);
	}


	void toggleDebugDisplay() {
		debugDisplay = !debugDisplay;
		updateDrawCommandBuffers();
		buildOffscreenCommandBuffer();
		updateUniformBuffersScreen();
	}

	void start() {


		auto planeModel = std::make_shared<vkx::Model>(&context, &assetManager);
		planeModel->load(getAssetPath() + "models/plane.fbx");
		planeModel->createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//auto otherModel2 = std::make_shared<vkx::Model>(&context, &assetManager);
		//otherModel2->load(getAssetPath() + "models/monkey.fbx");
		//otherModel2->createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);



		//meshes.push_back(skyboxMesh);
		//meshes.push_back(planeMesh);
		//meshes.push_back(otherMesh1);

		models.push_back(planeModel);
		//models.push_back(otherModel1);

		for (int i = 0; i < 10; ++i) {

			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/cube.fbx");
			testModel->createMeshes(meshVertexLayout, 0.5f, VERTEX_BUFFER_BIND_ID);

			models.push_back(testModel);
		}



		// important!
		//http://stackoverflow.com/questions/6624819/c-vector-of-objects-vs-vector-of-pointers-to-objects

		//auto skinnedMesh1 = std::make_shared<vkx::SkinnedMesh>(&context, &assetManager);
		//skinnedMesh1->load(getAssetPath() + "models/goblin.dae");
		//skinnedMesh1->createSkinnedMeshBuffer(skinnedMeshVertexLayout, 0.0005f);

		//skinnedMeshes.push_back(skinnedMesh1);

		for (int i = 0; i < 2; ++i) {

			//auto testSkinnedMesh = std::make_shared<vkx::SkinnedMesh>(&context, &assetManager);
			//->load(getAssetPath() + "models/goblin.dae");
			//testSkinnedMesh->createSkinnedMeshBuffer(skinnedMeshVertexLayout, 0.0005f);

			//skinnedMeshes.push_back(testSkinnedMesh);
		}

		auto physicsPlane = std::make_shared<vkx::PhysicsObject>(&physicsManager, models[0]);




		auto physicsBall = std::make_shared<vkx::PhysicsObject>(&physicsManager, models[1]);

		//the ground is a cube of side 100 at position y = -56.
		//the sphere will hit it at y = -6, with center at -5
		{
			btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(20.), btScalar(20.), btScalar(0.1)));

			this->physicsManager.collisionShapes.push_back(groundShape);

			btTransform groundTransform;
			groundTransform.setIdentity();
			groundTransform.setOrigin(btVector3(0, 0, 0));

			btScalar mass(0.);

			//rigidbody is dynamic if and only if mass is non zero, otherwise static
			bool isDynamic = (mass != 0.f);

			btVector3 localInertia(0, 0, 0);
			if (isDynamic) {
				groundShape->calculateLocalInertia(mass, localInertia);
			}

			//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
			btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
			btRigidBody* body = new btRigidBody(rbInfo);

			physicsPlane->rigidBody = body;

			//add the body to the dynamics world
			this->physicsManager.dynamicsWorld->addRigidBody(body);
		}

		{
			//create a dynamic rigidbody

			//btCollisionShape* colShape = new btBoxShape(btVector3(1,1,1));
			btCollisionShape* colShape = new btSphereShape(btScalar(1.));
			this->physicsManager.collisionShapes.push_back(colShape);

			/// Create Dynamic Objects
			btTransform startTransform;
			startTransform.setIdentity();

			btScalar	mass(1.f);

			//rigidbody is dynamic if and only if mass is non zero, otherwise static
			bool isDynamic = (mass != 0.f);

			btVector3 localInertia(0, 0, 0);
			if (isDynamic) {
				colShape->calculateLocalInertia(mass, localInertia);

				startTransform.setOrigin(btVector3(0, 0, 10));

				//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
				btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
				btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
				btRigidBody* body = new btRigidBody(rbInfo);

				physicsBall->rigidBody = body;

				this->physicsManager.dynamicsWorld->addRigidBody(body);

			}
		}

		physicsObjects.push_back(physicsPlane);
		physicsObjects.push_back(physicsBall);










		// deferred
		for (int i = 0; i < 6; ++i) {

			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/cube.fbx");
			testModel->createMeshes(deferredVertexLayout, 0.5f, VERTEX_BUFFER_BIND_ID);

			modelsDeferred.push_back(testModel);
		}







		// after any model loading with materials has occurred, updateMaterialBuffer() must be called
		// *after any material loading
		updateMaterialBuffer();

	}









	void updateWorld() {


		// z-up translations
		if (!keyStates.shift) {
			if (keyStates.w) {
				camera.translateLocal(glm::vec3(0.0f, camera.movementSpeed, 0.0f));
			}
			if (keyStates.s) {
				camera.translateLocal(glm::vec3(0.0f, -camera.movementSpeed, 0.0f));
			}
			if (keyStates.a) {
				camera.translateLocal(glm::vec3(-camera.movementSpeed, 0.0f, 0.0f));
			}
			if (keyStates.d) {
				camera.translateLocal(glm::vec3(camera.movementSpeed, 0.0f, 0.0f));
			}
			if (keyStates.q) {
				camera.translateLocal(glm::vec3(0.0f, 0.0f, -camera.movementSpeed));
			}
			if (keyStates.e) {
				camera.translateLocal(glm::vec3(0.0f, 0.0f, camera.movementSpeed));
			}
		} else {
			if (keyStates.w) {
				camera.translateWorld(glm::vec3(0.0f, camera.movementSpeed, 0.0f));
			}
			if (keyStates.s) {
				camera.translateWorld(glm::vec3(0.0f, -camera.movementSpeed, 0.0f));
			}
			if (keyStates.a) {
				camera.translateWorld(glm::vec3(-camera.movementSpeed, 0.0f, 0.0f));
			}
			if (keyStates.d) {
				camera.translateWorld(glm::vec3(camera.movementSpeed, 0.0f, 0.0f));
			}
			if (keyStates.q) {
				camera.translateWorld(glm::vec3(0.0f, 0.0f, -camera.movementSpeed));
			}
			if (keyStates.e) {
				camera.translateWorld(glm::vec3(0.0f, 0.0f, camera.movementSpeed));
			}
		}



		// z-up rotations
		camera.rotationSpeed = -0.025f;

		if (mouse.leftMouseButton.state) {
			camera.rotateWorldZ(mouse.delta.x*camera.rotationSpeed);
			camera.rotateLocalX(mouse.delta.y*camera.rotationSpeed);

			if (!camera.isFirstPerson) {
				camera.sphericalCoords.theta += mouse.delta.x*camera.rotationSpeed;
				camera.sphericalCoords.phi -= mouse.delta.y*camera.rotationSpeed;
			}


			SDL_SetRelativeMouseMode((SDL_bool)1);
		} else {
			bool isCursorLocked = (bool)SDL_GetRelativeMouseMode();
			if (isCursorLocked) {
				SDL_SetRelativeMouseMode((SDL_bool)0);
				SDL_WarpMouseInWindow(this->SDLWindow, mouse.leftMouseButton.pressedCoords.x, mouse.leftMouseButton.pressedCoords.y);
			}
		}


		camera.rotationSpeed = -0.02f;


		if (keyStates.up_arrow) {
			camera.rotateLocalX(-camera.rotationSpeed);
		}
		if (keyStates.down_arrow) {
			camera.rotateLocalX(camera.rotationSpeed);
		}

		if (!keyStates.shift) {
			if (keyStates.left_arrow) {
				camera.rotateWorldZ(-camera.rotationSpeed);
			}
			if (keyStates.right_arrow) {
				camera.rotateWorldZ(camera.rotationSpeed);
			}
		} else {
			//if (keyStates.left_arrow) {
			//	camera.rotateWorldY(-camera.rotationSpeed);
			//}
			//if (keyStates.right_arrow) {
			//	camera.rotateWorldY(camera.rotationSpeed);
			//}
		}

		if (keyStates.t) {
			camera.isFirstPerson = !camera.isFirstPerson;
		}

		if (keyStates.r) {
			toggleDebugDisplay();
		}

		if (keyStates.space) {
			physicsObjects[1]->rigidBody->activate();
			//physicsObjects[1]->rigidBody->setLinearVelocity(btVector3(0.0f, 0.0f, 12.0f));
			physicsObjects[1]->rigidBody->applyCentralForce(btVector3(0.0f, sin(globalP)*0.1f, 0.05f));
		}

		camera.updateViewMatrix();
		//// todo: definitely remove this from here
		updateUniformBuffersScreen();
		updateSceneBufferDeferred();
		updateMatrixBufferDeferred();
		updateUniformBufferDeferredLights();

		if (!camera.isFirstPerson) {
			camera.followOpts.point = models[1]->transform.translation;
		}


		if (keyStates.i) {
			physicsObjects[1]->rigidBody->activate();
			physicsObjects[1]->rigidBody->applyCentralForce(btVector3(0.0f, 0.1f, 0.0f));
		}
		if (keyStates.k) {
			physicsObjects[1]->rigidBody->activate();
			physicsObjects[1]->rigidBody->applyCentralForce(btVector3(0.0f, -0.1f, 0.0f));
		}
		if (keyStates.j) {
			physicsObjects[1]->rigidBody->activate();
			physicsObjects[1]->rigidBody->applyCentralForce(btVector3(-0.1f, 0.0f, 0.0f));
		}
		if (keyStates.l) {
			physicsObjects[1]->rigidBody->activate();
			physicsObjects[1]->rigidBody->applyCentralForce(btVector3(0.1f, 0.0f, 0.0f));
		}

	}






	void updatePhysics() {
		//this->physicsManager.dynamicsWorld->stepSimulation(1.f / 60.f, 10);
		this->physicsManager.dynamicsWorld->stepSimulation(1.f / (frameTimer*1000.0f), 10);

		for (int i = 0; i < this->physicsObjects.size(); ++i) {
			this->physicsObjects[i]->sync();
		}
	}














	void updateDrawCommandBuffer(const vk::CommandBuffer &cmdBuffer) {
		cmdBuffer.setViewport(0, vkx::viewport(size));
		cmdBuffer.setScissor(0, vkx::rect2D(size));


		//https://github.com/nvpro-samples/gl_vk_threaded_cadscene/blob/master/doc/vulkan_uniforms.md


		// left:
		//vk::Viewport testViewport = vkx::viewport((float)size.width / 3, (float)size.height, 0.0f, 1.0f);
		//cmdBuffer.setViewport(0, testViewport);
		// center
		//viewport.x += viewport.width;
		//cmdBuffer.setViewport(0, viewport);



		// todo: fix this// important
		// stop doing this every frame
		// only when necessary
		// actually not that bad, since it makes pushing to vector easy
		for (int i = 0; i < models.size(); ++i) {
			models[i]->matrixIndex = i;
		}


		// uses matrix indices directly after meshes' indices
		for (int i = 0; i < skinnedMeshes.size(); ++i) {
			skinnedMeshes[i]->matrixIndex = models.size() + i;
		}

		for (int i = 0; i < skinnedMeshes.size(); ++i) {
			skinnedMeshes[i]->boneIndex = i;
			// todo: set 4x4 matrix bone index
		}


		// uses matrix indices directly after skinnedMeshes' indices
		for (int i = 0; i < modelsDeferred.size(); ++i) {
			modelsDeferred[i]->matrixIndex = models.size() + skinnedMeshes.size() + i;
		}


		//for (int i = 0; i < matrixNodes.size(); ++i) {
		//	matrixNodes[i].boneIndex[0][0] = i;
		//}


		globalP += 0.005f;

		for (int i = 2; i < models.size(); ++i) {
			models[i]->setTranslation(glm::vec3((2.0f*i)-(models.size()), 0.0f, sin(globalP*i) + 2.0f));
		}

		//for (int i = 2; i < 50; ++i) {
		//	int off = i*5;
		//	for (int j = 0; j < 5; ++j) {
		//		int n = j + off;
		//		models[n]->setTranslation(glm::vec3((2.0f*i) - (models.size()), 0.0f, sin(globalP*i) + 2.0f));
		//	}
		//}

		//if (keyStates.space) {
		//	auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
		//	testModel->load(getAssetPath() + "models/monkey.fbx");
		//	testModel->createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
		//	testModel->matrixIndex = 0;

		//	models.push_back(testModel);
		//}




		if (skinnedMeshes.size() > 0) {
			//skinnedMeshes[0]->setTranslation(glm::vec3(2.0f, sin(globalP), 1.0f));

			glm::vec3 point = skinnedMeshes[0]->transform.translation;
			skinnedMeshes[0]->setTranslation(glm::vec3(point.x, point.y, 1.0f));
			skinnedMeshes[0]->translateLocal(glm::vec3(0.0f, 0.05f, 0.0f));
			skinnedMeshes[0]->rotateLocalZ(0.014f);
		}



		if (skinnedMeshes.size() > 0) {
			skinnedMeshes[0]->animationSpeed = 1.0f;
			//skinnedMeshes[1]->animationSpeed = 3.5f;
		}


		//uboScene.lightPos = glm::vec3(cos(globalP), 4.0f, cos(globalP));
		uboScene.lightPos = glm::vec4(cos(globalP), 4.0f, cos(globalP), 1.0f);
		//uboScene.lightPos = glm::vec3(1.0f, -2.0f, 4.0/**sin(globalP*10.0f)*/+4.0);


		//matrixNodes[0].model = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
		//matrixNodes[1].model = glm::translate(glm::mat4(), glm::vec3(sin(globalP), 1.0f, 0.0f));






		// todo: fix
		for (auto &model : models) {

			//consoleLog = std::to_string(model.transform.translation.x);
			matrixNodes[model->matrixIndex].model = model->transfMatrix;
			//glm::mat4* modelMat = (glm::mat4*)(((uint64_t)modelMatrices + (model.matrixIndex * dynamicAlignment)));
			//*modelMat = model.transfMatrix;

		}




		// uboBoneData.bones is a large bone data buffer
		// use offset to store bone data for each skinnedMesh
		// basically a manual dynamic buffer
		for (auto &skinnedMesh : skinnedMeshes) {

			matrixNodes[skinnedMesh->matrixIndex].model = skinnedMesh->transfMatrix;
			matrixNodes[skinnedMesh->matrixIndex].boneIndex[0][0] = skinnedMesh->boneIndex;

			// performance optimization needed here:
			skinnedMesh->update(globalP*skinnedMesh->animationSpeed);// update animation / interpolated


			uint32_t boneOffset = skinnedMesh->boneIndex*MAX_BONES;

			for (uint32_t i = 0; i < skinnedMesh->meshLoader->boneData.boneTransforms.size(); ++i) {

				//matrixNodes[skinnedMesh.matrixIndex].bones[i] = aiMatrix4x4ToGlm(&skinnedMesh.boneTransforms[i]);
				//matrixNodes[skinnedMesh.matrixIndex].bones[i] = glm::transpose(glm::make_mat4(&skinnedMesh.boneTransforms[i].a1));
				//std::ofstream log("logfile.txt", std::ios_base::app | std::ios_base::out);
				//log << skinnedMesh.boneTransforms[i].a1 << "\n";

				//uboBoneData.bones[boneOffset + i] = glm::transpose(glm::make_mat4(&skinnedMesh->boneTransforms[i].a1));
				//uboScene.bones[i] = glm::transpose(glm::make_mat4(&skinnedMesh->boneTransforms[i].a1));

				uboScene.bones[boneOffset + i] = glm::transpose(glm::make_mat4(&skinnedMesh->meshLoader->boneData.boneTransforms[i].a1));

			}
		}


		updateSceneBuffer();
		updateMatrixBuffer();
		updateMaterialBuffer();

		//updateTextOverlay();

		//uniformData.bonesVS.copy(uboBoneData);






		//// for each of the model's meshes
		//for (auto &mesh : meshes) {

		//	// bind vertex & index buffers
		//	cmdBuffer.bindVertexBuffers(mesh->vertexBufferBinding, mesh->meshBuffer.vertices.buffer, vk::DeviceSize());
		//	cmdBuffer.bindIndexBuffer(mesh->meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

		//	uint32_t setNum;

		//	// bind scene descriptor set
		//	setNum = 0;
		//	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, descriptorSets[setNum], nullptr);


		//	//uint32_t offset1 = mesh.matrixIndex * alignedMatrixSize;
		//	uint32_t offset1 = mesh->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
		//	//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
		//	// the third param is the set number!
		//	setNum = 1;
		//	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset1);


		//	if (lastMaterialIndex != mesh->meshBuffer.materialIndex) {
		//		lastMaterialIndex = mesh->meshBuffer.materialIndex;
		//		uint32_t offset2 = mesh->meshBuffer.materialIndex * static_cast<uint32_t>(alignedMaterialSize);
		//		// the third param is the set number!
		//		setNum = 2;
		//		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset2);

		//		// must make pipeline layout compatible
		//		setNum = 3;
		//		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, m.descriptorSet, nullptr);
		//	}

		//	// draw:
		//	cmdBuffer.drawIndexed(mesh->meshBuffer.indexCount, 1, 0, 0, 0);
		//}



		//https://github.com/SaschaWillems/Vulkan/tree/master/dynamicuniformbuffer




		// MODELS:

		// bind mesh pipeline
		// don't have to do this for every mesh
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("forward.meshes"));

		// for each model
		// model = group of meshes
		// todo: add skinned / animated model support
		for (auto &model : models) {
			// for each of the model's meshes
			for (auto &mesh : model->meshes) {


				// bind vertex & index buffers
				cmdBuffer.bindVertexBuffers(mesh.vertexBufferBinding, mesh.meshBuffer.vertices.buffer, vk::DeviceSize());
				cmdBuffer.bindIndexBuffer(mesh.meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

				// descriptor set #
				uint32_t setNum;

				// bind scene descriptor set
				setNum = 0;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, rscs.descriptorSets->get("forward.scene"), nullptr);


				uint32_t offset1 = model->matrixIndex * alignedMatrixSize;
				//uint32_t offset1 = model->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
				//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
				// the third param is the set number!
				setNum = 1;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, 1, &rscs.descriptorSets->get("forward.matrix"), 1, &offset1);


				if (lastMaterialName != mesh.meshBuffer.materialName) {

					lastMaterialName = mesh.meshBuffer.materialName;

					vkx::Material m = this->assetManager.materials.get(mesh.meshBuffer.materialName);
					//uint32_t materialIndex = this->assetManager.materials.get(mesh.meshBuffer.materialName).index;

					//uint32_t offset2 = m.index * alignedMaterialSize;
					uint32_t offset2 = m.index * static_cast<uint32_t>(alignedMaterialSize);
					// the third param is the set number!
					setNum = 2;
					cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, 1, &rscs.descriptorSets->get("forward.material"), 1, &offset2);



					//lastMaterialIndex = mesh.meshBuffer.materialIndex;

					// bind texture: // todo: implement a better way to bind textures
					// must make pipeline layout compatible
					
					setNum = 3;
					cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, m.descriptorSet, nullptr);
				}


				// draw:
				cmdBuffer.drawIndexed(mesh.meshBuffer.indexCount, 1, 0, 0, 0);
			}

		}











		// SKINNED MESHES:

		// bind skinned mesh pipeline
		// don't have to do this for every skinned mesh// bind once
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("forward.skinnedMeshes"));

		for (auto &skinnedMesh : skinnedMeshes) {


			// bind vertex & index buffers
			cmdBuffer.bindVertexBuffers(skinnedMesh->vertexBufferBinding, skinnedMesh->meshBuffer.vertices.buffer, vk::DeviceSize());
			cmdBuffer.bindIndexBuffer(skinnedMesh->meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);


			uint32_t setNum;
			//uint32_t offset;

			// bind scene descriptor set
			setNum = 0;
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, rscs.descriptorSets->get("forward.scene"), nullptr);

			//uint32_t offset1 = skinnedMesh->matrixIndex * alignedMatrixSize;
			uint32_t offset1 = skinnedMesh->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
			//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
			// the third param is the set number!
			setNum = 1;
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, 1, &rscs.descriptorSets->get("forward.matrix"), 1, &offset1);



			if (lastMaterialName != skinnedMesh->meshBuffer.materialName) {

				lastMaterialName = skinnedMesh->meshBuffer.materialName;

				vkx::Material m = this->assetManager.materials.get(skinnedMesh->meshBuffer.materialName);
				//uint32_t materialIndex = this->assetManager.materials.get(skinnedMesh->meshBuffer.materialName).index;

				//uint32_t offset2 = m.index * alignedMaterialSize;
				uint32_t offset2 = m.index * static_cast<uint32_t>(alignedMaterialSize);
				// the third param is the set number!
				setNum = 2;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, 1, &rscs.descriptorSets->get("forward.material"), 1, &offset2);


				// bind texture:
				//vkx::Material m = this->assetManager.materials.get(skinnedMesh->meshBuffer.materialName);
				setNum = 3;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, m.descriptorSet, nullptr);
			}


			// draw:
			cmdBuffer.drawIndexed(skinnedMesh->meshBuffer.indexCount, 1, 0, 0, 0);
		}

		






		updateUniformBufferDeferredLights();




		vk::Viewport viewport = vkx::viewport(size);
		cmdBuffer.setViewport(0, viewport);
		cmdBuffer.setScissor(0, vkx::rect2D(size));


		// renders quad
		uint32_t setNum = 3;
		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("deferred.deferred"), setNum, rscs.descriptorSets->get("deferred.deferred"), nullptr);
		if (debugDisplay) {
			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("deferred.debug"));
			cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshBuffers.quad.vertices.buffer, { 0 });
			cmdBuffer.bindIndexBuffer(meshBuffers.quad.indices.buffer, 0, vk::IndexType::eUint32);
			cmdBuffer.drawIndexed(meshBuffers.quad.indexCount, 1, 0, 0, 1);
			// Move viewport to display final composition in lower right corner
			viewport.x = viewport.width * 0.5f;
			viewport.y = viewport.height * 0.5f;
		}

		viewport.x = viewport.width * 0.5f;
		viewport.y = viewport.height * 0.5f;

		cmdBuffer.setViewport(0, viewport);
		// Final composition as full screen quad
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("deferred.deferred"));
		cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshBuffers.quad.vertices.buffer, { 0 });
		cmdBuffer.bindIndexBuffer(meshBuffers.quad.indices.buffer, 0, vk::IndexType::eUint32);
		cmdBuffer.drawIndexed(6, 1, 0, 0, 1);


	}




	// Build command buffer for rendering the scene to the offscreen frame buffer 
	// and blitting it to the different texture targets
	void buildOffscreenCommandBuffer() override {
		// Create separate command buffer for offscreen 
		// rendering
		if (!offscreenCmdBuffer) {
			vk::CommandBufferAllocateInfo cmd = vkx::commandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1);
			offscreenCmdBuffer = device.allocateCommandBuffers(cmd)[0];
		}

		vk::CommandBufferBeginInfo cmdBufInfo;
		cmdBufInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

		// Clear values for all attachments written in the fragment shader
		std::array<vk::ClearValue, 4> clearValues;
		clearValues[0].color = vkx::clearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
		clearValues[1].color = vkx::clearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
		clearValues[2].color = vkx::clearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
		clearValues[3].depthStencil = { 1.0f, 0 };

		vk::RenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.renderPass = offscreen.renderPass;
		renderPassBeginInfo.framebuffer = offscreen.framebuffers[0].framebuffer;
		renderPassBeginInfo.renderArea.extent.width = offscreen.size.x;
		renderPassBeginInfo.renderArea.extent.height = offscreen.size.y;
		renderPassBeginInfo.clearValueCount = clearValues.size();
		renderPassBeginInfo.pClearValues = clearValues.data();

		offscreenCmdBuffer.begin(cmdBufInfo);
		offscreenCmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);


		// start of render pass


		vk::Viewport viewport = vkx::viewport(offscreen.size);
		offscreenCmdBuffer.setViewport(0, viewport);

		vk::Rect2D scissor = vkx::rect2D(offscreen.size);
		offscreenCmdBuffer.setScissor(0, scissor);

		vk::DeviceSize offsets = { 0 };









		



		//offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelinesDeferred.offscreen);


		








		//offscreenCmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshBuffers.example.vertices.buffer, { 0 });
		//offscreenCmdBuffer.bindIndexBuffer(meshBuffers.example.indices.buffer, 0, vk::IndexType::eUint32);
		//offscreenCmdBuffer.drawIndexed(meshBuffers.example.indexCount, 1, 0, 0, 0);






		float t = 0;



		// todo: add matrix indices for deferred models
		// for(int i = 0; i < deferredModels.size(); ++i) {




		// MODELS:

		// bind mesh pipeline
		// don't have to do this for every mesh
		// todo: create pipelinesDefferd.mesh
		offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("deferred.meshes"));

		// for each model
		// model = group of meshes
		// todo: add skinned / animated model support
		for (auto &model : modelsDeferred) {
			// for each of the model's meshes
			for (auto &mesh : model->meshes) {


				// bind vertex & index buffers
				offscreenCmdBuffer.bindVertexBuffers(mesh.vertexBufferBinding, mesh.meshBuffer.vertices.buffer, vk::DeviceSize());
				offscreenCmdBuffer.bindIndexBuffer(mesh.meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

				// descriptor set #
				uint32_t setNum;

				// bind scene descriptor set
				setNum = 0;
				offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("deferred.offscreen"), setNum, rscs.descriptorSets->get("deferred.scene"), nullptr);
				//offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.offscreen, setNum, descriptorSets[setNum], nullptr);


				uint32_t offset1 = model->matrixIndex * alignedMatrixSize;
				//uint32_t offset1 = model->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
				//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
				// the third param is the set number!
				setNum = 1;
				//offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.offscreen, setNum, 1, &descriptorSets[setNum], 1, &offset1);


				if (lastMaterialName != mesh.meshBuffer.materialName) {

					lastMaterialName = mesh.meshBuffer.materialName;

					vkx::Material m = this->assetManager.materials.get(mesh.meshBuffer.materialName);
					//uint32_t materialIndex = this->assetManager.materials.get(mesh.meshBuffer.materialName).index;

					//uint32_t offset2 = m.index * alignedMaterialSize;
					uint32_t offset2 = m.index * static_cast<uint32_t>(alignedMaterialSize);
					// the third param is the set number!
					setNum = 2;
					//offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.offscreen, setNum, 1, &descriptorSets[setNum], 1, &offset2);



					//lastMaterialIndex = mesh.meshBuffer.materialIndex;

					// bind texture: // todo: implement a better way to bind textures
					// must make pipeline layout compatible

					setNum = 3;
					//offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, m.descriptorSet, nullptr);
				}


				// draw:
				offscreenCmdBuffer.drawIndexed(mesh.meshBuffer.indexCount, 1, 0, 0, 0);
			}

		}














		// end render pass

		offscreenCmdBuffer.endRenderPass();
		offscreenCmdBuffer.end();
	}














	void viewChanged() override {
		updateSceneBufferDeferred();
	}






	void loadTextures() {
		//textures.colorMap = textureLoader->loadTexture(
		//	getAssetPath() + "models/armor/colormap.ktx",
		//	vk::Format::eBc3UnormBlock);
		textures.colorMap = textureLoader->loadTexture(
			getAssetPath() + "models/kamen.ktx",
			vk::Format::eBc3UnormBlock);
	}

	void loadMeshes() {
		//meshes.example = loadMesh(getAssetPath() + "models/armor/armor.dae", vertexLayout, 1.0f);

		vkx::MeshLoader loader(&this->context, &this->assetManager);
		//loader.load(getAssetPath() + "models/armor/armor.dae");
		loader.load(getAssetPath() + "models/cube.fbx");
		loader.createMeshBuffer(deferredVertexLayout, 1.0f);
		meshBuffers.example = loader.combinedBuffer;
	}

	void generateQuads() {
		// Setup vertices for multiple screen aligned quads
		// Used for displaying final result and debug 
		struct Vertex {
			float pos[3];
			float uv[2];
			float col[3];
			float normal[3];
		};

		std::vector<Vertex> vertexBuffer;

		float x = 0.0f;
		float y = 0.0f;
		for (uint32_t i = 0; i < 3; i++) {
			// Last component of normal is used for debug display sampler index
			vertexBuffer.push_back({ { x + 1.0f, y + 1.0f, 0.0f },{ 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, (float)i } });
			vertexBuffer.push_back({ { x,      y + 1.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, (float)i } });
			vertexBuffer.push_back({ { x,      y,      0.0f },{ 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, (float)i } });
			vertexBuffer.push_back({ { x + 1.0f, y,      0.0f },{ 1.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, (float)i } });
			x += 1.0f;
			if (x > 1.0f) {
				x = 0.0f;
				y += 1.0f;
			}
		}
		meshBuffers.quad.vertices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);

		// Setup indices
		std::vector<uint32_t> indexBuffer = { 0,1,2, 2,3,0 };
		for (uint32_t i = 0; i < 3; ++i) {
			uint32_t indices[6] = { 0,1,2, 2,3,0 };
			for (auto index : indices) {
				indexBuffer.push_back(i * 4 + index);
			}
		}
		meshBuffers.quad.indexCount = indexBuffer.size();
		meshBuffers.quad.indices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
	}












	void prepare() override {


		offscreen.size = glm::uvec2(TEX_DIM);
		offscreen.colorFormats = std::vector<vk::Format>{ {
				vk::Format::eR16G16B16A16Sfloat,
				vk::Format::eR16G16B16A16Sfloat,
				vk::Format::eR8G8B8A8Unorm
			} };

		//vulkanApp::prepare();
		//offscreen.prepare();

		OffscreenExampleBase::prepare();



		loadTextures();
		generateQuads();
		loadMeshes();


		prepareVertexDescriptions();



		prepareUniformBuffers();
		prepareUniformBuffersDeferred();

		prepareDescriptorSetLayouts();
		prepareDescriptorPools();
		prepareDescriptorSets();

		preparePipelines();
		prepareDeferredPipelines();

		start();


		updateDrawCommandBuffers();
		buildOffscreenCommandBuffer();



		prepared = true;
	}

	void draw() override {
		prepareFrame();
		{
			vk::SubmitInfo submitInfo;
			submitInfo.pWaitDstStageMask = this->submitInfo.pWaitDstStageMask;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &offscreenCmdBuffer;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &semaphores.acquireComplete;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &offscreen.renderComplete;
			queue.submit(submitInfo, VK_NULL_HANDLE);
		}
		drawCurrentCommandBuffer(offscreen.renderComplete);
		submitFrame();
	}

	virtual void render() {
		if (!prepared) {
			return;
		}
		draw();
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


		//ss.str("");
		//ss.clear();

		//ss << "Frame #: " << frameCounter;
		//textOverlay->addText(ss.str(), 5.0f, 65.0f, vkx::TextOverlay::alignLeft);

		ss.str("");
		ss.clear();

		ss << "GPU: ";
		ss << deviceProperties.deviceName;
		textOverlay->addText(ss.str(), 5.0f, 65.0f, vkx::TextOverlay::alignLeft);


		// vertical offset
		float vOffset = 120.0f;
		float spacing = 20.0f;

		textOverlay->addText("pos x: " + std::to_string(camera.transform.translation.x), 5.0f, vOffset + 100.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("pos y: " + std::to_string(camera.transform.translation.y), 5.0f, vOffset + 120.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("pos z: " + std::to_string(camera.transform.translation.z), 5.0f, vOffset + 140.0f, vkx::TextOverlay::alignLeft);
	}

};




int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {

	VulkanExample* example = new VulkanExample();
	example->run();
	delete(example);
	return 0;
}