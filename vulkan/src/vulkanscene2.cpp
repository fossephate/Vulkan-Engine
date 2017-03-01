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

#define PI 3.14159265359





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



std::vector<vkx::VertexLayout> SSAOVertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_TANGENT,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4
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

float rand0t1() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0.0f, 1.0f);
	float rnd = dis(gen);
	return rnd;
}




// todo: move this somewhere else
btConvexHullShape* createConvexHullFromMesh(vkx::MeshLoader *meshLoader) {
	btConvexHullShape *convexHullShape = new btConvexHullShape();
	for (int i = 0; i < meshLoader->m_Entries[0].Indices.size(); ++i) {
		uint32_t index = meshLoader->m_Entries[0].Indices[i];
		glm::vec3 point = meshLoader->m_Entries[0].Vertices[index].m_pos;
		btVector3 p = btVector3(point.x, point.y, point.z);
		convexHullShape->addPoint(p);
	}
	convexHullShape->optimizeConvexHull();
	convexHullShape->initializePolyhedralFeatures();

	return convexHullShape;
};

int completeHack = 0;


// todo: move this somewhere else
btConvexHullShape* createConvexHullFromMeshEntry(vkx::MeshLoader *meshLoader, int index) {
	btConvexHullShape *convexHullShape = new btConvexHullShape();
	for (int i = 0; i < meshLoader->m_Entries[completeHack].Indices.size(); ++i) {
		uint32_t index = meshLoader->m_Entries[completeHack].Indices[i];
		glm::vec3 point = meshLoader->m_Entries[completeHack].Vertices[index].m_pos;
		btVector3 p = btVector3(point.x, point.y, point.z);
		convexHullShape->addPoint(p);
	}
	convexHullShape->optimizeConvexHull();
	convexHullShape->initializePolyhedralFeatures();

	return convexHullShape;
};






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
		glm::mat4 view;
		glm::mat4 projection;

		glm::vec4 lightPos;
		glm::vec4 cameraPos;
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
		glm::mat4 bones[MAX_BONES*MAX_SKINNED_MESHES];
	} uboBoneData;



	unsigned int alignedMatrixSize;
	unsigned int alignedMaterialSize;

	size_t dynamicAlignment;

	float globalP = 0.0f;

	bool debugDisplay = false;
	float fullDeferred = false;

	//glm::vec3 lightPos = glm::vec3(1.0f, -2.0f, 2.0f);
	glm::vec4 lightPos = glm::vec4(1.0f, -2.0f, 2.0f, 1.0f);

	std::string consoleLog;



	// todo: remove this:
	struct {
		vkx::Texture colorMap;
	} textures;


	struct {
		vk::PipelineVertexInputStateCreateInfo inputState;
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	//struct {
	//	vk::PipelineVertexInputStateCreateInfo inputState;
	//	std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
	//	std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	//} meshVertices;

	//struct {
	//	vk::PipelineVertexInputStateCreateInfo inputState;
	//	std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
	//	std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	//} skinnedMeshVertices;




	struct {
		glm::mat4 model;
		glm::mat4 projection;
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
		Light lights[100];
		glm::vec4 viewPos;
	} uboFSLights;


	struct {
		vkx::UniformData vsFullScreen;
		vkx::UniformData vsOffscreen;
		vkx::UniformData fsLights;

		vkx::UniformData matrixVS;
	} uniformDataDeferred;


	struct Resources {

		vkx::PipelineLayoutList* pipelineLayouts;
		vkx::PipelineList *pipelines;
		vkx::DescriptorSetLayoutList *descriptorSetLayouts;
		vkx::DescriptorSetList *descriptorSets;
		vkx::DescriptorPoolList *descriptorPools;

	} rscs;

	struct {
		size_t models = 0;
		size_t skinnedMeshes = 0;
	} lastSizes;

	bool updateDraw = true;
	bool updateOffscreen = true;





	vk::CommandBuffer offscreenCmdBuffer;


	uint32_t lastMaterialIndex = -1;
	std::string lastMaterialName;

	










	VulkanExample() : /*vkx::vulkanApp*/ vkx::OffscreenExampleBase(ENABLE_VALIDATION)/*, offscreen(context)*/ {




		rscs.pipelineLayouts = new vkx::PipelineLayoutList(device);
		rscs.pipelines = new vkx::PipelineList(device);

		rscs.descriptorPools = new vkx::DescriptorPoolList(device);
		rscs.descriptorSetLayouts = new vkx::DescriptorSetLayoutList(device);
		rscs.descriptorSets = new vkx::DescriptorSetList(device);



		camera.setTranslation({ -0.0f, -16.0f, 3.0f });
		glm::quat initialOrientation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		camera.setRotation(initialOrientation);

		// todo: pick better numbers
		// or pick based on screen size
		size.width = 1280;
		size.height = 720;

		camera.setProjection(80.0f, (float)size.width / (float)size.height, 0.01f, 128.0f);



		matrixNodes.resize(1000);
		materialNodes.resize(1000);


		// todo: move this somewhere else
		unsigned int alignment = (uint32_t)context.deviceProperties.limits.minUniformBufferOffsetAlignment;

		alignedMatrixSize = (unsigned int)(alignedSize(alignment, sizeof(MatrixNode)));
		alignedMaterialSize = (unsigned int)(alignedSize(alignment, sizeof(vkx::MaterialProperties)));

		//camera.matrixNodes.projection = glm::perspectiveRH(glm::radians(60.0f), (float)size.width / (float)size.height, 0.0001f, 256.0f);

		title = "Vulkan test";
	}

	~VulkanExample() {
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class


		// todo: fix this all up



		// destroy pipelines



		// destroy pipeline layouts

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


		// skinned mesh vertex layout

		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, vkx::vertexSize(SSAOVertexLayout), vk::VertexInputRate::eVertex);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(7);
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
		// Location 4 : Tangent
		vertices.attributeDescriptions[4] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, vk::Format::eR32G32B32Sfloat, sizeof(float) * 11);
		// Location 5 : Bone weights
		vertices.attributeDescriptions[5] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 14);
		// Location 6 : Bone IDs
		vertices.attributeDescriptions[6] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 5, vk::Format::eR32G32B32A32Sint, sizeof(float) * 18);



		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();

		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();


		{
		// mesh vertex layout

		//// Binding description
		//meshVertices.bindingDescriptions.resize(1);
		//meshVertices.bindingDescriptions[0] =
		//	vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, vkx::vertexSize(skinnedMeshVertexLayout), vk::VertexInputRate::eVertex);

		//// Attribute descriptions
		//// Describes memory layout and shader positions
		//meshVertices.attributeDescriptions.resize(6);
		//// Location 0 : Position
		//meshVertices.attributeDescriptions[0] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);
		//// Location 1 : (UV) Texture coordinates
		//meshVertices.attributeDescriptions[1] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32Sfloat, sizeof(float) * 3);
		//// Location 2 : Color
		//meshVertices.attributeDescriptions[2] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32B32Sfloat, sizeof(float) * 5);
		//// Location 3 : Normal
		//meshVertices.attributeDescriptions[3] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat, sizeof(float) * 8);



		//meshVertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		//meshVertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();

		//meshVertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		//meshVertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();


		//// skinned mesh vertex layout

		//// Binding description
		//skinnedMeshVertices.bindingDescriptions.resize(1);
		//skinnedMeshVertices.bindingDescriptions[0] =
		//	vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, vkx::vertexSize(deferredVertexLayout), vk::VertexInputRate::eVertex);

		//// Attribute descriptions
		//// Describes memory layout and shader positions
		//skinnedMeshVertices.attributeDescriptions.resize(4);
		//// Location 0 : Position
		//skinnedMeshVertices.attributeDescriptions[0] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);
		//// Location 1 : (UV) Texture coordinates
		//skinnedMeshVertices.attributeDescriptions[1] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32Sfloat, sizeof(float) * 3);
		//// Location 2 : Color
		//skinnedMeshVertices.attributeDescriptions[2] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32B32Sfloat, sizeof(float) * 5);
		//// Location 3 : Normal
		//skinnedMeshVertices.attributeDescriptions[3] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat, sizeof(float) * 8);
		//// Location 4 : Bone weights
		//skinnedMeshVertices.attributeDescriptions[4] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 11);
		//// Location 5 : Bone IDs
		//skinnedMeshVertices.attributeDescriptions[5] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 5, vk::Format::eR32G32B32A32Sint, sizeof(float) * 15);



		//skinnedMeshVertices.inputState.vertexBindingDescriptionCount = verticesDeferred.bindingDescriptions.size();
		//skinnedMeshVertices.inputState.pVertexBindingDescriptions = verticesDeferred.bindingDescriptions.data();

		//skinnedMeshVertices.inputState.vertexAttributeDescriptionCount = verticesDeferred.attributeDescriptions.size();
		//skinnedMeshVertices.inputState.pVertexAttributeDescriptions = verticesDeferred.attributeDescriptions.data();
		}



	}






	void prepareDescriptorPools() {

		// scene data
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes0 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),// mostly static data
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





		// combined image sampler
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes4 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 10000),
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo4 =
			vkx::descriptorPoolCreateInfo(descriptorPoolSizes4.size(), descriptorPoolSizes4.data(), 10000);
		rscs.descriptorPools->add("forward.textures", descriptorPoolCreateInfo4);

		this->assetManager.materialDescriptorPool = rscs.descriptorPools->getPtr("forward.textures");








		/* DEFERRED */


		// later:
		// deferred:
		// scene data
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes5 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),// mostly static data
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

			// Set 0: Binding 1: Vertex shader uniform buffer// bone data
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				1),// binding 1
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






		// descriptor set layout 3
		// combined image sampler
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings3 =
		{
			// Set 3: Binding 0 : Fragment shader color map image sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),// binding 0

			// Set 3: Binding 1: Fragment shader specular
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1),// binding 1
			// Set 3: Binding 2: Fragment shader bump
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				2),// binding 2
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo3 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings3.data(), descriptorSetLayoutBindings3.size());
		rscs.descriptorSetLayouts->add("forward.textures", descriptorSetLayoutCreateInfo3);
		this->assetManager.materialDescriptorSetLayout = rscs.descriptorSetLayouts->getPtr("forward.textures");




		//// descriptor set layout 4
		//// bone data
		//std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings4 =
		//{
		//	// Set 4: Binding 0 : Vertex shader uniform buffer
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBuffer,
		//		vk::ShaderStageFlagBits::eVertex,
		//		0),// binding 0
		//};

		//vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo4 =
		//	vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings4.data(), descriptorSetLayoutBindings4.size());
		//rscs.descriptorSetLayouts->add("forward.bones", descriptorSetLayoutCreateInfo4);









		// use all descriptor set layouts
		// to form pipeline layout

		// todo: if possible find a better way to do this
		// index / use ordered map and get ptr? ordered by name so probably wouldn't work as intended

		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

		vk::DescriptorSetLayout descriptorSetLayout0 = rscs.descriptorSetLayouts->get("forward.scene");
		vk::DescriptorSetLayout descriptorSetLayout1 = rscs.descriptorSetLayouts->get("forward.matrix");
		vk::DescriptorSetLayout descriptorSetLayout2 = rscs.descriptorSetLayouts->get("forward.material");
		vk::DescriptorSetLayout descriptorSetLayout3 = rscs.descriptorSetLayouts->get("forward.textures");
		//vk::DescriptorSetLayout descriptorSetLayout4 = rscs.descriptorSetLayouts->get("forward.bones");
		descriptorSetLayouts.push_back(descriptorSetLayout0);
		descriptorSetLayouts.push_back(descriptorSetLayout1);
		descriptorSetLayouts.push_back(descriptorSetLayout2);
		descriptorSetLayouts.push_back(descriptorSetLayout3);
		//descriptorSetLayouts.push_back(descriptorSetLayout4);

		// create pipelineLayout from descriptorSetLayouts
		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkx::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());
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

			// Set 0: Binding 1: Vertex shader uniform buffer// bone data
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				1),// binding 1
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
			// Set 2: Binding 0: Fragment shader color map image sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),// binding 0
			// Set 2: Binding 1: Fragment shader specular
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1),// binding 1
			// Set 2: Binding 2: Fragment shader bump
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				2),// binding 2
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo7 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings7.data(), descriptorSetLayoutBindings7.size());

		rscs.descriptorSetLayouts->add("deferred.textures", descriptorSetLayoutCreateInfo7);
		







		/* deferred ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
		rscs.descriptorSetLayouts->add("deferred.deferred", descriptorSetLayoutCreateInfoDeferred);














		std::vector<vk::DescriptorSetLayout> descriptorSetLayoutsDeferred;

		vk::DescriptorSetLayout descriptorSetLayout5 = rscs.descriptorSetLayouts->get("deferred.scene");
		vk::DescriptorSetLayout descriptorSetLayout6 = rscs.descriptorSetLayouts->get("deferred.matrix");
		//vk::DescriptorSetLayout descriptorSetLayout6 = rscs.descriptorSetLayouts->get("deferred.material");
		vk::DescriptorSetLayout descriptorSetLayout7 = rscs.descriptorSetLayouts->get("deferred.textures");
		vk::DescriptorSetLayout descriptorSetLayout8 = rscs.descriptorSetLayouts->get("deferred.deferred");


		//descriptorSetLayoutsDeferred.push_back(descriptorSetLayout4);
		descriptorSetLayoutsDeferred.push_back(descriptorSetLayout5);
		descriptorSetLayoutsDeferred.push_back(descriptorSetLayout6);
		descriptorSetLayoutsDeferred.push_back(descriptorSetLayout7);
		descriptorSetLayoutsDeferred.push_back(descriptorSetLayout8);
		//descriptorSetLayouts.push_back(descriptorSetLayout7);




		// use all descriptor set layouts
		// to form pipeline layout

		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfoDeferred = vkx::pipelineLayoutCreateInfo(descriptorSetLayoutsDeferred.data(), descriptorSetLayoutsDeferred.size());

		// todo:
		// deferred render pass, I should combine this with the above pipeline layout?

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


		//// descriptor set 0
		//// bone data
		//vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo3 =
		//	vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("forward.bones"), &rscs.descriptorSetLayouts->get("forward.bones"), 1);
		//rscs.descriptorSets->add("forward.bones", descriptorSetAllocateInfo3);

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

			// Set 0: Binding 1: bones uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("forward.scene"),// descriptor set 0
				vk::DescriptorType::eUniformBuffer,
				1,// binding 1
				&uniformData.bonesVS.descriptor),

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

			//// Set 4: Binding 0: bones uniform buffer
			//vkx::writeDescriptorSet(
			//	rscs.descriptorSets->get("forward.bones"),// descriptor set 4
			//	vk::DescriptorType::eUniformBuffer,
			//	0,// binding 0
			//	&uniformData.bonesVS.descriptor),


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
		// or dont

		std::vector<vk::WriteDescriptorSet> offscreenWriteDescriptorSets =
		{
			// Set 0: Binding 0: Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.scene"),
				vk::DescriptorType::eUniformBuffer,
				0,
				&uniformDataDeferred.vsOffscreen.descriptor),

			// Set 0: Binding 1: bones uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.scene"),// descriptor set 0
				vk::DescriptorType::eUniformBuffer,
				1,// binding 1
				&uniformData.bonesVS.descriptor),// bind to forward descriptor since it's the same


			// Set 1: Binding 0: Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred.matrix"),
				vk::DescriptorType::eUniformBufferDynamic,
				0,
				&uniformData.matrixVS.descriptor),// bind to forward descriptor since it's the same

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
		
		
		vk::Pipeline meshPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("forward.meshes", meshPipeline);



		// skinned meshes:
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/forward/skinnedMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/forward/skinnedMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline skinnedMeshPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("forward.skinnedMeshes", skinnedMeshPipeline);




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
		//vk::Pipeline meshPipelineBlending = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo)[0];
		//rscs.pipelines->resources["forward.meshes.blending"] = meshPipelineBlending;
		//blendAttachmentState.blendEnable = VK_TRUE;


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

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;

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


		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/deferred.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/deferred.frag.spv", vk::ShaderStageFlagBits::eFragment);

		// fullscreen quad
		vk::Pipeline deferredPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("deferred.deferred", deferredPipeline);


		// Alpha blended pipeline
		// transparency
		rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
		blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eOne;
		blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		vk::Pipeline deferredPipelineBlending = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo)[0];
		rscs.pipelines->add("deferred.deferred.blending", deferredPipelineBlending);
		blendAttachmentState.blendEnable = VK_FALSE;



		// Debug display pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/debug.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/debug.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline debugPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("deferred.debug", debugPipeline);



		// OFFSCREEN PIPELINES:

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

		// Offscreen pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrtMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrtMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline deferredMeshPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("deferred.meshes", deferredMeshPipeline);

		// Offscreen pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrtSkinnedMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrtSkinnedMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline deferredSkinnedMeshPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("deferred.skinnedMeshes", deferredSkinnedMeshPipeline);

		// Offscreen pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/mrtMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/mrtMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline deferredMeshSSAOPipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("deferred.meshes.ssao", deferredMeshSSAOPipeline);


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
		updateBoneBuffer();
		// todo: update bonedata ubo

	}


	void updateSceneBuffer() {

		//camera.updateViewMatrix();

		uboScene.view = camera.matrices.view;
		uboScene.projection = camera.matrices.projection;
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


	void updateBoneBuffer() {
		uniformData.bonesVS.copy(uboBoneData);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffersDeferred() {
		// Fullscreen quad vertex shader
		uniformDataDeferred.vsFullScreen = context.createUniformBuffer(uboVS);

		// Offscreen vertex shader
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
		//camera.updateViewMatrix();
		uboOffscreenVS.projection = camera.matrices.projection;
		uboOffscreenVS.view = camera.matrices.view;
		uniformDataDeferred.vsOffscreen.copy(uboOffscreenVS);
	}

	void updateMatrixBufferDeferred() {
		uniformDataDeferred.matrixVS.copy(matrixNodes);
	}

	// Update fragment shader light position uniform block
	void updateUniformBufferDeferredLights() {

		int n = 0;
		int w = 8;
		int h = 6;
		int sw = 15;
		int sh = 15;

		for (int i = 0; i < w; ++i) {

			for (int j = 0; j < h; ++j) {

				//float rnd = rand0t1();

				float xOffset = (w*sw) / 2.0;
				float yOffset = (h*sh) / 2.0;

				float x = (i * sw) - xOffset;
				float y = (j * sh) - yOffset;
				float z = (10.0f) + (sin((0.5*globalP) + n)*2.0f);

				uboFSLights.lights[n].position = glm::vec4(x, y, z, 0.0f);
				uboFSLights.lights[n].color = glm::vec4((i * 2) - 3.0f, i, j, 0.0f) * glm::vec4(2.5f);
				uboFSLights.lights[n].radius = 15.0f;
				uboFSLights.lights[n].linearFalloff = 0.3f;
				uboFSLights.lights[n].quadraticFalloff = 0.4f;

				// increment counter
				n++;
			}

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

		// add plane model
		auto planeModel = std::make_shared<vkx::Model>(&context, &assetManager);
		planeModel->load(getAssetPath() + "models/plane.fbx");
		planeModel->createMeshes(SSAOVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
		models.push_back(planeModel);

		
		

		for (int i = 0; i < 10; ++i) {
			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/cube.fbx");
			testModel->createMeshes(SSAOVertexLayout, 0.5f, VERTEX_BUFFER_BIND_ID);

			models.push_back(testModel);
		}


		for (int i = 0; i < 1; ++i) {

			auto testSkinnedMesh = std::make_shared<vkx::SkinnedMesh>(&context, &assetManager);
			testSkinnedMesh->load(getAssetPath() + "models/goblin.dae");
			testSkinnedMesh->createSkinnedMeshBuffer(SSAOVertexLayout, 0.0005f);

			skinnedMeshes.push_back(testSkinnedMesh);
		}

		auto physicsPlane = std::make_shared<vkx::PhysicsObject>(&physicsManager, planeModel);
		btCollisionShape* boxShape = new btBoxShape(btVector3(btScalar(200.), btScalar(200.), btScalar(0.1)));
		physicsPlane->createRigidBody(boxShape, 0.0f);
		//btTransform t;
		//t.setOrigin(btVector3(0., 0., 0.));
		//physicsPlane->rigidBody->setWorldTransform(t);
		physicsObjects.push_back(physicsPlane);


		//auto physicsBall = std::make_shared<vkx::PhysicsObject>(&physicsManager, models[1]);
		//btCollisionShape* sphereShape = new btSphereShape(btScalar(1.));
		//physicsBall->createRigidBody(sphereShape, 1.0f);
		//btTransform t2;
		//t2.setOrigin(btVector3(0., 0., 10.));
		//physicsBall->rigidBody->setWorldTransform(t2);
		//physicsObjects.push_back(physicsBall);






		// deferred

		if (false) {
			auto sponzaModel = std::make_shared<vkx::Model>(&context, &assetManager);
			sponzaModel->load(getAssetPath() + "models/sponza.dae");
			sponzaModel->createMeshes(SSAOVertexLayout, 0.5f, VERTEX_BUFFER_BIND_ID);
			sponzaModel->rotateWorldX(PI / 2.0);
			modelsDeferred.push_back(sponzaModel);
		}


		//auto boxModel = std::make_shared<vkx::Model>(&context, &assetManager);
		//boxModel->load(getAssetPath() + "models/boxVhacd.fbx");
		//boxModel->createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
		////boxModel->rotateWorldX(PI / 2.0);
		//modelsDeferred.push_back(boxModel);
		////printf(std::to_string(boxModel->meshLoader->m_Entries.size()).c_str());

		//for (int i = 0; i < 12; ++i) {
		//	auto physicsPart = std::make_shared<vkx::PhysicsObject>(&physicsManager, nullptr);
		//	btCollisionShape *wallShape1 = createConvexHullFromMeshEntry(boxModel->meshLoader, i);
		//	physicsPart->createRigidBody(wallShape1, 0.0f);

		//	completeHack += 1;
		//	//btTransform t1;
		//	//t1.setOrigin(btVector3(0., 0., 4.));
		//	//physicsPart->rigidBody->setWorldTransform(t1);
		//	//physicsObjects.push_back(physicsWall1);
		//}

		//// add plane model
		//auto planeModel2 = std::make_shared<vkx::Model>(&context, &assetManager);
		//planeModel2->load(getAssetPath() + "models/plane.fbx");
		//planeModel2->createMeshes(deferredVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
		//modelsDeferred.push_back(planeModel2);

		
		for (int i = 0; i < 6; ++i) {
			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/monkey.fbx");
			testModel->createMeshes(SSAOVertexLayout, 0.5f, VERTEX_BUFFER_BIND_ID);

			modelsDeferred.push_back(testModel);
		}


		for (int i = 0; i < 1; ++i) {

			auto testSkinnedMesh = std::make_shared<vkx::SkinnedMesh>(&context, &assetManager);
			testSkinnedMesh->load(getAssetPath() + "models/goblin.dae");
			testSkinnedMesh->createSkinnedMeshBuffer(SSAOVertexLayout, 0.0005f);
			// todo: figure out why there must be atleast one deferred skinned mesh here
			// inorder to not cause problems
			skinnedMeshesDeferred.push_back(testSkinnedMesh);
		}





		auto wallModel1 = std::make_shared<vkx::Model>(&context, &assetManager);
		wallModel1->load(getAssetPath() + "models/plane.fbx");
		wallModel1->createMeshes(SSAOVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
		modelsDeferred.push_back(wallModel1);



		//auto physicsWall1 = std::make_shared<vkx::PhysicsObject>(&physicsManager, wallModel1);
		//btCollisionShape *wallShape1 = new btBoxShape(btVector3(btScalar(1.0), btScalar(1.), btScalar(0.1)));
		//physicsWall1->createRigidBody(wallShape1, 0.0f);
		//btTransform t1;
		//t1.setOrigin(btVector3(0., 0., 4.));
		//physicsWall1->rigidBody->setWorldTransform(t1);
		//physicsObjects.push_back(physicsWall1);








		// after any loading with materials has occurred, updateMaterialBuffer() must be called
		// to update texture descriptor sets and sync
		updateMaterialBuffer();

	}









	void updateWorld() {

		updateDraw = false;
		updateOffscreen = false;


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

		//if (keyStates.onKeyDown(&keyStates.t)) {
		if (keyStates.t) {
			camera.isFirstPerson = !camera.isFirstPerson;
		}

		if (keyStates.onKeyDown(&keyStates.r)) {
		//if (keyStates.r) {
			toggleDebugDisplay();
		}

		//if (keyStates.onKeyDown(&keyStates.p)) {
		if (keyStates.p) {
			updateDraw = true;
			updateOffscreen = true;
		}

		if (keyStates.onKeyDown(&keyStates.y)) {
		//if (keyStates.y) {
			fullDeferred = !fullDeferred;
			updateDraw = true;
			updateOffscreen = true;

		}





		if (keyStates.space) {
			//physicsObjects[1]->rigidBody->activate();
			//physicsObjects[1]->rigidBody->setLinearVelocity(btVector3(0.0f, 0.0f, 12.0f));
			//physicsObjects[1]->rigidBody->applyCentralForce(btVector3(0.0f, sin(globalP)*0.1f, 0.05f));


			//auto testModel0 = std::make_shared<vkx::Model>(&context, &assetManager);
			//testModel0->load(getAssetPath() + "models/myCube.dae");
			//testModel0->createMeshes(skinnedMeshVertexLayout, 0.5f, VERTEX_BUFFER_BIND_ID);
			//modelsDeferred.push_back(testModel0);

			//auto physicsBall0 = std::make_shared<vkx::PhysicsObject>(&physicsManager, testModel0);
			//btCollisionShape* sphereShape0 = new btSphereShape(btScalar(1.));
			//btCollisionShape* boxShape0 = new btBoxShape(btVector3(btScalar(1.), btScalar(1.), btScalar(1.)));

			//physicsBall0->createRigidBody(boxShape0, 1.0f);
			//btTransform t0;
			//t0.setOrigin(btVector3(0., 0., 10.));
			////physicsBall0->rigidBody->setWorldTransform(t0);
			//physicsBall0->rigidBody->getMotionState()->setWorldTransform(t0);
			//physicsObjects.push_back(physicsBall0);
		
		
		










			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/monkey.fbx");
			testModel->createMeshes(SSAOVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
			modelsDeferred.push_back(testModel);



			auto physicsBall = std::make_shared<vkx::PhysicsObject>(&physicsManager, testModel);
			btConvexHullShape *convexHullShape = createConvexHullFromMesh(testModel->meshLoader);
			physicsBall->createRigidBody(convexHullShape, 1.0f);
			physicsBall->rigidBody->activate();
			physicsBall->rigidBody->translate(btVector3(0., 0., 10.));
			physicsObjects.push_back(physicsBall);



			updateOffscreen = true;



		}


		if (keyStates.b) {

			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/myCube.dae");
			testModel->createMeshes(SSAOVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
			modelsDeferred.push_back(testModel);



			auto physicsBall = std::make_shared<vkx::PhysicsObject>(&physicsManager, testModel);
			btConvexHullShape *convexHullShape = createConvexHullFromMesh(testModel->meshLoader);
			physicsBall->createRigidBody(convexHullShape, 1.0f);
			physicsBall->rigidBody->activate();
			physicsBall->rigidBody->translate(btVector3(0., 0., 10.));
			physicsObjects.push_back(physicsBall);




			updateOffscreen = true;
		}




		if (keyStates.m) {
			if (modelsDeferred.size() > 4) {
				modelsDeferred[modelsDeferred.size() - 1]->destroy();
				modelsDeferred.pop_back();
				//updateDraw = true;
				updateOffscreen = true;
			}
		}

		if (keyStates.n) {
			if (physicsObjects.size() > 4) {
				physicsObjects[physicsObjects.size() - 1]->destroy();
				physicsObjects.pop_back();
				updateOffscreen = true;
			}
		}


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



		if (keyStates.minus) {
			settings.fpsCap -= 0.2f;
		} else if (keyStates.equals) {
			settings.fpsCap += 0.2f;
		}





		camera.updateViewMatrix();



























		if (skinnedMeshes.size() > 0) {
			skinnedMeshes[0]->animationSpeed = 1.0f;
			glm::vec3 point = skinnedMeshes[0]->transform.translation;
			skinnedMeshes[0]->setTranslation(glm::vec3(point.x, point.y, 1.0f));
			skinnedMeshes[0]->translateLocal(glm::vec3(0.0f, -0.024f, 0.0f));
			skinnedMeshes[0]->rotateLocalZ(0.014f);
		}

		if (skinnedMeshes.size() > 1) {
			skinnedMeshes[1]->animationSpeed = 5.0f;
			glm::vec3 point = skinnedMeshes[1]->transform.translation;
			skinnedMeshes[1]->setTranslation(glm::vec3(point.x, point.y, 1.0f));
			skinnedMeshes[1]->translateLocal(glm::vec3(0.0f, -0.024f, 0.0f));
			skinnedMeshes[1]->rotateLocalZ(-0.014f);
		}


		//if (modelsDeferred.size() > 3) {
		//	for (int i = 0; i < modelsDeferred.size(); ++i) {
		//		modelsDeferred[i]->setTranslation(glm::vec3(2 * i, 0.0f, sin(4*globalP*i)));
		//	}
		//}

		//for (int i = 0; i < modelsDeferred.size(); ++i) {
		//	modelsDeferred[i]->setTranslation(glm::vec3((2.0f*i) - (modelsDeferred.size()), 0.0f, sin(globalP*i) + 2.0f));
		//}

		if (skinnedMeshesDeferred.size() > 0) {
			skinnedMeshesDeferred[0]->animationSpeed = 1.0f;
			glm::vec3 point = skinnedMeshesDeferred[0]->transform.translation;
			skinnedMeshesDeferred[0]->setTranslation(glm::vec3(point.x, point.y, 1.0f));
			skinnedMeshesDeferred[0]->translateLocal(glm::vec3(0.0f, -0.024f, 0.0f));
			skinnedMeshesDeferred[0]->rotateLocalZ(0.014f);
		}

		if (skinnedMeshesDeferred.size() > 1) {
			skinnedMeshesDeferred[1]->animationSpeed = 5.0f;
			glm::vec3 point = skinnedMeshesDeferred[1]->transform.translation;
			skinnedMeshesDeferred[1]->setTranslation(glm::vec3(point.x, point.y, 1.0f));
			skinnedMeshesDeferred[1]->translateLocal(glm::vec3(0.0f, -0.024f, 0.0f));
			skinnedMeshesDeferred[1]->rotateLocalZ(0.024f);
		}


		uboScene.lightPos = glm::vec4(cos(globalP), 4.0f, cos(globalP)+3.0f, 1.0f);



		globalP += 0.005f;



		{
			// todo: fix this// important
			for (int i = 0; i < models.size(); ++i) {
				models[i]->matrixIndex = i;
			}


			// uses matrix indices directly after meshes' indices
			for (int i = 0; i < skinnedMeshes.size(); ++i) {
				skinnedMeshes[i]->matrixIndex = models.size() + i;
			}


			// uses matrix indices directly after skinnedMeshes' indices
			for (int i = 0; i < modelsDeferred.size(); ++i) {
				// added a buffer of 10 so that there is time to update command buffers
				modelsDeferred[i]->matrixIndex = models.size() + skinnedMeshes.size() + i + 10;// todo: figure this out
			}

			// uses matrix indices directly after modelsDeferred' indices
			for (int i = 0; i < skinnedMeshesDeferred.size(); ++i) {
				// added a buffer of 10 so that there is time to update command buffers
				skinnedMeshesDeferred[i]->matrixIndex = models.size() + skinnedMeshes.size() + modelsDeferred.size() + i + 10;// todo: figure this out
			}

			// set bone indices
			for (int i = 0; i < skinnedMeshes.size(); ++i) {
				skinnedMeshes[i]->boneIndex = i;// todo: fix
			}

			for (int i = 0; i < skinnedMeshesDeferred.size(); ++i) {
				skinnedMeshesDeferred[i]->boneIndex = skinnedMeshes.size() + i;
			}
		}





















		/* UPDATE BUFFERS */

		// todo: fix
		for (auto &model : models) {
			matrixNodes[model->matrixIndex].model = model->transfMatrix;
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
				uboBoneData.bones[boneOffset + i] = glm::transpose(glm::make_mat4(&skinnedMesh->meshLoader->boneData.boneTransforms[i].a1));
			}
		}



		for (auto &model : modelsDeferred) {
			matrixNodes[model->matrixIndex].model = model->transfMatrix;
		}


		// uboBoneData.bones is a large bone data buffer
		// use offset to store bone data for each skinnedMesh
		// basically a manual dynamic buffer
		for (auto &skinnedMesh : skinnedMeshesDeferred) {

			matrixNodes[skinnedMesh->matrixIndex].model = skinnedMesh->transfMatrix;
			matrixNodes[skinnedMesh->matrixIndex].boneIndex[0][0] = skinnedMesh->boneIndex;

			// performance optimization needed here:
			skinnedMesh->update(globalP*skinnedMesh->animationSpeed);// update animation / interpolated


			uint32_t boneOffset = skinnedMesh->boneIndex*MAX_BONES;

			for (uint32_t i = 0; i < skinnedMesh->meshLoader->boneData.boneTransforms.size(); ++i) {
				uboBoneData.bones[boneOffset + i] = glm::transpose(glm::make_mat4(&skinnedMesh->meshLoader->boneData.boneTransforms[i].a1));
			}
		}


		updateSceneBuffer();
		updateMatrixBuffer();
		updateMaterialBuffer();
		updateBoneBuffer();



		updateUniformBuffersScreen();
		updateSceneBufferDeferred();
		updateMatrixBufferDeferred();
		updateUniformBufferDeferredLights();


		if (models.size() != lastSizes.models) {
			updateDraw = true;
		}
		if (skinnedMeshes.size() != lastSizes.skinnedMeshes) {
			updateDraw = true;
		}

		lastSizes.models = models.size();
		lastSizes.skinnedMeshes = skinnedMeshes.size();














	}






	void updatePhysics() {
		//this->physicsManager.dynamicsWorld->stepSimulation(1.f / 60.f, 10);
		//this->physicsManager.dynamicsWorld->stepSimulation(1.f / (frameTimer*1000.0f), 10);
		
		
		// get current time
		auto tNow = std::chrono::high_resolution_clock::now();
		// the time since the last tick
		auto tDuration = std::chrono::duration<double, std::milli>(tNow - this->physicsManager.tLastTimeStep);
		
		// todo: fix
		this->physicsManager.dynamicsWorld->stepSimulation(tDuration.count()/1000.0, 10);





		for (int i = 0; i < this->physicsObjects.size(); ++i) {
			this->physicsObjects[i]->sync();
		}
	}








	void updateCommandBuffers() {

		if (updateDraw) {
			// record / update draw command buffers
			updateDrawCommandBuffers();
		}

		if (updateOffscreen) {
			buildOffscreenCommandBuffer();
		}

	}





	void updateDrawCommandBuffer(const vk::CommandBuffer &cmdBuffer) {







		cmdBuffer.setViewport(0, vkx::viewport(size));
		cmdBuffer.setScissor(0, vkx::rect2D(size));





		

		/*for (int i = 2; i < 5; ++i) {
			models[i]->setTranslation(glm::vec3((2.0f*i)-(models.size()), 0.0f, sin(globalP*i) + 2.0f));
		}*/

		// todo: move this
		/*for (int i = 2; i < 5; ++i) {
			modelsDeferred[i]->setTranslation(glm::vec3((2.0f*i) - (modelsDeferred.size()), 0.0f, sin(globalP*i) + 2.0f));
		}*/


		//for (int i = 2; i < 50; ++i) {
		//	int off = i*5;
		//	for (int j = 0; j < 5; ++j) {
		//		int n = j + off;
		//		models[n]->setTranslation(glm::vec3((2.0f*i) - (models.size()), 0.0f, sin(globalP*i) + 2.0f));
		//	}
		//}









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
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("forward.meshes"));

		// for each model
		// model = group of meshes
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

				//uint32_t offset1 = model->matrixIndex * alignedMatrixSize;
				uint32_t offset1 = model->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
				setNum = 1;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, 1, &rscs.descriptorSets->get("forward.matrix"), 1, &offset1);


				if (lastMaterialName != mesh.meshBuffer.materialName) {

					lastMaterialName = mesh.meshBuffer.materialName;
					vkx::Material m = this->assetManager.materials.get(mesh.meshBuffer.materialName);

					// Set 2: Binding: 0
					//uint32_t offset2 = m.index * alignedMaterialSize;
					uint32_t offset2 = m.index * static_cast<uint32_t>(alignedMaterialSize);
					setNum = 2;
					cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, 1, &rscs.descriptorSets->get("forward.material"), 1, &offset2);

					// bind texture: // todo: implement a better way to bind textures
					// Set: 3 Binding: 0
					setNum = 3;
					cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, m.descriptorSet, nullptr);
				}

				// draw:
				cmdBuffer.drawIndexed(mesh.meshBuffer.indexCount, 1, 0, 0, 0);
			}

		}











		// SKINNED MESHES:

		// bind skinned mesh pipeline
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("forward.skinnedMeshes"));
		for (auto &skinnedMesh : skinnedMeshes) {
			// bind vertex & index buffers
			cmdBuffer.bindVertexBuffers(skinnedMesh->vertexBufferBinding, skinnedMesh->meshBuffer.vertices.buffer, vk::DeviceSize());
			cmdBuffer.bindIndexBuffer(skinnedMesh->meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

			// descriptor set #
			uint32_t setNum;

			// bind scene descriptor set
			// Set 0: Binding 0:
			setNum = 0;
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, rscs.descriptorSets->get("forward.scene"), nullptr);

			// there is a bone uniform, set: 0, binding: 1


			// Set 1: Binding 0:
			//uint32_t offset1 = skinnedMesh->matrixIndex * alignedMatrixSize;
			uint32_t offset1 = skinnedMesh->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
			setNum = 1;
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, 1, &rscs.descriptorSets->get("forward.matrix"), 1, &offset1);

			if (lastMaterialName != skinnedMesh->meshBuffer.materialName) {
				lastMaterialName = skinnedMesh->meshBuffer.materialName;
				vkx::Material m = this->assetManager.materials.get(skinnedMesh->meshBuffer.materialName);

				// Set 2: Binding: 0
				//uint32_t offset2 = m.index * alignedMaterialSize;
				uint32_t offset2 = m.index * static_cast<uint32_t>(alignedMaterialSize);
				setNum = 2;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, 1, &rscs.descriptorSets->get("forward.material"), 1, &offset2);

				// bind texture:
				// Set 3: Binding 0:
				setNum = 3;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, m.descriptorSet, nullptr);
			}

			// bind bone descriptor set
			//setNum = 0;
			// Set 0: Binding 1:
			//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, rscs.descriptorSets->get("forward.bones"), nullptr);


			// draw:
			cmdBuffer.drawIndexed(skinnedMesh->meshBuffer.indexCount, 1, 0, 0, 0);
		}





		{
			/* DEFERRED QUAD */

			updateUniformBufferDeferredLights();

			vk::Viewport viewport = vkx::viewport(size);
			cmdBuffer.setViewport(0, viewport);
			cmdBuffer.setScissor(0, vkx::rect2D(size));


			// renders quad
			uint32_t setNum = 3;// important!
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

			if (!fullDeferred) {
				viewport.x = viewport.width * 0.5f;
				viewport.y = viewport.height * 0.5f;
			}



			cmdBuffer.setViewport(0, viewport);
			// Final composition as full screen quad
			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("deferred.deferred"));
			cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshBuffers.quad.vertices.buffer, { 0 });
			cmdBuffer.bindIndexBuffer(meshBuffers.quad.indices.buffer, 0, vk::IndexType::eUint32);
			cmdBuffer.drawIndexed(6, 1, 0, 0, 1);
		}

		



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


				//uint32_t offset1 = model->matrixIndex * alignedMatrixSize;
				uint32_t offset1 = model->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
				setNum = 1;
				offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("deferred.offscreen"), setNum, 1, &rscs.descriptorSets->get("deferred.matrix"), 1, &offset1);


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

					setNum = 2;
					offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("deferred.offscreen"), setNum, m.descriptorSet, nullptr);
				}


				// draw:
				offscreenCmdBuffer.drawIndexed(mesh.meshBuffer.indexCount, 1, 0, 0, 0);
			}

		}














		// SKINNED MESHES:

		// bind skinned mesh pipeline
		offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("deferred.skinnedMeshes"));
		for (auto &skinnedMesh : skinnedMeshesDeferred) {
			// bind vertex & index buffers
			offscreenCmdBuffer.bindVertexBuffers(skinnedMesh->vertexBufferBinding, skinnedMesh->meshBuffer.vertices.buffer, vk::DeviceSize());
			offscreenCmdBuffer.bindIndexBuffer(skinnedMesh->meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

			// descriptor set #
			uint32_t setNum;

			// bind scene descriptor set
			// Set 0: Binding 0:
			setNum = 0;
			offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("deferred.offscreen"), setNum, rscs.descriptorSets->get("deferred.scene"), nullptr);

			// there is a bone uniform, set: 0, binding: 1


			// Set 1: Binding 0:
			//uint32_t offset1 = skinnedMesh->matrixIndex * alignedMatrixSize;
			uint32_t offset1 = skinnedMesh->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
			setNum = 1;
			offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("deferred.offscreen"), setNum, 1, &rscs.descriptorSets->get("deferred.matrix"), 1, &offset1);

			if (lastMaterialName != skinnedMesh->meshBuffer.materialName) {
				lastMaterialName = skinnedMesh->meshBuffer.materialName;
				vkx::Material m = this->assetManager.materials.get(skinnedMesh->meshBuffer.materialName);

				// Set 2: Binding: 0
				//uint32_t offset2 = m.index * alignedMaterialSize;
				//uint32_t offset2 = m.index * static_cast<uint32_t>(alignedMaterialSize);
				//setNum = 2;
				//offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("forward.basic"), setNum, 1, &rscs.descriptorSets->get("forward.material"), 1, &offset2);

				// bind texture:
				// Set 2: Binding 0:
				setNum = 2;
				offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("deferred.offscreen"), setNum, m.descriptorSet, nullptr);
			}


			// draw:
			offscreenCmdBuffer.drawIndexed(skinnedMesh->meshBuffer.indexCount, 1, 0, 0, 0);
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

	void generateQuads() {
		// Setup vertices for multiple screen aligned quads
		// Used for displaying final result and debug 
		//struct Vertex {
		//	float pos[3];
		//	float uv[2];
		//	float col[3];
		//	float normal[3];
		//};
		struct Vertex {
			float pos[3];
			float uv[2];
			float col[3];
			float normal[3];
			float tangent[3];
			float dummy1[4];
			float dummy2[4];
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

		vulkanApp::prepare();
		offscreen.prepare();

		offscreen.depthFinalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		//OffscreenExampleBase::prepare();

		loadTextures();
		generateQuads();


		prepareVertexDescriptions();



		prepareUniformBuffers();
		prepareUniformBuffersDeferred();

		prepareDescriptorSetLayouts();
		prepareDescriptorPools();
		prepareDescriptorSets();

		preparePipelines();
		prepareDeferredPipelines();

		start();

		updateWorld();
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