
/*
* Vulkan Example - Skeletal animation
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanApp.h"



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

#define SSAO_KERNEL_SIZE 64
#define SSAO_RADIUS 2.0f
#define SSAO_NOISE_DIM 4

#define NUM_POINT_LIGHTS 70// 100
#define NUM_SPOT_LIGHTS 2
#define NUM_DIR_LIGHTS 3
#define NUM_LIGHTS_TOTAL 5

#define SSAO_ON 1

#define TEST_DEFINE 0




std::vector<vkx::VertexComponent> meshVertexLayout =
{
	vkx::VertexComponent::VERTEX_COMPONENT_POSITION,
	vkx::VertexComponent::VERTEX_COMPONENT_UV,
	vkx::VertexComponent::VERTEX_COMPONENT_COLOR,
	vkx::VertexComponent::VERTEX_COMPONENT_NORMAL,
	vkx::VertexComponent::VERTEX_COMPONENT_DUMMY_VEC4,
	vkx::VertexComponent::VERTEX_COMPONENT_DUMMY_VEC4
};


std::vector<vkx::VertexComponent> skinnedMeshVertexLayout =
{
	vkx::VertexComponent::VERTEX_COMPONENT_POSITION,
	vkx::VertexComponent::VERTEX_COMPONENT_UV,
	vkx::VertexComponent::VERTEX_COMPONENT_COLOR,
	vkx::VertexComponent::VERTEX_COMPONENT_NORMAL,
	vkx::VertexComponent::VERTEX_COMPONENT_DUMMY_VEC4,
	vkx::VertexComponent::VERTEX_COMPONENT_DUMMY_VEC4
};


std::vector<vkx::VertexComponent> deferredVertexLayout =
{
	vkx::VertexComponent::VERTEX_COMPONENT_POSITION,
	vkx::VertexComponent::VERTEX_COMPONENT_UV,
	vkx::VertexComponent::VERTEX_COMPONENT_COLOR,
	vkx::VertexComponent::VERTEX_COMPONENT_NORMAL
};



std::vector<vkx::VertexComponent> SSAOVertexLayout =
{
	vkx::VertexComponent::VERTEX_COMPONENT_POSITION,
	vkx::VertexComponent::VERTEX_COMPONENT_UV,
	vkx::VertexComponent::VERTEX_COMPONENT_COLOR,
	vkx::VertexComponent::VERTEX_COMPONENT_NORMAL,
	vkx::VertexComponent::VERTEX_COMPONENT_TANGENT,
	vkx::VertexComponent::VERTEX_COMPONENT_DUMMY_VEC4,
	vkx::VertexComponent::VERTEX_COMPONENT_DUMMY_VEC4
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

float rnd(float min, float max) {
	float range = max - min;
	float num = rand0t1()*range;
	num = num + min;
	return num;
}




// todo: move this somewhere else
btConvexHullShape* createConvexHullFromMesh(vkx::MeshLoader *meshLoader, float scale = 1.0f) {
	btConvexHullShape *convexHullShape = new btConvexHullShape();
	for (int i = 0; i < meshLoader->m_Entries.size(); ++i) {
		for (int j = 0; j < meshLoader->m_Entries[i].Indices.size(); ++j) {
			uint32_t index = meshLoader->m_Entries[i].Indices[j];
			glm::vec3 point = meshLoader->m_Entries[i].Vertices[index].m_pos*scale;
			btVector3 p = btVector3(point.x, point.y, point.z);
			convexHullShape->addPoint(p);
		}
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





class VulkanExample : public vkx::vulkanApp {

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
		vkx::CreateBufferResult sceneVS;		// scene data
		vkx::CreateBufferResult matrixVS;		// matrix data
		vkx::CreateBufferResult materialVS;		// material data
		vkx::CreateBufferResult bonesVS;		// bone data for all skinned meshes // max of 1000 skinned meshes w/64 bones/mesh
	} uniformData;


	// static scene uniform buffer
	struct {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;

		//glm::mat4 g1;
		//glm::mat4 g2;
	} uboScene;

	// todo: fix this
	struct MatrixNode {
		glm::mat4 model;
		uint32_t boneIndex;
		glm::vec3 padding;
		glm::vec4 padding2[3];
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

	struct TestingVariables {
		std::vector<std::shared_ptr<vkx::Model>> modelsDeferred;
		float splitDepths[4] = { 0.1f, 5.0f, 20.0f, 256.0f };
	} temporary;


	unsigned int alignedMatrixSize;
	unsigned int alignedMaterialSize;

	size_t dynamicAlignment;

	float globalP = 0.0f;

	bool debugDisplay = false;
	float fullDeferred = false;

	//glm::vec3 lightPos = glm::vec3(1.0f, -2.0f, 2.0f);
	//glm::vec4 lightPos = glm::vec4(1.0f, -2.0f, 2.0f, 1.0f);

	std::string consoleLog;



	// todo: remove this:
	struct {
		vkx::Texture colorMap;
		vkx::Texture ssaoNoise;
	} textures;


	struct {
		vk::PipelineVertexInputStateCreateInfo inputState;
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	} vertices;




	struct {
		glm::mat4 model;
		glm::mat4 projection;
	} uboVS;

	struct {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	} uboOffscreenVS;

	struct PointLight {
		glm::vec4 position;
		glm::vec4 color;

		float radius;
		float quadraticFalloff;
		float linearFalloff;
		float _pad;
	};

	struct SpotLight {
		glm::vec4 position;		// position
		glm::vec4 target;		// where the light points
		glm::vec4 color;		// color of the light
		glm::mat4 viewMatrix;	// view matrix (used in geometry shader and fragment shader)

		float innerAngle = 40.0f;
		float outerAngle = 60.0f;
		float zNear = 0.1f;
		float zFar = 64.0f;

		float range = 100.0f;
		float pad1;
		float pad2;
		float pad3;
	};

	struct DirectionalLight {
		glm::vec4 direction = glm::vec4(0.3f, 0.0f, -10.0f, 0.0f);	// unit directional vector
		glm::vec4 color;		// color of the light
		glm::mat4 viewMatrix;	// view matrix (used in geometry shader and fragment shader)
		float zNear = -32.0f;
		float zFar = 32.0f;
		float size = 15.0f;// size of the orthographic projection
		float pad1;

		// todo: better solution may be possible:
		float cascadeNear = 0.1;
		float cascadeFar = 4.0f;

		float pad2;
		float pad3;
		//float pad4;

		//float pad5;
		//float pad6;
		//float pad7;
		//float pad8;
	};


	//struct CSMLight {
	//	glm::vec4 direction = glm::vec4(0.3f, 0.0f, -10.0f, 0.0f);	// unit directional vector
	//	glm::vec4 color;		// color of the light
	//	glm::mat4 viewMatrix;	// view matrix (used in geometry shader and fragment shader)
	//	float zNear = -32.0f;
	//	float zFar = 32.0f;
	//	float size = 15.0f;// size of the orthographic projection
	//};

	struct {

		glm::vec4 viewPos;
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 invViewProj;
		//glm::vec4 pad[];
		
		PointLight pointlights[NUM_POINT_LIGHTS];
		SpotLight spotlights[NUM_SPOT_LIGHTS];
		DirectionalLight directionalLights[NUM_DIR_LIGHTS];
	} uboFSLights;

	// ssao
	struct {
		glm::mat4 projection;
		glm::mat4 view;// added 4/20/17
		//uint32_t ssao = true;
		//uint32_t ssaoOnly = false;
		//uint32_t ssaoBlur = true;
	} uboSSAOParams;

	struct {
		glm::vec4 samples[SSAO_KERNEL_SIZE];
	} uboSSAOKernel;

	// This UBO stores the shadow matrices for all of the light sources
	// The matrices are indexed using geometry shader instancing
	struct {
		glm::mat4 spotlightMVP[NUM_SPOT_LIGHTS];
		glm::mat4 dirlightMVP[NUM_DIR_LIGHTS];
	} uboShadowGS;

	struct {
		vkx::UniformData vsFullScreen;
		vkx::UniformData vsOffscreen;
		vkx::UniformData fsLights;

		vkx::UniformData matrixVS;


		vkx::UniformData ssaoKernel;
		vkx::UniformData ssaoParams;

		vkx::UniformData gsShadow;

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

	// todo: move this:
	bool rayPicking = false;
	btRigidBody *m_pickedBody;
	btScalar m_oldPickingDist;
	btPoint2PointConstraint *m_pickedConstraint = nullptr;
	btVector3 m_oldPickingPos;
	int m_savedState;



	vkx::Offscreen offscreen;

	vk::Fence renderFence;





	VulkanExample() : vkx::vulkanApp(ENABLE_VALIDATION), offscreen(context) {




		rscs.pipelineLayouts = new vkx::PipelineLayoutList(context.device);
		rscs.pipelines = new vkx::PipelineList(context.device);

		rscs.descriptorPools = new vkx::DescriptorPoolList(context.device);
		rscs.descriptorSetLayouts = new vkx::DescriptorSetLayoutList(context.device);
		rscs.descriptorSets = new vkx::DescriptorSetList(context.device);


		//camera.setTranslation(glm::vec3(-20.0f, 0.0f, 7.0f));
		//glm::quat initialOrientation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		//camera.setRotation(initialOrientation);

		camera.setTranslation({ 0.0f, -2.0f, 1.6f });
		camera.rotateWorldX(PI / 2.0);



		//camera.setProjection(80.0f, (float)settings.windowSize.width / (float)settings.windowSize.height, 1.0f, 512.0f);
		camera.setProjection(80.0f, (float)settings.windowSize.width / (float)settings.windowSize.height, 0.1f, 256.0f);


		matrixNodes.resize(1000);
		materialNodes.resize(1000);


		// todo: move this somewhere else
		unsigned int alignment = (uint32_t)context.deviceProperties.limits.minUniformBufferOffsetAlignment;

		alignedMatrixSize = (unsigned int)(alignedSize(alignment, sizeof(MatrixNode)));
		alignedMaterialSize = (unsigned int)(alignedSize(alignment, sizeof(vkx::MaterialProperties)));


		title = "Vulkan test";

		renderFence = context.device.createFence(vk::FenceCreateInfo(), nullptr);// temporary


	}

	~VulkanExample() {

		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class


		// todo: fix this all up

		offscreen.destroy();
		imGui->destroy();


		// destroy pipelines



		// destroy pipeline layouts

		// destroy uniform buffers
		uniformData.sceneVS.destroy();
		uniformData.matrixVS.destroy();
		uniformData.materialVS.destroy();
		uniformData.bonesVS.destroy();




		// destroy offscreen uniform buffers
		uniformDataDeferred.vsOffscreen.destroy();
		uniformDataDeferred.vsFullScreen.destroy();
		uniformDataDeferred.fsLights.destroy();

		uniformDataDeferred.matrixVS.destroy();
		uniformDataDeferred.ssaoKernel.destroy();
		uniformDataDeferred.ssaoParams.destroy();

		// destroy textures:
		// todo: move / clean this up
		textures.ssaoNoise.destroy();
		textures.colorMap.destroy();


		// destroy offscreen command buffer
		context.device.freeCommandBuffers(cmdPool, offscreenCmdBuffer);

		context.device.destroyFence(renderFence, nullptr);// temp


		for (auto &mesh : meshes) {
			mesh->destroy();
		}
		meshes.clear();

		for (auto &model : models) {
			model->destroy();
		}
		models.clear();

		for (auto &skinnedMesh : skinnedMeshes) {
			skinnedMesh->destroy();
		}
		skinnedMeshes.clear();

		for (auto &mesh : meshesDeferred) {
			mesh->destroy();
			mesh->meshBuffer->destroy();
		}
		meshesDeferred.clear();

		for (auto &model : modelsDeferred) {
			model->destroy();
		}
		modelsDeferred.clear();

		for (auto &skinnedMesh : skinnedMeshesDeferred) {
			skinnedMesh->destroy();
		}
		skinnedMeshesDeferred.clear();


		for (auto &physicsObject : physicsObjects) {
			physicsObject->destroy();
		}




		// Destroy and free resources


		assetManager.destroy();


		rscs.pipelines->destroy();

		rscs.pipelineLayouts->destroy();

		rscs.descriptorPools->destroy();

		// todo: destroy desriptorsetlist's descriptor sets, needs command pool
		rscs.descriptorSets->destroy();// does nothing, fix

		rscs.descriptorSetLayouts->destroy();






		//skinnedMesh->meshBuffer.destroy();
		//delete(skinnedMesh->meshLoader);
		//delete(skinnedMesh);
		//textures.skybox.destroy();

	}

	// todo: move this somewhere else:
	glm::vec3 generateRay() {

		float mouseX = mouse.current.x;
		float mouseY = mouse.current.y;
		float screenWidth = settings.windowSize.width;
		float screenHeight = settings.windowSize.height;

		// The ray Start and End positions, in Normalized device Coordinates (Have you read Tutorial 4 ?)
		glm::vec4 lRayStart_NDC(
			((float)mouseX / (float)screenWidth - 0.5f) * 2.0f, // [0,1024] -> [-1,1]
			((float)mouseY / (float)screenHeight - 0.5f) * 2.0f, // [0, 768] -> [-1,1]
			-1.0, // The near plane maps to Z=-1 in Normalized device Coordinates
			1.0f
		);
		glm::vec4 lRayEnd_NDC(
			((float)mouseX / (float)screenWidth - 0.5f) * 2.0f,
			((float)mouseY / (float)screenHeight - 0.5f) * 2.0f,
			0.0,
			1.0f
		);


		glm::mat4 M = glm::inverse(camera.matrices.projection * camera.matrices.view);
		glm::vec4 lRayStart_world = M * lRayStart_NDC; lRayStart_world /= lRayStart_world.w;
		glm::vec4 lRayEnd_world = M * lRayEnd_NDC; lRayEnd_world /= lRayEnd_world.w;


		glm::vec3 lRayDir_world(lRayEnd_world - lRayStart_world);
		lRayDir_world = glm::normalize(lRayDir_world);

		glm::vec3 ray_end = camera.transform.translation + lRayDir_world*200.0f;
		return ray_end;
	}






	void prepareVertexDescriptions() {


		// skinned mesh vertex layout

		// Binding description
		vertices.bindingDescriptions = std::vector<vk::VertexInputBindingDescription>{
			vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, vkx::vertexSize(SSAOVertexLayout), vk::VertexInputRate::eVertex),
		};

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions = std::vector<vk::VertexInputAttributeDescription>{
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0),						// Location 0 : Position
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32Sfloat,		sizeof(float) * 3),		// Location 1 : (UV) Texture coordinates
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32B32Sfloat,	sizeof(float) * 5),		// Location 2 : Color
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat,	sizeof(float) * 8),		// Location 3 : Normal
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, vk::Format::eR32G32B32Sfloat,	sizeof(float) * 11),	// Location 4 : Tangent
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 5, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 14),	// Location 5 : Bone weights
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 6, vk::Format::eR32G32B32A32Sint,	sizeof(float) * 18),	// Location 6 : Bone IDs
		};


		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();

		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}






	void prepareDescriptorPools() {


		// todo: I don't need this many descriptor pools
		// like at all
		// fix this:

		//// scene data
		//std::vector<vk::DescriptorPoolSize> descriptorPoolSizes0 = {
		//	vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),// mostly static data
		//};
		//rscs.descriptorPools->add("forward.scene", descriptorPoolSizes0, 1);

		//// matrix data
		//std::vector<vk::DescriptorPoolSize> descriptorPoolSizes1 = {
		//	vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),// non-static data
		//};
		//rscs.descriptorPools->add("forward.matrix", descriptorPoolSizes1, 1);

		//// material data
		//std::vector<vk::DescriptorPoolSize> descriptorPoolSizes2 = {
		//	vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),
		//};
		//rscs.descriptorPools->add("forward.material", descriptorPoolSizes2, 1);





		// combined image sampler
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes4 = {
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 100),
		};
		rscs.descriptorPools->add("forward.textures", descriptorPoolSizes4, 100);

		this->assetManager.materialDescriptorPool = rscs.descriptorPools->getPtr("forward.textures");








		/* DEFERRED */


		// later:
		// deferred:
		// scene data
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes5 = {
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),// mostly static data
		};
		rscs.descriptorPools->add("offscreen.scene", descriptorPoolSizes5, 1);


		// matrix data
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes6 = {
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 2),// non-static data
		};
		rscs.descriptorPools->add("offscreen.matrix", descriptorPoolSizes6, 2);



		// combined image sampler
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes7 = {
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 100),
		};
		rscs.descriptorPools->add("offscreen.textures", descriptorPoolSizes7, 100);



		std::vector<vk::DescriptorPoolSize> descriptorPoolSizesDeferred = {
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 16),
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 16)
		};
		rscs.descriptorPools->add("deferred", descriptorPoolSizesDeferred, 4);

	}





	void prepareDescriptorSetLayouts() {


		//// descriptor set layout 0
		//// scene data
		//std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings0 = {
		//	// Set 0: Binding 0 : Vertex shader uniform buffer
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBuffer,
		//		vk::ShaderStageFlagBits::eVertex,
		//		0),
		//		   
		//	// Set 0: Binding 1: Vertex shader uniform buffer// bone data
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBuffer,
		//		vk::ShaderStageFlagBits::eVertex,
		//		1),
		//};
		//rscs.descriptorSetLayouts->add("forward.scene", descriptorSetLayoutBindings0);

		//// descriptor set layout 1
		//// matrix data
		//std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings1 = {

		//	// Set 1: Binding 0 : Vertex shader dynamic uniform buffer
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBufferDynamic,
		//		vk::ShaderStageFlagBits::eVertex,
		//		0),
		//};
		//rscs.descriptorSetLayouts->add("forward.matrix", descriptorSetLayoutBindings1);

		// descriptor set layout 2
		// material data
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings2 = {

			// Set 2: Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),
		};
		rscs.descriptorSetLayouts->add("forward.material", descriptorSetLayoutBindings2);






		// descriptor set layout 3
		// combined image sampler
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings3 = {

			// Set 3: Binding 0 : Fragment shader color map image sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),
			// Set 3: Binding 1: Fragment shader specular
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1),
			// Set 3: Binding 2: Fragment shader bump
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				2),
		};
		rscs.descriptorSetLayouts->add("forward.textures", descriptorSetLayoutBindings3);
		this->assetManager.materialDescriptorSetLayout = rscs.descriptorSetLayouts->getPtr("forward.textures");




		////// descriptor set layout 4
		////// bone data
		////std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings4 =
		////{
		////	// Set 4: Binding 0 : Vertex shader uniform buffer
		////	vkx::descriptorSetLayoutBinding(
		////		vk::DescriptorType::eUniformBuffer,
		////		vk::ShaderStageFlagBits::eVertex,
		////		0),
		////};

		////vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo4 =
		////	vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings4.data(), descriptorSetLayoutBindings4.size());
		////rscs.descriptorSetLayouts->add("forward.bones", descriptorSetLayoutCreateInfo4);









		//// use descriptor set layouts
		//// to form pipeline layout

		//std::vector<vk::DescriptorSetLayout> descriptorSetLayouts{
		//	rscs.descriptorSetLayouts->get("forward.scene"),
		//	rscs.descriptorSetLayouts->get("forward.matrix"),
		//	rscs.descriptorSetLayouts->get("forward.material"),
		//	rscs.descriptorSetLayouts->get("forward.textures"),
		//};

		//// create pipelineLayout from descriptorSetLayouts
		//vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vkx::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());
		//rscs.pipelineLayouts->add("forward.basic", pPipelineLayoutCreateInfo);























		/* DEFERRED / OFFSCREEN */










		// descriptor set layout 0
		// scene data
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings5 = {
			// Set 0: Binding 0 : Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),

			// Set 0: Binding 1: Vertex shader uniform buffer// bone data
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				1),
		};
		rscs.descriptorSetLayouts->add("offscreen.scene", descriptorSetLayoutBindings5);


		// descriptor set layout 1
		// matrix data
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings6 = {
			// Set 1: Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),
		};
		rscs.descriptorSetLayouts->add("offscreen.matrix", descriptorSetLayoutBindings6);

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
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings7 = {

			// Set 2: Binding 0: Fragment shader color map image sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),
			// Set 2: Binding 1: Fragment shader specular
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1),
			// Set 2: Binding 2: Fragment shader bump
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				2),
		};
		rscs.descriptorSetLayouts->add("offscreen.textures", descriptorSetLayoutBindings7);








		/* deferred ----------------------------*/

		// descriptor set layout for full screen quad // deferred pass

		// Deferred shading layout
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindingsDeferred = {

			// Set 3: Binding 0: Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				//vk::ShaderStageFlagBits::eVertex,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry,// added geometry 4/12/17
				0),
			// Set 3: Binding 1: Position texture target / Scene colormap
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1),
			// Set 3: Binding 2: Normals texture target
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				2),
			// Set 3: Binding 3: Albedo texture target
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				3),

			// Set 3: Binding 4: SSAO Sampler texture target
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				4),

			// Set 3: Binding 5: Shadow Map
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				5),

			// Set 3: Binding 6: Fragment shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eFragment,
				6),
		};
		rscs.descriptorSetLayouts->add("deferred", descriptorSetLayoutBindingsDeferred);







		std::vector<vk::DescriptorSetLayout> descriptorSetLayoutsDeferred{
			rscs.descriptorSetLayouts->get("offscreen.scene"),
			rscs.descriptorSetLayouts->get("offscreen.matrix"),
			rscs.descriptorSetLayouts->get("offscreen.textures"),
			rscs.descriptorSetLayouts->get("deferred"),
		};

		// use all descriptor set layouts
		// to form pipeline layout

		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfoDeferred = vkx::pipelineLayoutCreateInfo(descriptorSetLayoutsDeferred.data(), descriptorSetLayoutsDeferred.size());



		
		// Offscreen (scene) rendering pipeline layout
		rscs.pipelineLayouts->add("offscreen", pPipelineLayoutCreateInfoDeferred);

		rscs.pipelineLayouts->add("deferred", pPipelineLayoutCreateInfoDeferred);



		//std::vector<vk::DescriptorSetLayout> descriptorSetLayoutsOffscreen{
		//	rscs.descriptorSetLayouts->get("offscreen.scene"),
		//	rscs.descriptorSetLayouts->get("offscreen.matrix"),
		//	rscs.descriptorSetLayouts->get("offscreen.textures"),
		//	rscs.descriptorSetLayouts->get("deferred"),// todo: remove
		//};

		//vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfoOffscreen = vkx::pipelineLayoutCreateInfo(descriptorSetLayoutsOffscreen.data(), descriptorSetLayoutsOffscreen.size());


		//// Offscreen (scene) rendering pipeline layout
		//rscs.pipelineLayouts->add("offscreen", pPipelineLayoutCreateInfoOffscreen);






		//std::vector<vk::DescriptorSetLayout> descriptorSetLayoutsDeferred{
		//	rscs.descriptorSetLayouts->get("deferred"),
		//};
		//vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfoDeferred = vkx::pipelineLayoutCreateInfo(descriptorSetLayoutsDeferred.data(), descriptorSetLayoutsDeferred.size());

		//rscs.pipelineLayouts->add("deferred", pPipelineLayoutCreateInfoDeferred);








		// ---------------------------------------------------------------------------------------
		// SSAO Generate:


		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings9 = {
			// Set 0: Binding 0 : // FS Depth
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),

			// Set 0: Binding 1: // FS Normals
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1),

			// Set 0: Binding 2: // FS SSAO Noise
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				2),

			// Set 0: Binding 3: // FS SSAO Kernel UBO
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eFragment,
				3),

			// Set 0: Binding 4: // FS Params UBO
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eFragment,
				4),
		};
		rscs.descriptorSetLayouts->add("offscreen.ssao.generate", descriptorSetLayoutBindings9);


		// use descriptor set layouts
		// to form pipeline layout

		std::vector<vk::DescriptorSetLayout> descriptorSetLayoutsSSAO{
			rscs.descriptorSetLayouts->get("offscreen.ssao.generate"),// descriptor set layout
		};

		// create pipelineLayout from descriptorSetLayouts
		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfoSSAO = vkx::pipelineLayoutCreateInfo(descriptorSetLayoutsSSAO.data(), descriptorSetLayoutsSSAO.size());
		rscs.pipelineLayouts->add("offscreen.ssaoGenerate", pPipelineLayoutCreateInfoSSAO);






		// ---------------------------------------------------------------------------------------
		// SSAO Blur:

		// combined image sampler
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings10 = {
			// Set 0: Binding 0 : // FS Sampler SSAO
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),
		};
		rscs.descriptorSetLayouts->add("offscreen.ssao.blur", descriptorSetLayoutBindings10);


		// use descriptor set layouts
		// to form pipeline layout

		std::vector<vk::DescriptorSetLayout> descriptorSetLayoutsSSAOBlur{
			rscs.descriptorSetLayouts->get("offscreen.ssao.blur"),// descriptor set layout
		};

		// create pipelineLayout from descriptorSetLayouts
		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfoSSAOBlur = vkx::pipelineLayoutCreateInfo(descriptorSetLayoutsSSAOBlur.data(), descriptorSetLayoutsSSAOBlur.size());
		rscs.pipelineLayouts->add("offscreen.ssaoBlur", pPipelineLayoutCreateInfoSSAOBlur);







		// ---------------------------------------------------------------------------------------
		// ---------------------------------------------------------------------------------------
		// shadow:

		// Geometry shader descriptor set layout binding
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindingsShadow = {
			// Set 0: Binding 0: Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry,// geometry shader
				0),
		};
		rscs.descriptorSetLayouts->add("shadow.scene", descriptorSetLayoutBindingsShadow);

		// Geometry shader descriptor set layout binding
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindingsShadowMatrix = {
			// Set 0: Binding 0: Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry,// geometry shader
				0),
		};
		rscs.descriptorSetLayouts->add("shadow.matrix", descriptorSetLayoutBindingsShadowMatrix);


		std::vector<vk::DescriptorSetLayout> descriptorSetLayoutsShadow{
			rscs.descriptorSetLayouts->get("shadow.scene"),	// descriptor set layout
			rscs.descriptorSetLayouts->get("shadow.matrix"),// descriptor set layout
		};

		// create pipelineLayout from descriptorSetLayouts
		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfoShadow = vkx::pipelineLayoutCreateInfo(descriptorSetLayoutsShadow.data(), descriptorSetLayoutsShadow.size());
		rscs.pipelineLayouts->add("offscreen.shadow", pPipelineLayoutCreateInfoShadow);


	}

	void prepareDescriptorSets() {


		// create descriptor sets with descriptor pools and set layouts






		// descriptor set 0
		// scene data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo5 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("offscreen.scene"), &rscs.descriptorSetLayouts->get("offscreen.scene"), 1);
		rscs.descriptorSets->add("offscreen.scene", descriptorSetAllocateInfo5);

		// descriptor set 1
		// matrix data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo6 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("offscreen.matrix"), &rscs.descriptorSetLayouts->get("offscreen.matrix"), 1);
		rscs.descriptorSets->add("offscreen.matrix", descriptorSetAllocateInfo6);

		// descriptor set 2
		// textures data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo7 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("offscreen.textures"), &rscs.descriptorSetLayouts->get("offscreen.textures"), 1);
		rscs.descriptorSets->add("offscreen.textures", descriptorSetAllocateInfo7);


		// descriptor set 3
		// offscreen textures data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo8 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("deferred"), &rscs.descriptorSetLayouts->get("deferred"), 1);
		rscs.descriptorSets->add("deferred", descriptorSetAllocateInfo8);







		// vk::Image descriptor for the offscreen texture targets
		vk::DescriptorImageInfo texDescriptorPosition =
			vkx::descriptorImageInfo(offscreen.framebuffers[0].attachments[0].sampler, offscreen.framebuffers[0].attachments[0].view, vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::DescriptorImageInfo texDescriptorNormal =
			vkx::descriptorImageInfo(offscreen.framebuffers[0].attachments[0].sampler, offscreen.framebuffers[0].attachments[1].view, vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::DescriptorImageInfo texDescriptorAlbedo =
			vkx::descriptorImageInfo(offscreen.framebuffers[0].attachments[0].sampler, offscreen.framebuffers[0].attachments[2].view, vk::ImageLayout::eShaderReadOnlyOptimal);


		vk::DescriptorImageInfo texDescriptorSSAOBlurred =
			vkx::descriptorImageInfo(offscreen.framebuffers[0].attachments[0].sampler, offscreen.framebuffers[2].attachments[0].view, vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::DescriptorImageInfo texDescriptorShadowMap =
			vkx::descriptorImageInfo(offscreen.framebuffers[3].attachments[0].sampler, offscreen.framebuffers[3].attachments[0].view, vk::ImageLayout::eShaderReadOnlyOptimal);
		//vkx::descriptorImageInfo(offscreen.framebuffers[0].attachments[0].sampler, offscreen.framebuffers[3].attachments[0].view, vk::ImageLayout::eShaderReadOnlyOptimal);

		// depth attachment:
		vk::DescriptorImageInfo texDescriptorDepthStencil =
			vkx::descriptorImageInfo(offscreen.framebuffers[0].attachments[0].sampler, offscreen.framebuffers[0].attachments[3].view, vk::ImageLayout::eShaderReadOnlyOptimal);



		// Offscreen texture targets:
		std::vector<vk::WriteDescriptorSet> writeDescriptorSets2 = {


			// set 3: Binding 0: Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred"),
				vk::DescriptorType::eUniformBuffer,
				0,
				&uniformDataDeferred.vsFullScreen.descriptor),
			// set 3: Binding 1: Position texture target
			// replaced with depth
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred"),
				vk::DescriptorType::eCombinedImageSampler,
				1,
				//&texDescriptorPosition),
				&texDescriptorPosition),
			// set 3: Binding 2: Normals texture target
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred"),
				vk::DescriptorType::eCombinedImageSampler,
				2,
				&texDescriptorNormal),
			// set 3: Binding 3: Albedo texture target
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred"),
				vk::DescriptorType::eCombinedImageSampler,
				3,
				&texDescriptorAlbedo),

			// set 3: Binding 4: SSAO Blurred
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred"),
				vk::DescriptorType::eCombinedImageSampler,
				4,
				&texDescriptorSSAOBlurred),

			// set 3: Binding 5: Shadow Map
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred"),
				vk::DescriptorType::eCombinedImageSampler,
				5,
				&texDescriptorShadowMap),

			// set 3: Binding 6: Fragment shader uniform buffer// lights
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("deferred"),
				vk::DescriptorType::eUniformBuffer,
				6,
				&uniformDataDeferred.fsLights.descriptor),




		};

		context.device.updateDescriptorSets(writeDescriptorSets2, nullptr);









		// offscreen descriptor set
		// todo: combine with above
		// or dont

		std::vector<vk::WriteDescriptorSet> offscreenWriteDescriptorSets = {
			// Set 0: Binding 0: Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("offscreen.scene"),
				vk::DescriptorType::eUniformBuffer,
				0,
				&uniformDataDeferred.vsOffscreen.descriptor),

			// Set 0: Binding 1: bones uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("offscreen.scene"),// descriptor set 0
				vk::DescriptorType::eUniformBuffer,
				1,// binding 1
				&uniformData.bonesVS.descriptor),// bind to forward descriptor since it's the same


			// Set 1: Binding 0: Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("offscreen.matrix"),
				vk::DescriptorType::eUniformBufferDynamic,
				0,
				&uniformData.matrixVS.descriptor),// bind to forward descriptor since it's the same

			//// Set 2: Binding 0: Scene color map
			// replaced with materials write descriptor sets
			//vkx::writeDescriptorSet(
			//	rscs.descriptorSets->get("offscreen.textures"),
			//	vk::DescriptorType::eCombinedImageSampler,
			//	0,
			//	&textures.colorMap.descriptor),




		};
		context.device.updateDescriptorSets(offscreenWriteDescriptorSets, nullptr);



		// -----------------------------------------------------------------------------------
		// SSAO ------------------------------------------------------------------------------

		{
			// descriptor set
			vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo9 =
				vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("deferred"), &rscs.descriptorSetLayouts->get("offscreen.ssao.generate"), 1);
			rscs.descriptorSets->add("offscreen.ssao.generate", descriptorSetAllocateInfo9);


			vk::DescriptorImageInfo texDescriptorPosDepth =
				vkx::descriptorImageInfo(offscreen.framebuffers[0].attachments[0].sampler, offscreen.framebuffers[0].attachments[0].view, vk::ImageLayout::eShaderReadOnlyOptimal);

			vk::DescriptorImageInfo texDescriptorNorm =
				vkx::descriptorImageInfo(offscreen.framebuffers[0].attachments[0].sampler, offscreen.framebuffers[0].attachments[1].view, vk::ImageLayout::eShaderReadOnlyOptimal);


			std::vector<vk::WriteDescriptorSet> ssaoGenerateWriteDescriptorSets = {

				// Set 0: Binding 0: Fragment shader image sampler// FS Position+Depth
				// replaced with just depth
				vkx::writeDescriptorSet(
					rscs.descriptorSets->get("offscreen.ssao.generate"),
					vk::DescriptorType::eCombinedImageSampler,
					0,
					//&texDescriptorPosDepth),
					&texDescriptorPosition),
				// Set 0: Binding 1: Fragment shader image sampler// FS Normals
				vkx::writeDescriptorSet(
					rscs.descriptorSets->get("offscreen.ssao.generate"),
					vk::DescriptorType::eCombinedImageSampler,
					1,
					&texDescriptorNorm),
				// Set 0: Binding 2: Fragment shader image sampler// FS SSAO Noise
				vkx::writeDescriptorSet(
					rscs.descriptorSets->get("offscreen.ssao.generate"),
					vk::DescriptorType::eCombinedImageSampler,
					2,
					&textures.ssaoNoise.descriptor),
				// Set 0: Binding 3: Fragment shader uniform buffer// FS SSAO Kernel UBO
				vkx::writeDescriptorSet(
					rscs.descriptorSets->get("offscreen.ssao.generate"),
					vk::DescriptorType::eUniformBuffer,
					3,
					&uniformDataDeferred.ssaoKernel.descriptor),
				// Set 0: Binding 4: Fragment shader uniform buffer// FS SSAO Params UBO
				vkx::writeDescriptorSet(
					rscs.descriptorSets->get("offscreen.ssao.generate"),
					vk::DescriptorType::eUniformBuffer,
					4,
					&uniformDataDeferred.ssaoParams.descriptor),
			};
			context.device.updateDescriptorSets(ssaoGenerateWriteDescriptorSets, nullptr);
		}

		// ------------------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------------------
		// SSAO Blur


		// descriptor set 
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo10 =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("deferred"), &rscs.descriptorSetLayouts->get("offscreen.ssao.blur"), 1);
		rscs.descriptorSets->add("offscreen.ssao.blur", descriptorSetAllocateInfo10);

		vk::DescriptorImageInfo texDescriptorSSAOBlur =
			vkx::descriptorImageInfo(offscreen.framebuffers[0].attachments[0].sampler, offscreen.framebuffers[1].attachments[0].view, vk::ImageLayout::eShaderReadOnlyOptimal);



		std::vector<vk::WriteDescriptorSet> ssaoBlurWriteDescriptorSets =
		{
			// Set 0: Binding 0: Fragment shader image sampler
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("offscreen.ssao.blur"),
				vk::DescriptorType::eCombinedImageSampler,
				0,
				&texDescriptorSSAOBlur),
		};
		context.device.updateDescriptorSets(ssaoBlurWriteDescriptorSets, nullptr);




		// ------------------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------------------
		// shadow mapping:


		// descriptor set 0
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfoShadow =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("deferred"), &rscs.descriptorSetLayouts->get("shadow.scene"), 1);
		rscs.descriptorSets->add("shadow.scene", descriptorSetAllocateInfoShadow);// todo: actually make a descriptor pool for this set


		// descriptor set 1
		// matrix data
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfoShadowMatrix =
			vkx::descriptorSetAllocateInfo(rscs.descriptorPools->get("offscreen.matrix"), &rscs.descriptorSetLayouts->get("shadow.matrix"), 1);
		rscs.descriptorSets->add("shadow.matrix", descriptorSetAllocateInfoShadowMatrix);

		std::vector<vk::WriteDescriptorSet> writeDescriptorSetsShadow =
		{
			// Set 0: Binding 0: geometry shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("shadow.scene"),
				vk::DescriptorType::eUniformBuffer,
				0,
				&uniformDataDeferred.gsShadow.descriptor),

			// Set 1: Binding 0: Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				rscs.descriptorSets->get("shadow.matrix"),
				vk::DescriptorType::eUniformBufferDynamic,
				0,
				&uniformData.matrixVS.descriptor),// bind to forward descriptor since it's the same
		};
		context.device.updateDescriptorSets(writeDescriptorSetsShadow, nullptr);


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

		vk::PipelineMultisampleStateCreateInfo multisampleState = vkx::pipelineMultisampleStateCreateInfo(vk::SampleCountFlagBits::e1);// added 1/20/17

		std::vector<vk::DynamicState> dynamicStateEnables = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};
		vk::PipelineDynamicStateCreateInfo dynamicState;
		dynamicState.dynamicStateCount = dynamicStateEnables.size();
		dynamicState.pDynamicStates = dynamicStateEnables.data();


		// Load shaders
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;


		vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
		// set pipeline layout
		//pipelineCreateInfo.layout = rscs.pipelineLayouts->get("forward.basic");
		// set render pass
		pipelineCreateInfo.renderPass = renderPass;

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
		


		//// meshes:
		//shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/forward/mesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		//shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/forward/mesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//vk::Pipeline meshPipeline = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		//rscs.pipelines->add("forward.meshes", meshPipeline);



		//// skinned meshes:
		//shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/forward/skinnedMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		//shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/forward/skinnedMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//vk::Pipeline skinnedMeshPipeline = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		//rscs.pipelines->add("forward.skinnedMeshes", skinnedMeshPipeline);



		// ----------------------------------------------------------------------------------------------------------
		// ----------------------------------------------------------------------------------------------------------
		// deferred:

		// todo: fix
		// this doesn't need to be a seperate pipeline layout:

		// Separate layout
		pipelineCreateInfo.layout = rscs.pipelineLayouts->get("deferred");

		rasterizationState.cullMode = vk::CullModeFlagBits::eNone;



		// deferred quad that is blitted to:
		// not offscreen
		// fullscreen quad
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/composition.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/composition.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline deferredPipeline = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("deferred.composition", deferredPipeline);


		// fullscreen quad
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/composition.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/composition.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline deferredSSAOQuadPipeline = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("deferred.composition.ssao", deferredSSAOQuadPipeline);


		// Debug display pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/debug.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/debug.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline debugPipeline = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("deferred.debug", debugPipeline);


		// Debug display pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/debug.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/debug.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline debugPipelineSSAO = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("deferred.debug.ssao", debugPipelineSSAO);











		// -----------------------------------------------------------------------------------------------------------------------------------------
		// -----------------------------------------------------------------------------------------------------------------------------------------
		// OFFSCREEN PIPELINES:

		// Separate render pass
		pipelineCreateInfo.renderPass = offscreen.framebuffers[0].renderPass;

		// Separate layout
		pipelineCreateInfo.layout = rscs.pipelineLayouts->get("offscreen");

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

		rasterizationState.cullMode = vk::CullModeFlagBits::eBack;// added

		// Offscreen pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrtMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrtMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline deferredMeshPipeline = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("offscreen.meshes", deferredMeshPipeline);

		// Offscreen pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrtSkinnedMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/deferred/mrtSkinnedMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline deferredSkinnedMeshPipeline = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("offscreen.skinnedMeshes", deferredSkinnedMeshPipeline);

		// ssao:

		// Offscreen pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/mrtMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/mrtMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline deferredMeshSSAOPipeline = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("offscreen.meshes.ssao", deferredMeshSSAOPipeline);

		// Offscreen pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/mrtSkinnedMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/mrtSkinnedMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		vk::Pipeline deferredSkinnedMeshSSAOPipeline = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("offscreen.skinnedMeshes.ssao", deferredSkinnedMeshSSAOPipeline);



		// -----------------------------------------------------------------------------------------------------------------------------------
		// SSAO


		colorBlendState.attachmentCount = 1;

		vk::PipelineVertexInputStateCreateInfo emptyInputState;
		emptyInputState.vertexAttributeDescriptionCount = 0;
		emptyInputState.pVertexAttributeDescriptions = nullptr;
		emptyInputState.vertexBindingDescriptionCount = 0;
		emptyInputState.pVertexBindingDescriptions = nullptr;
		pipelineCreateInfo.pVertexInputState = &emptyInputState;

		// SSAO Generate pass
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/fullscreen.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/ssao.frag.spv", vk::ShaderStageFlagBits::eFragment);
		{
			//// Set constant parameters via specialization constants
			//struct SpecializationData {
			//	uint32_t kernelSize = SSAO_KERNEL_SIZE;
			//	float radius = SSAO_RADIUS;
			//	float power = 1.5f;
			//} specializationData;

			//std::vector<VkSpecializationMapEntry> specializationMapEntries;
			//specializationMapEntries = {
			//	vkTools::initializers::specializationMapEntry(0, offsetof(SpecializationData, kernelSize), sizeof(uint32_t)),	// SSAO Kernel size
			//	vkTools::initializers::specializationMapEntry(1, offsetof(SpecializationData, radius), sizeof(float)),			// SSAO radius
			//	vkTools::initializers::specializationMapEntry(2, offsetof(SpecializationData, power), sizeof(float)),			// SSAO power
			//};

			//VkSpecializationInfo specializationInfo = vkTools::initializers::specializationInfo(specializationMapEntries.size(), specializationMapEntries.data(), sizeof(specializationData), &specializationData);
			//shaderStages[1].pSpecializationInfo = &specializationInfo;

			pipelineCreateInfo.renderPass = offscreen.framebuffers[1].renderPass;// SSAO Generate render pass
			pipelineCreateInfo.layout = rscs.pipelineLayouts->get("offscreen.ssaoGenerate");

			vk::Pipeline ssaoGenerate = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
			rscs.pipelines->add("ssao.generate", ssaoGenerate);
		}

		// SSAO blur pass
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/fullscreen.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/blur.frag.spv", vk::ShaderStageFlagBits::eFragment);

		pipelineCreateInfo.renderPass = offscreen.framebuffers[2].renderPass;// SSAO Blur render pass
		pipelineCreateInfo.layout = rscs.pipelineLayouts->get("offscreen.ssaoBlur");

		vk::Pipeline ssaoBlur = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
		rscs.pipelines->add("ssao.blur", ssaoBlur);






		// change vertex input state back to what it was:
		pipelineCreateInfo.pVertexInputState = &vertices.inputState;// important

		// seperate pipeline layout:
		pipelineCreateInfo.layout = rscs.pipelineLayouts->get("offscreen.shadow");


		{

			// Shadow mapping pipeline
			// The shadow mapping pipeline uses geometry shader instancing (invocations layout modifier) to output 
			// shadow maps for multiple lights sources into the different shadow map layers in one single render pass
			std::array<vk::PipelineShaderStageCreateInfo, 3> shadowStages;
			shadowStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/shadow.vert.spv", vk::ShaderStageFlagBits::eVertex);
			shadowStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/shadow.frag.spv", vk::ShaderStageFlagBits::eFragment);
			shadowStages[2] = context.loadShader(getAssetPath() + "shaders/vulkanscene/ssao/shadow.geom.spv", vk::ShaderStageFlagBits::eGeometry);

			pipelineCreateInfo.pStages = shadowStages.data();
			pipelineCreateInfo.stageCount = static_cast<uint32_t>(shadowStages.size());

			// Shadow pass doesn't use any color attachments
			colorBlendState.attachmentCount = 0;
			colorBlendState.pAttachments = nullptr;

			// Cull front faces
			rasterizationState.cullMode = vk::CullModeFlagBits::eFront;
			//rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
			depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;

			// Enable depth bias
			rasterizationState.depthBiasEnable = VK_TRUE;
			// Add depth bias to dynamic state, so we can change it at runtime
			dynamicStateEnables.push_back(vk::DynamicState::eDepthBias);
			dynamicState = vkx::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				static_cast<uint32_t>(dynamicStateEnables.size()));

			// Reset blend attachment state
			pipelineCreateInfo.renderPass = offscreen.framebuffers[3].renderPass;

			vk::Pipeline shadowPipeline = context.device.createGraphicsPipeline(context.pipelineCache, pipelineCreateInfo, nullptr);
			rscs.pipelines->add("shadow", shadowPipeline);

		}



	}


	void prepareDeferredPipelines() {

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
		//uboScene.cameraPos = glm::vec4(camera.transform.translation, 0.0f);
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

		if (materialNodes.size() != this->assetManager.materials.resources.size()) {
			if (this->assetManager.materials.resources.size() == 0) {
				vkx::MaterialProperties p;
				p.ambient = glm::vec4();
				p.diffuse = glm::vec4();
				p.specular = glm::vec4();
				p.opacity = 1.0f;
				materialNodes[0] = p;
			} else {
				materialNodes.resize(this->assetManager.materials.resources.size());
			}
		}



		for (auto &iterator : this->assetManager.materials.resources) {
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




		// ssao
		uniformDataDeferred.ssaoParams = context.createUniformBuffer(uboSSAOParams);
		uniformDataDeferred.ssaoKernel = context.createUniformBuffer(uboSSAOKernel);




		// shadow mapping
		uniformDataDeferred.gsShadow = context.createUniformBuffer(uboShadowGS);



		// Update uniform buffers:

		// offscreen:
		updateUniformBuffersScreen();
		updateSceneBufferDeferred();
		updateMatrixBufferDeferred();

		initLights();
		updateUniformBufferDeferredLights();

		// ssao:
		updateUniformBufferSSAOParams();
		updateUniformBufferSSAOKernel();

		// shadow mapping:
		updateUniformBufferShadow();

	}

	void updateUniformBuffersScreen() {
		if (debugDisplay) {
			uboVS.projection = glm::ortho(0.0f, 2.0f, 0.0f, 2.0f, -1.0f, 1.0f);
		} else {
			uboVS.projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
		}
		uboVS.model = glm::mat4();
		//uboVS.camPos = glm::vec4(camera.transform.translation, 1.0);// added

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


	SpotLight initLight(glm::vec3 pos, glm::vec3 target, glm::vec3 color) {
		SpotLight light;
		light.position = glm::vec4(pos, 1.0f);
		light.target = glm::vec4(target, 0.0f);
		light.color = glm::vec4(color, 0.0f);
		return light;
	}

	void initLights() {
		uboFSLights.spotlights[0] = initLight(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		uboFSLights.spotlights[1] = initLight(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		//uboFSLights.spotlights[0].position = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);
		//uboFSLights.spotlights[0].color = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

		//uboFSLights.spotlights[1].position = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);
		//uboFSLights.spotlights[1].color = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

		//uboFSLights.spotlights[4000].color = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);


	}


	glm::mat4 calculateFrustum(glm::vec3 lightDir, float zMin, float zMax, float mainNear, float mainFar) {
		float projWidth = settings.windowSize.width;
		float projHeight = settings.windowSize.height;
		float projFov = 80.0f;

		//float mainNear = 0.1f;
		//float mainFar = 256.0f;

		//float minZ = 25.0f;
		//float maxZ = 90.0f;



		// Get the inverse of the view transform
		//p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());
		//glm::mat4 Cam = p.GetViewTrans();
		//glm::mat4 CamInv = Cam.Inverse();

		glm::vec3 dir2 = glm::normalize(camera.transform.orientation*glm::vec3(0.0, 0.0, -1.0));
		glm::mat4 test = glm::lookAt(
			camera.transform.translation, // camera position
			camera.transform.translation - dir2, // point to look at
			glm::vec3(0, 0, 1)  // up vector
		);


		//glm::mat4 CamInv = glm::inverse(camera.matrices.view);
		//glm::mat4 CamInv = camera.matrices.view;

		//glm::mat4 CamInv = test;
		glm::mat4 CamInv = glm::inverse(test);

		// Get the light space tranform
		//p.SetCamera(glm::vec3(0.0f, 0.0f, 0.0f), m_dirLight.Direction, glm::vec3(0.0f, 1.0f, 0.0f));
		//glm::mat4 LightM = p.GetViewTrans();

		glm::mat4 LightM = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), lightDir, glm::vec3(0.0f, 0.0f, 1.0f));
		//glm::mat4 LightM = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f)-lightDir, glm::vec3(0.0f, 0.0f, 1.0f));

		float ar = projHeight / projWidth;
		float tanHalfHFOV = tanf(glm::radians(projFov / 2.0f));
		float tanHalfVFOV = tanf(glm::radians((projFov * ar) / 2.0f));


		float xn = zMin * tanHalfHFOV;
		float xf = zMax * tanHalfHFOV;
		float yn = zMin * tanHalfVFOV;
		float yf = zMax * tanHalfVFOV;

		glm::vec4 frustumCorners[/*NUM_FRUSTUM_CORNERS*/8] = {
			// near face
			glm::vec4(xn, yn, zMin, 1.0),
			glm::vec4(-xn, yn, zMin, 1.0),
			glm::vec4(xn, -yn, zMin, 1.0),
			glm::vec4(-xn, -yn, zMin, 1.0),

			// far face
			glm::vec4(xf, yf, zMax, 1.0),
			glm::vec4(-xf, yf, zMax, 1.0),
			glm::vec4(xf, -yf, zMax, 1.0),
			glm::vec4(-xf, -yf, zMax, 1.0)
		};




		glm::vec4 frustumCornersL[/*NUM_FRUSTUM_CORNERS*/8];

		float minX = 999.0f;//std::numeric_limits::max();
		float maxX = -999.0f;//std::numeric_limits::min();
		float minY = 999.0f;//std::numeric_limits::max();
		float maxY = -999.0f;//std::numeric_limits::min();
		float minZ = 999.0f;//std::numeric_limits::max();
		float maxZ = -999.0f;//std::numeric_limits::min();

		for (int j = 0; j < /*NUM_FRUSTUM_CORNERS*/8; j++) {

			// Transform the frustum coordinate from view to world space
			glm::vec4 vW = CamInv * frustumCorners[j];

			// Transform the frustum coordinate from world to light space
			frustumCornersL[j] = LightM * vW;

			minX = std::min(minX, frustumCornersL[j].x);
			maxX = std::max(maxX, frustumCornersL[j].x);
			minY = std::min(minY, frustumCornersL[j].y);
			maxY = std::max(maxY, frustumCornersL[j].y);
			minZ = std::min(minZ, frustumCornersL[j].z);
			maxZ = std::max(maxZ, frustumCornersL[j].z);
		}

		//m_shadowOrthoProjInfo[i].r = maxX;
		//m_shadowOrthoProjInfo[i].l = minX;
		//m_shadowOrthoProjInfo[i].b = minY;
		//m_shadowOrthoProjInfo[i].t = maxY;
		//m_shadowOrthoProjInfo[i].f = maxZ;
		//m_shadowOrthoProjInfo[i].n = minZ;
		
		// offset since the bounding box isn't perfect?
		float offsetScale = 1.1f;
		//glm::mat4 proj = glm::ortho(-size, size, -size, size, -30.0f, 30.0f);

		//glm::mat4 proj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
		//glm::mat4 proj = glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);
		glm::mat4 proj = glm::ortho(minX*offsetScale, maxX*offsetScale, minY*offsetScale, maxY*offsetScale, -30.0f, 30.0f);

		return proj;

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
				//float z = (10.0f) + (sin((0.5*globalP) + n)*2.0f);
				float z = (10.0f) + 10 * (sin((2.5*globalP) + n)*2.0f);

				uboFSLights.pointlights[n].position = glm::vec4(x, y, z, 0.0f);
				uboFSLights.pointlights[n].color = glm::vec4((i * 2) - 3.0f, i, j, 0.0f) * glm::vec4(2.5f);
				uboFSLights.pointlights[n].radius = 2.0f;
				uboFSLights.pointlights[n].linearFalloff = 0.2f;
				uboFSLights.pointlights[n].quadraticFalloff = 0.2f;

				// increment counter
				n++;
			}
		}

		//uboFSLights.spotlights[0].target = glm::vec4(cos(globalP*8.0f)*9.0f, sin(globalP*8.0f)*4.0f, 0.0f, 0.0f);

		uboFSLights.spotlights[0].target = glm::vec4(cos(globalP*4.0f)*1.0f, sin(globalP*4.0f)*1.0f, 0.0f, 0.0f);
		uboFSLights.spotlights[1].target = glm::vec4(cos(-globalP*8.0f)*1.0f, sin(-globalP*8.0f)*1.0f, 0.0f, 0.0f);

		// r = 8sin(2t)
		// x = rcos(t)
		// y = rsin(t)
		// x = 8sin(2*globalP)*cos(globalP)
		// y = 8sin(2*globalP)*sin(globalP)

		uboFSLights.spotlights[1].target = glm::vec4(1*sin(4 * globalP)*cos(globalP), 1*sin(4 * globalP)*sin(globalP), 0.0f, 0.0f);


		//uboFSLights.directionalLights[0].direction = glm::normalize(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) - glm::vec4(camera.transform.translation, 0.0f));
		uboFSLights.directionalLights[0].color = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)*0.04f;
		//uboFSLights.directionalLights[0].direction = glm::normalize(glm::vec4(camera.transform.translation, 0.0f) - glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		//uboFSLights.directionalLights[0].direction = glm::vec4(0.5f, 0.0f, -0.5f, 0.0f);

		uboFSLights.directionalLights[0].pad1 = globalP;
		uboFSLights.directionalLights[0].pad2 = globalP;


		// csm lights:
		//uboFSLights.csmlights[0].color = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)*0.04f;

		// spot lights:
		for (uint32_t i = 0; i < NUM_SPOT_LIGHTS; i++) {

			SpotLight &light = uboFSLights.spotlights[i];


			// mvp from light's pov (for shadows)
			glm::mat4 shadowProj = glm::perspectiveRH(glm::radians(light.innerAngle), 1.0f, light.zNear, light.zFar);
			shadowProj[1][1] *= -1;// because glm produces matrix for opengl and this is vulkan

			glm::mat4 shadowView = glm::lookAt(glm::vec3(light.position), glm::vec3(light.target), glm::vec3(0.0f, 0.0f, 1.0f));

			glm::mat4 shadowModel = glm::mat4();

			uboShadowGS.spotlightMVP[i] = shadowProj * shadowView * shadowModel;
			light.viewMatrix = uboShadowGS.spotlightMVP[i];
		}



		//// directional lights:
		//for (uint32_t i = 0; i < NUM_DIR_LIGHTS; i++) {

		//	DirectionalLight &light = uboFSLights.directionalLights[i];

		//	glm::mat4 shadowProj = glm::ortho(-light.size, light.size, -light.size, light.size, light.zNear, light.zFar);
		//	shadowProj[1][1] *= -1;// because glm produces matrix for opengl and this is vulkan

		//	glm::mat4 shadowView = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(light.direction), glm::vec3(0.0f, 0.0f, 1.0f));

		//	//glm::mat4 shadowProj = camera.matrices.projection;
		//	//glm::mat4 shadowView = camera.matrices.view;


		//	glm::mat4 shadowModel = glm::mat4();

		//	uboShadowGS.dirlightMVP[i] = shadowProj * shadowView * shadowModel;
		//	light.viewMatrix = uboShadowGS.dirlightMVP[i];
		//}



		float numOfSplits = 3;
		

		float mainNear = 0.1f;
		float mainFar = 256.0f;


		//const float splitConstant = 0.95f;
		//for (int i = 1; i < numOfSplits; i++) {
		//	splitDepths[i] = splitConstant * mainNear * (float)pow(mainFar / mainNear, i / numOfSplits) + (1.0f - splitConstant) * ((mainNear + (i / numOfSplits)) * (mainFar - mainNear));
		//}


		// directional lights:
		for (uint32_t i = 0; i < NUM_DIR_LIGHTS; i++) {

			DirectionalLight &light = uboFSLights.directionalLights[i];

			//glm::mat4 shadowProj = glm::ortho(-light.size, light.size, -light.size, light.size, light.zNear, light.zFar);
			glm::mat4 shadowProj = calculateFrustum(glm::vec3(light.direction), temporary.splitDepths[i], temporary.splitDepths[i+1], mainNear, mainFar);
			shadowProj[1][1] *= -1;// because glm produces matrix for opengl and this is vulkan

			glm::mat4 shadowView = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(light.direction), glm::vec3(0.0f, 0.0f, 1.0f));


			glm::mat4 shadowModel = glm::mat4();

			//uboShadowGS.csmlightMVP[i] = shadowProj * shadowView * shadowModel;
			//light.viewMatrix = uboShadowGS.csmlightMVP[i];

			uboShadowGS.dirlightMVP[i] = shadowProj * shadowView * shadowModel;
			light.viewMatrix = uboShadowGS.dirlightMVP[i];

			light.cascadeNear = temporary.splitDepths[i];
			light.cascadeFar = temporary.splitDepths[i + 1];

		}






























		// test:
		//uboFSLights.spotlights[0].viewMatrix = camera.matrices.projection * camera.matrices.view;
		//uboShadowGS.spotlightMVP[0] = camera.matrices.projection * camera.matrices.view;

		updateUniformBufferShadow();

		// Current view position
		uboFSLights.viewPos = glm::vec4(camera.transform.translation, 0.0f) * glm::vec4(-1.0f);

		uboFSLights.view = camera.matrices.view;
		uboFSLights.model = glm::mat4();
		
		// just for testing:
		//uboFSLights.model = glm::inverse(uboVS.projection);


		
		uboFSLights.projection = camera.matrices.projection;// new

		uboFSLights.invViewProj = glm::inverse(camera.matrices.projection * camera.matrices.view);// new


		uniformDataDeferred.fsLights.copy(uboFSLights);
	}

	void updateUniformBufferSSAOParams() {
		uboSSAOParams.projection = camera.matrices.projection;
		uboSSAOParams.view = camera.matrices.view;
		uniformDataDeferred.ssaoParams.copy(uboSSAOParams);
	}

	inline float lerp(float a, float b, float f) {
		return a + f * (b - a);
	}

	void updateUniformBufferSSAOKernel() {

		// https://learnopengl.com/#!Advanced-Lighting/SSAO

		std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
		std::random_device rndDev;
		std::default_random_engine rndGen;

		// Sample kernel
		std::vector<glm::vec4> ssaoKernel(SSAO_KERNEL_SIZE);
		//std::vector<glm::vec4> ssaoKernel;

		for (uint32_t i = 0; i < SSAO_KERNEL_SIZE; ++i) {

			glm::vec3 sample(
				rndDist(rndGen) * 2.0 - 1.0,// -1.0 - 1.0// X
				rndDist(rndGen) * 2.0 - 1.0,// -1.0 - 1.0// Y
				rndDist(rndGen)//				0.0 - 1.0// Z
			);

			sample = glm::normalize(sample);

			sample *= rndDist(rndGen);
			float scale = float(i) / float(SSAO_KERNEL_SIZE);
			scale = lerp(0.1f, 1.0f, scale * scale);// todo: fix

			ssaoKernel[i] = glm::vec4(sample * scale, 0.0f);
			//ssaoKernel.push_back(glm::vec4(sample * scale, 0.0f));
		}

		for (uint32_t i = 0; i < SSAO_KERNEL_SIZE; ++i) {
			uboSSAOKernel.samples[i] = ssaoKernel[i];
		}

		uniformDataDeferred.ssaoKernel.copy(uboSSAOKernel);


		// todo: fix

		// Random noise
		std::vector<glm::vec4> ssaoNoise(SSAO_NOISE_DIM * SSAO_NOISE_DIM);

		for (uint32_t i = 0; i < static_cast<uint32_t>(ssaoNoise.size()); i++) {

			ssaoNoise[i] = glm::vec4(
				rndDist(rndGen) * 2.0f - 1.0f,// -1.0 - 1.0// X
				rndDist(rndGen) * 2.0f - 1.0f,// -1.0 - 1.0// Y
				0.0f,
				0.0f);

		}

		// Upload as texture
		textureLoader->createTexture(ssaoNoise.data(), ssaoNoise.size() * sizeof(glm::vec4), vk::Format::eR32G32B32A32Sfloat, SSAO_NOISE_DIM, SSAO_NOISE_DIM, &textures.ssaoNoise, vk::Filter::eNearest);

	}



	void updateUniformBufferShadow() {
		uniformDataDeferred.gsShadow.copy(uboShadowGS);
	}


	void toggleDebugDisplay() {
		debugDisplay = !debugDisplay;

		if (TEST_DEFINE) {
			updateDrawCommandBuffers();
		} else {
			buildDrawCommandBuffers();
		}

		buildOffscreenCommandBuffer();
		updateUniformBuffersScreen();
	}



	// temporary:
	void createDomino(glm::vec3 pos, float angle) {
		auto dominoModel = std::make_shared<vkx::Model>(&context, &assetManager);
		dominoModel->load(getAssetPath() + "models/domino3.fbx");
		dominoModel->createMeshes(SSAOVertexLayout, 0.125f, VERTEX_BUFFER_BIND_ID);

		modelsDeferred.push_back(dominoModel);


		auto physicsDomino = std::make_shared<vkx::PhysicsObject>(&physicsManager, dominoModel);

		btCollisionShape* dominoShape = new btBoxShape(btVector3(1.0/8, 0.3/8, 1.9/8));
		physicsDomino->createRigidBody(dominoShape, 2.5f);



		btTransform tr;
		tr.setIdentity();
		tr.setOrigin(btVector3(pos.x, pos.y, pos.z));

		btQuaternion quat;
		quat.setEulerZYX(angle, 0.0, 0.0); //or quat.setEulerZYX depending on the ordering you want
		tr.setRotation(quat);

		physicsDomino->rigidBody->setCenterOfMassTransform(tr);


		//physicsDomino->rigidBody->activate();
		//physicsDomino->rigidBody->translate(btVector3(rnd(-10, 10), rnd(-10, 10), 10.));
		physicsObjects.push_back(physicsDomino);

		updateOffscreen = true;
	}





	void start() {

		toggleDebugDisplay();

		// add plane model
		auto planeModel = std::make_shared<vkx::Model>(&context, &assetManager);
		planeModel->load(getAssetPath() + "models/plane.fbx");
		planeModel->createMeshes(SSAOVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
		models.push_back(planeModel);

		auto physicsPlane = std::make_shared<vkx::PhysicsObject>(&physicsManager, planeModel);
		//btCollisionShape* boxShape = new btBoxShape(btVector3(btScalar(200.), btScalar(200.), btScalar(0.01)));
		btCollisionShape* planeShape = new btStaticPlaneShape(btVector3(0.0, 0.0, 1.0), 0.0);
		physicsPlane->createRigidBody(planeShape, 0.0f);
		//btTransform t;
		//t.setOrigin(btVector3(0., 0., 0.));
		//physicsPlane->rigidBody->setWorldTransform(t);
		physicsObjects.push_back(physicsPlane);




		//for (int i = 0; i < 2; ++i) {
		//	auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
		//	testModel->load(getAssetPath() + "models/cube.fbx");
		//	testModel->createMeshes(SSAOVertexLayout, 0.5f, VERTEX_BUFFER_BIND_ID);

		//	models.push_back(testModel);
		//}


		//for (int i = 0; i < 1; ++i) {

		//	auto testSkinnedMesh = std::make_shared<vkx::SkinnedMesh>(&context, &assetManager);
		//	testSkinnedMesh->load(getAssetPath() + "models/goblin.dae");
		//	testSkinnedMesh->createSkinnedMeshBuffer(SSAOVertexLayout, 0.0005f);

		//	skinnedMeshes.push_back(testSkinnedMesh);
		//}




		//auto physicsBall = std::make_shared<vkx::PhysicsObject>(&physicsManager, models[1]);
		//btCollisionShape* sphereShape = new btSphereShape(btScalar(1.));
		//physicsBall->createRigidBody(sphereShape, 1.0f);
		//btTransform t2;
		//t2.setOrigin(btVector3(0., 0., 10.));
		//physicsBall->rigidBody->setWorldTransform(t2);
		//physicsObjects.push_back(physicsBall);






		// deferred

		if (!false) {
			auto sponzaModel = std::make_shared<vkx::Model>(&context, &assetManager);
			sponzaModel->load(getAssetPath() + "models/sponza.dae");
			sponzaModel->createMeshes(SSAOVertexLayout, 0.08f, VERTEX_BUFFER_BIND_ID);//0.3
			sponzaModel->rotateWorldX(PI / 2.0);
			sponzaModel->rotateWorldZ(PI / 2.0);
			//sponzaModel->rotateWorldX(glm::radians(90.0f));
			modelsDeferred.push_back(sponzaModel);
		}

		if (false) {
			auto sibModel = std::make_shared<vkx::Model>(&context, &assetManager);
			sibModel->load(getAssetPath() + "models/sibenik/sibenik.dae");
			sibModel->createMeshes(SSAOVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
			sibModel->rotateWorldX(PI / 2.0);
			modelsDeferred.push_back(sibModel);
		}


		//autoModel = std::make_shared<vkx::Model>(&context, &assetManager);
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


		for (int i = 0; i < 2; ++i) {
			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/monkey.fbx");
			testModel->createMeshes(SSAOVertexLayout, 0.0f, VERTEX_BUFFER_BIND_ID);

			modelsDeferred.push_back(testModel);
		}


		for (int i = 0; i < 1; ++i) {

			auto testSkinnedMesh = std::make_shared<vkx::SkinnedMesh>(&context, &assetManager);
			testSkinnedMesh->load(getAssetPath() + "models/goblin.dae");// breaks size?
			testSkinnedMesh->createSkinnedMeshBuffer(SSAOVertexLayout, 0.000005f);
			//todo: figure out why there must be atleast one deferred skinned mesh here
			//inorder to not cause problems
			//fixed?
			skinnedMeshesDeferred.push_back(testSkinnedMesh);
		}


		//for (int i = 0; i < 10; ++i) {
		//	auto boxModel = std::make_shared<vkx::Model>(&context, &assetManager);
		//	boxModel->load(getAssetPath() + "models/myCube.dae");
		//	boxModel->createMeshes(SSAOVertexLayout, 0.1f, VERTEX_BUFFER_BIND_ID);
		//	//boxModel->rotateWorldX(PI / 2.0);
		//	modelsDeferred.push_back(boxModel);
		//	temporary.modelsDeferred.push_back(boxModel);
		//}




		//auto floorModel = std::make_shared<vkx::Model>(&context, &assetManager);
		//floorModel->load(getAssetPath() + "models/plane.fbx");
		//floorModel->createMeshes(SSAOVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
		//modelsDeferred.push_back(floorModel);



		//auto skyboxModel = std::make_shared<vkx::Model>(&context, &assetManager);
		//skyboxModel->load(getAssetPath() + "models/myCube.dae");
		//skyboxModel->createMeshes(SSAOVertexLayout, 10.0f, VERTEX_BUFFER_BIND_ID);
		//modelsDeferred.push_back(skyboxModel);



		//auto physicsWall1 = std::make_shared<vkx::PhysicsObject>(&physicsManager, wallModel1);
		//btCollisionShape *wallShape1 = new btBoxShape(btVector3(btScalar(1.0), btScalar(1.), btScalar(0.1)));
		//physicsWall1->createRigidBody(wallShape1, 0.0f);
		//btTransform t1;
		//t1.setOrigin(btVector3(0., 0., 4.));
		//physicsWall1->rigidBody->setWorldTransform(t1);
		//physicsObjects.push_back(physicsWall1);

		for (int i = 0; i < 30; ++i) {

			float xFreq = 0.4;
			float Xamplitude = 0.5;

			float dSpacing = 0.3;

			float x = sin(i*xFreq) * Xamplitude;
			float y = (i*dSpacing)-2.0;
			float z = 0.3;

			float angle = /*glm::radians(90.0)*/-cos(i*xFreq);

			createDomino(glm::vec3(x, y, z), angle);
		}








		// after any loading with materials has occurred, updateMaterialBuffer() must be called
		// to update texture descriptor sets and sync
		updateMaterialBuffer();

	}









	void updateWorld() {



		updateDraw = false;
		updateOffscreen = false;


		camera.movementSpeed = 0.0012f;

		camera.movementSpeed = camera.movementSpeed*deltaTime*1000.0;


		if (keyStates.onKeyDown(&keyStates.o)) {
			GUIOpen = !GUIOpen;
			
		}

		if (keyStates.onKeyDown(&keyStates.j)) {
			testBool = !testBool;
			printf("Size of UBOFS: %d\n", sizeof(uboFSLights));
		}

		//if (GUIOpen) {
		//	goto endofcontrols;
		//}



		if (keyStates.shift) {
			camera.movementSpeed *= 2;
		}

		// z-up translations
		if (keyStates.w) {
			camera.translateLocal(glm::vec3(0.0f, 0.0f, -camera.movementSpeed));
		}
		if (keyStates.s) {
			camera.translateLocal(glm::vec3(0.0f, 0.0f, camera.movementSpeed));
		}
		if (keyStates.a) {
			camera.translateLocal(glm::vec3(-camera.movementSpeed, 0.0f, 0.0f));
		}
		if (keyStates.d) {
			camera.translateLocal(glm::vec3(camera.movementSpeed, 0.0f, 0.0f));
		}
		if (keyStates.q) {
			camera.translateLocal(glm::vec3(0.0f, -camera.movementSpeed, 0.0f));
		}
		if (keyStates.e) {
			camera.translateLocal(glm::vec3(0.0f, camera.movementSpeed, 0.0f));
		}



		// z-up rotations
		camera.rotationSpeed = -0.25f;
		camera.rotationSpeed = camera.rotationSpeed*deltaTime;

		//if (mouse.leftMouseButton.state) {
		if (mouse.leftMouseButton.state && !GUIOpen) {

			camera.rotateWorldZ(mouse.delta.x*camera.rotationSpeed);
			camera.rotateLocalX(mouse.delta.y*camera.rotationSpeed);

			if (!camera.isFirstPerson) {
				camera.sphericalCoords.theta += mouse.delta.x*camera.rotationSpeed;
				camera.sphericalCoords.phi -= mouse.delta.y*camera.rotationSpeed;
			}


			//SDL_SetRelativeMouseMode((SDL_bool)1);
		} else {
			//bool isCursorLocked = (bool)SDL_GetRelativeMouseMode();
			//if (isCursorLocked) {
			//	SDL_SetRelativeMouseMode((SDL_bool)0);
			//	SDL_WarpMouseInWindow(this->SDLWindow, mouse.leftMouseButton.pressedCoords.x, mouse.leftMouseButton.pressedCoords.y);
			//}
		}


		camera.rotationSpeed = -0.002f;
		camera.rotationSpeed = camera.rotationSpeed*deltaTime*1000.0;


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
		//	camera.isFirstPerson = !camera.isFirstPerson;
		//}

		if (keyStates.onKeyDown(&keyStates.r)) {
			toggleDebugDisplay();
		}

		if (keyStates.p) {
			updateDraw = true;
			updateOffscreen = true;
		}

		if (keyStates.onKeyDown(&keyStates.y)) {
			fullDeferred = !fullDeferred;
			updateDraw = true;
			updateOffscreen = true;

		}





		if (keyStates.space) {

			float scale = 0.1f;

			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/monkey.fbx");
			testModel->createMeshes(SSAOVertexLayout, scale, VERTEX_BUFFER_BIND_ID);
			//testModel->loadAndCreateMeshes(getAssetPath() + "models/monkey.fbx", SSAOVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
			modelsDeferred.push_back(testModel);

			auto physicsBall = std::make_shared<vkx::PhysicsObject>(&physicsManager, testModel);
			btConvexHullShape *convexHullShape = createConvexHullFromMesh(testModel->meshLoader, scale);
			physicsBall->createRigidBody(convexHullShape, 1.0f);
			physicsBall->rigidBody->activate();
			physicsBall->rigidBody->translate(btVector3(0., 0., 3.));
			physicsObjects.push_back(physicsBall);

			updateOffscreen = true;
		}

		if (keyStates.i) {
			//auto dominoModel = std::make_shared<vkx::Model>(&context, &assetManager);
			//dominoModel->load(getAssetPath() + "models/myCube.dae");
			//dominoModel->createMeshes(SSAOVertexLayout, 0.45f, VERTEX_BUFFER_BIND_ID);

			//modelsDeferred.push_back(dominoModel);


			//auto physicsDomino = std::make_shared<vkx::PhysicsObject>(&physicsManager, dominoModel);

			//btCollisionShape* dominoShape = new btBoxShape(btVector3(0.5, 0.1, 1.0));
			//physicsDomino->createRigidBody(dominoShape, 1.0f);


			//physicsDomino->rigidBody->activate();
			//physicsDomino->rigidBody->translate(btVector3(rnd(-10, 10), rnd(-10, 10), 10.));
			//physicsObjects.push_back(physicsDomino);

			//updateOffscreen = true;

			createDomino(glm::vec3(rnd(-10, 10), rnd(-2, 2), 3.), rnd(0.0, 90.0));
		}


		if (keyStates.b) {

			float scale = 0.1f;

			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/myCube.dae");
			testModel->createMeshes(SSAOVertexLayout, scale, VERTEX_BUFFER_BIND_ID);

			//testModel->loadAndCreateMeshes(getAssetPath() + "models/myCube.dae", SSAOVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

			modelsDeferred.push_back(testModel);


			auto physicsBall = std::make_shared<vkx::PhysicsObject>(&physicsManager, testModel);
			btConvexHullShape *convexHullShape = createConvexHullFromMesh(testModel->meshLoader, scale);
			physicsBall->createRigidBody(convexHullShape, 1.0f);
			physicsBall->rigidBody->activate();
			physicsBall->rigidBody->translate(btVector3(rnd(-2, 2), rnd(-2, 2), 3.));
			physicsObjects.push_back(physicsBall);

			//updateMaterialBuffer();


			updateOffscreen = true;
		}

		if (keyStates.f) {

			float scale = 0.1f;

			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/sphere.dae");
			testModel->createMeshes(SSAOVertexLayout, scale, VERTEX_BUFFER_BIND_ID);
			modelsDeferred.push_back(testModel);



			auto physicsBall = std::make_shared<vkx::PhysicsObject>(&physicsManager, testModel);
			btConvexHullShape *convexHullShape = createConvexHullFromMesh(testModel->meshLoader, scale);
			physicsBall->createRigidBody(convexHullShape, 1.0f);
			physicsBall->rigidBody->activate();
			physicsBall->rigidBody->translate(btVector3(0., 0., 3.));
			physicsObjects.push_back(physicsBall);




			updateOffscreen = true;
		}




		if (keyStates.m) {
			if (modelsDeferred.size() > 3) {
				//modelsDeferred[modelsDeferred.size() - 1]->destroy();
				modelsDeferred.pop_back();
				updateDraw = true;// probably not necessary here
				updateOffscreen = true;
			}
		}

		if (keyStates.n) {
			if (physicsObjects.size() > 3) {
				physicsObjects[physicsObjects.size() - 1]->destroy();
				physicsObjects.pop_back();
				updateOffscreen = true;
			}
		}

		if (keyStates.onKeyDown(&keyStates.l)) {
			settings.SSAO = !settings.SSAO;
			updateOffscreen = true;
			updateDraw = true;
		}


		if (!camera.isFirstPerson) {
			camera.followOpts.point = models[1]->transform.translation;
		}


		//if (keyStates.i) {
		//	physicsObjects[1]->rigidBody->activate();
		//	physicsObjects[1]->rigidBody->applyCentralForce(btVector3(0.0f, 0.1f, 0.0f));
		//}



		if (keyStates.minus) {
			settings.fpsCap -= 0.2f;
			//settings.frameTimeCapMS += 0.2f;
		} else if (keyStates.equals) {
			settings.fpsCap += 0.2f;
			//settings.frameTimeCapMS -= 0.2f;
		}


		// just to load async models:
		if (runningTimeMS > 4000.0f && runningTimeMS < 4100.0f) {
			updateDraw = true;
			updateOffscreen = true;
		}


		if (keyStates.onKeyDown(&mouse.rightMouseButton.state)) {

			glm::vec3 ray_end = generateRay();
			btVector3 rayFromWorld = btVector3(camera.transform.translation.x, camera.transform.translation.y, camera.transform.translation.z);
			btVector3 rayToWorld = btVector3(ray_end.x, ray_end.y, ray_end.z);

			btCollisionWorld::ClosestRayResultCallback rayCallback(rayFromWorld, rayToWorld);
			this->physicsManager.dynamicsWorld->rayTest(rayFromWorld, rayToWorld, rayCallback);
			if (rayCallback.hasHit()) {
				btVector3 pickPos = rayCallback.m_hitPointWorld;
				btRigidBody* body = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject);

				if (body) {

					if (!(body->isStaticObject() || body->isKinematicObject())) {
						m_pickedBody = body;
						m_savedState = m_pickedBody->getActivationState();
						m_pickedBody->setActivationState(DISABLE_DEACTIVATION);

						btVector3 localPivot = body->getCenterOfMassTransform().inverse() * pickPos;
						btPoint2PointConstraint* p2p = new btPoint2PointConstraint(*body, localPivot);
						this->physicsManager.dynamicsWorld->addConstraint(p2p, true);
						m_pickedConstraint = p2p;

						btScalar mousePickClamping = 30.f;
						p2p->m_setting.m_impulseClamp = mousePickClamping;
						p2p->m_setting.m_tau = 0.001f;
					}
					rayPicking = true;
				}
				m_oldPickingPos = rayToWorld;
				auto m_hitPos = pickPos;
				m_oldPickingDist = (pickPos - rayFromWorld).length();
				//rayPicking = true;
			}
		}



		if (rayPicking == true) {

			glm::vec3 ray_end = generateRay();
			btVector3 rayFromWorld = btVector3(camera.transform.translation.x, camera.transform.translation.y, camera.transform.translation.z);
			btVector3 rayToWorld = btVector3(ray_end.x, ray_end.y, ray_end.z);

			if (m_pickedBody  && m_pickedConstraint) {
				btPoint2PointConstraint* pickCon = static_cast<btPoint2PointConstraint*>(m_pickedConstraint);
				if (pickCon) {
					btVector3 newPivotB;
					btVector3 dir = rayToWorld - rayFromWorld;
					dir.normalize();
					dir *= m_oldPickingDist;
					newPivotB = rayFromWorld + dir;
					pickCon->setPivotB(newPivotB);
				}
			}

		}

		if (keyStates.onKeyUp(&mouse.rightMouseButton.state)) {
			rayPicking = false;

			if (m_pickedConstraint) {
				m_pickedBody->forceActivationState(m_savedState);
				m_pickedBody->activate();
				this->physicsManager.dynamicsWorld->removeConstraint(m_pickedConstraint);
				delete m_pickedConstraint;
				m_pickedConstraint = 0;
				m_pickedBody = 0;
			}
		}








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


		camera.updateViewMatrix();



		globalP += 0.005f;



		{
			
			for (int i = 0; i < models.size(); ++i) {
				models[i]->matrixIndex = i;
			}


			// uses matrix indices directly after meshes' indices
			for (int i = 0; i < skinnedMeshes.size(); ++i) {
				skinnedMeshes[i]->matrixIndex = models.size() + i;
			}


			// uses matrix indices directly after skinnedMeshes' indices
			for (int i = 0; i < modelsDeferred.size(); ++i) {
				// added a buffer of 5 so that there is time to update command buffers
				modelsDeferred[i]->matrixIndex = models.size() + skinnedMeshes.size() + i + 2;// todo: figure this out
			}

			// uses matrix indices directly after modelsDeferred' indices
			for (int i = 0; i < skinnedMeshesDeferred.size(); ++i) {
				// added a buffer of 5 so that there is time to update command buffers
				skinnedMeshesDeferred[i]->matrixIndex = models.size() + skinnedMeshes.size() + modelsDeferred.size() + i + 2;// todo: figure this out
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

		for (auto &model : models) {
			matrixNodes[model->matrixIndex].model = model->transfMatrix;
		}

		// uboBoneData.bones is a large bone data buffer
		// use offset to store bone data for each skinnedMesh
		// basically a manual dynamic buffer
		for (auto &skinnedMesh : skinnedMeshes) {

			matrixNodes[skinnedMesh->matrixIndex].model = skinnedMesh->transfMatrix;
			matrixNodes[skinnedMesh->matrixIndex].boneIndex = skinnedMesh->boneIndex;

			// optimization needed here:
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
			matrixNodes[skinnedMesh->matrixIndex].boneIndex = skinnedMesh->boneIndex;

			// optimization needed here:
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


		// change to whenever camera moves
		//viewChanged();


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
		// the time since the last tick in seconds
		//auto tDuration = std::chrono::duration<double, std::milli>(tNow - this->physicsManager.tLastTimeStep);
		auto tSinceUpdate = std::chrono::duration_cast<std::chrono::microseconds>(tNow - this->physicsManager.tLastTimeStep).count() / 1000000.0;


		this->physicsManager.dynamicsWorld->stepSimulation(tSinceUpdate * 2, 4/*,1/50.*/);

		//this->physicsManager.dynamicsWorld->stepSimulation(tSinceUpdate*1.0, 1, 1/120.0);

		this->physicsManager.tLastTimeStep = std::chrono::high_resolution_clock::now();

		// todo: fix
		//this->physicsManager.dynamicsWorld->stepSimulation(tDuration.count() / 1000.0, 4);
		//this->physicsManager.dynamicsWorld->stepSimulation(deltaTime, 4);

		// sync:
		for (int i = 0; i < this->physicsObjects.size(); ++i) {
			this->physicsObjects[i]->sync();
		}
	}








	void updateCommandBuffers() {

		if (updateDraw) {
			// record / update draw command buffers
			if (TEST_DEFINE) {
				updateDrawCommandBuffers();
			} else {
				buildDrawCommandBuffers();
			}
		}

		if (updateOffscreen) {
			buildOffscreenCommandBuffer();
		}

	}


	void updateGUI() {

		ImGui::NewFrame();

		// Init imGui windows and elements

		ImVec4 clear_color = ImColor(114, 144, 154);
		static float f = 0.0f;
		ImGui::Text(this->title.c_str());
		ImGui::Text(context.deviceProperties.deviceName);


		// Update frame time display
		if (frameCounter == 0) {
			std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
			float frameTime = 1000.0f / (this->deltaTime * 1000.0f);
			//frameTime = rand0t1() * 100;
			uiSettings.frameTimes.back() = frameTime;

			if (frameTime < uiSettings.frameTimeMin) {
				uiSettings.frameTimeMin = frameTime;
			}
			if (frameTime > uiSettings.frameTimeMax && frameTime < 9000) {
				uiSettings.frameTimeMax = frameTime;
			}
		}

		ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 80));
		//ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 200));

		ImGui::Text("Camera");
		//ImGui::InputFloat3("position", &this->camera.transform.translation.x, 2);
		ImGui::DragFloat3("position", &this->camera.transform.translation.x, 0.1f);
		//ImGui::InputFloat3("rotation", &example->camera.transform.orientation.x, 3);

		ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Settings");

		ImGui::Checkbox("Update Draw Command Buffers", &updateDraw);
		ImGui::Checkbox("Update Offscreen Command Buffers", &updateOffscreen);
		ImGui::Checkbox("SSAO", &settings.SSAO);
		ImGui::Checkbox("Shadows", &settings.shadows);
		ImGui::Checkbox("Add Boxes", &keyStates.b);
		ImGui::SliderFloat("FPS Cap", &settings.fpsCap, 5.0f, 500.0f);





		// testing:
		//ImGui::InputFloat4("pos", &uboShadowGS.pos.x, 3);
		//ImGui::InputFloat4("pos", &uboShadowGS.pos.x, 3);

		//ImGui::DragFloat4("pos", &uboShadowGS.pos[0].x, 0.1f);
		//ImGui::DragFloat4("pos", &uboShadowGS.pos[1].x, 0.1f);
		//ImGui::DragFloat4("pos", &uboShadowGS.pos[2].x, 0.1f);
		ImGui::DragFloat("Depth Bias Slope", &settings.depthBiasSlope, 0.01f);
		ImGui::DragFloat("Depth Bias Constant", &settings.depthBiasConstant, 100.0f);
		ImGui::DragFloat("Spot Light FOV", &uboFSLights.spotlights[0].innerAngle, 0.05f);
		ImGui::DragFloat3("Spot Light Position", &uboFSLights.spotlights[0].position.x, 0.1f);
		ImGui::DragFloat3("Spot Light Target", &uboFSLights.spotlights[0].target.x, 0.1f);
		//ImGui::DragFloat("Spot Light FOV2", &uboFSLights.spotlights[0].outerAngle, 0.05f);
		ImGui::DragFloat("Spot Light Range", &uboFSLights.spotlights[0].range, 0.05f);

		ImGui::DragFloat3("Spot Light1 Color", &uboFSLights.spotlights[0].color.x, 0.1f);
		ImGui::DragFloat3("Spot Light2 Color", &uboFSLights.spotlights[1].color.x, 0.1f);

		ImGui::DragFloat3("Directional Light Dir", &uboFSLights.directionalLights[0].direction.x, 0.05f);
		ImGui::DragFloat3("Directional Light Dir2", &uboFSLights.directionalLights[1].direction.x, 0.05f);

		ImGui::DragFloat("Directional Light Near", &uboFSLights.directionalLights[0].zNear, 0.05f);
		ImGui::DragFloat("Directional Light Far", &uboFSLights.directionalLights[0].zFar, 0.05f);
		ImGui::DragFloat("Directional Light size", &uboFSLights.directionalLights[0].size, 0.05f);

		ImGui::DragFloat4("Split Depths", &temporary.splitDepths[0], 0.1f);

		//ImGui::DragFloat3("CSM Light Dir", &uboFSLights.csmlights[0].direction.x, 0.05f);


		//ImGui::DragIntRange2("range int (no bounds)", &begin_i, &end_i, 5, 0, 0, "Min: %.0f units", "Max: %.0f units");
		//ImGui::InputFloat4("mat4[0]", &uboShadowGS.mvp[0][0][0], 3);
		//ImGui::InputFloat4("mat4[1]", &uboShadowGS.mvp[0][1][0], 3);
		//ImGui::InputFloat4("mat4[2]", &uboShadowGS.mvp[0][2][0], 3);
		//ImGui::InputFloat4("mat4[3]", &uboShadowGS.mvp[0][3][0], 3);


		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow();

		// Render to generate draw buffers
		ImGui::Render();
	}







	void updateDrawCommandBuffer(const vk::CommandBuffer &cmdBuffer) {

		// todo: definitely remove thise:
		updateUniformBufferDeferredLights();
		updateUniformBufferSSAOParams();

		{
			/* DEFERRED QUAD */

			vk::Viewport viewport = vkx::viewport(settings.windowSize);
			cmdBuffer.setViewport(0, viewport);

			vk::Rect2D scissor = vkx::rect2D(settings.windowSize);
			cmdBuffer.setScissor(0, scissor);


			// renders quad
			uint32_t setNum = 3;// important!
			//uint32_t setNum = 0;// important!
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("deferred"), setNum, rscs.descriptorSets->get("deferred"), nullptr);
			if (debugDisplay) {
				if (settings.SSAO) {
					cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("deferred.debug.ssao"));
				} else {
					cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("deferred.debug"));
				}
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
			if (settings.SSAO) {
				cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("deferred.composition.ssao"));
			} else {
				cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("deferred.composition"));
			}
			cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshBuffers.quad.vertices.buffer, { 0 });
			cmdBuffer.bindIndexBuffer(meshBuffers.quad.indices.buffer, 0, vk::IndexType::eUint32);
			cmdBuffer.drawIndexed(6, 1, 0, 0, 1);
		}
	}





	void buildDrawCommandBuffers() {

		// todo: remove this:
		updateUniformBufferDeferredLights();

		{


			// start new imgui frame
			if (GUIOpen) {
				updateGUI();
				imGui->updateBuffers();
			}

			//context.trashCommandBuffers(drawCmdBuffers);

			vk::CommandBufferBeginInfo cmdBufInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };

			for (uint32_t i = 0; i < drawCmdBuffers.size(); ++i) {
				//for (uint32_t i = 0; i < swapChain.imageCount; ++i) {


				vk::CommandBuffer &cmdBuffer = drawCmdBuffers[i];

				cmdBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

				// begin
				cmdBuffer.begin(cmdBufInfo);


				// set target framebuffer
				renderPassBeginInfo.framebuffer = framebuffers[i];

				// begin renderpass
				//cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eSecondaryCommandBuffers);
				cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);



				updateDrawCommandBuffer(cmdBuffer);

				// render gui
				if (GUIOpen) {
					imGui->drawFrame(cmdBuffer);
				}


				// end render pass
				cmdBuffer.endRenderPass();
				// end command buffer
				cmdBuffer.end();


			}

		}

	}




	// Build command buffer for rendering the scene to the offscreen frame buffer 
	// and blitting it to the different texture targets
	void buildOffscreenCommandBuffer() {

		// Create separate command buffer for offscreen 
		// rendering
		if (!offscreenCmdBuffer) {
			vk::CommandBufferAllocateInfo cmd = vkx::commandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1);
			offscreenCmdBuffer = context.device.allocateCommandBuffers(cmd)[0];
		}

		// todo: create semaphore here?:

		vk::CommandBufferBeginInfo commandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };

		// begin offscreen command buffer
		offscreenCmdBuffer.begin(commandBufferBeginInfo);









		if (settings.shadows) {
			// shadow pass:
			{

				// Clear values for all attachments written in the fragment shader
				std::array<vk::ClearValue, 1> clearValues;
				clearValues[0].depthStencil = { 1.0f, 0 };

				vk::RenderPassBeginInfo renderPassBeginInfo;
				renderPassBeginInfo.renderPass = offscreen.framebuffers[3].renderPass;
				renderPassBeginInfo.framebuffer = offscreen.framebuffers[3].framebuffer;
				renderPassBeginInfo.renderArea.extent.width = offscreen.framebuffers[3].width;
				renderPassBeginInfo.renderArea.extent.height = offscreen.framebuffers[3].height;
				renderPassBeginInfo.clearValueCount = clearValues.size();
				renderPassBeginInfo.pClearValues = clearValues.data();


				// begin offscreen render pass
				offscreenCmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

				// set viewport and scissor
				vk::Viewport viewport = vkx::viewport(glm::uvec2(offscreen.framebuffers[3].width, offscreen.framebuffers[3].height));
				offscreenCmdBuffer.setViewport(0, viewport);
				vk::Rect2D scissor = vkx::rect2D(glm::uvec2(offscreen.framebuffers[3].width, offscreen.framebuffers[3].height));
				offscreenCmdBuffer.setScissor(0, scissor);


				// Set depth bias (aka "Polygon offset")
				offscreenCmdBuffer.setDepthBias(settings.depthBiasConstant, 0.0f, settings.depthBiasSlope);








				//if (settings.SSAO) {
				//	offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("offscreen.meshes.ssao"));
				//} else {
				//	offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("offscreen.meshes"));
				//}

				offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("shadow"));

				// for each model
				// model = group of meshes
				// todo: add skinned / animated model support
				for (auto &model : modelsDeferred) {

					// todo: fix
					//model->checkIfReady();
					if (!model->buffersReady) {
						continue;
					}

					// for each of the model's meshBuffers
					for (auto &meshBuffer : model->meshBuffers) {


						// bind vertex & index buffers
						offscreenCmdBuffer.bindVertexBuffers(meshBuffer->vertexBufferBinding, meshBuffer->vertices.buffer, vk::DeviceSize());
						offscreenCmdBuffer.bindIndexBuffer(meshBuffer->indices.buffer, 0, vk::IndexType::eUint32);

						// descriptor set #
						uint32_t setNum;

						// bind scene descriptor set
						//setIndex = 0;
						//offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen"), setIndex, rscs.descriptorSets->get("offscreen.scene"), nullptr);

						// bind shadow descriptor set?
						// for vs uniform buffer?
						// bind deferred descriptor set
						// layout: offscreen, set index = 0
						setNum = 0;
						offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen.shadow"), setNum, rscs.descriptorSets->get("shadow.scene"), nullptr);


						// dynamic uniform buffer to position objects
						uint32_t offset1 = model->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
						setNum = 1;
						offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen.shadow"), setNum, 1, &rscs.descriptorSets->get("shadow.matrix"), 1, &offset1);



						// draw:
						offscreenCmdBuffer.drawIndexed(meshBuffer->indexCount, 1, 0, 0, 0);
					}

				}



				offscreenCmdBuffer.endRenderPass();
			}
		}



















		// Offscreen render pass:
		{
			// Clear values for all attachments written in the fragment shader
			std::array<vk::ClearValue, 4> clearValues;
			clearValues[0].color = vkx::clearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
			clearValues[1].color = vkx::clearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
			clearValues[2].color = vkx::clearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
			clearValues[3].depthStencil = { 1.0f, 0 };

			vk::RenderPassBeginInfo renderPassBeginInfo;
			renderPassBeginInfo.renderPass = offscreen.framebuffers[0].renderPass;
			renderPassBeginInfo.framebuffer = offscreen.framebuffers[0].framebuffer;
			renderPassBeginInfo.renderArea.extent.width = offscreen.size.x;
			renderPassBeginInfo.renderArea.extent.height = offscreen.size.y;
			renderPassBeginInfo.clearValueCount = clearValues.size();
			renderPassBeginInfo.pClearValues = clearValues.data();


			// begin offscreen render pass
			offscreenCmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);


			// start of render pass
			vk::Viewport viewport = vkx::viewport(offscreen.size);
			offscreenCmdBuffer.setViewport(0, viewport);
			vk::Rect2D scissor = vkx::rect2D(offscreen.size);
			offscreenCmdBuffer.setScissor(0, scissor);



			// todo: add matrix indices for deferred models
			// for(int i = 0; i < deferredModels.size(); ++i) {




			// MODELS:

			// bind mesh pipeline
			// don't have to do this for every mesh
			// todo: create pipelinesDeferred.mesh
			if (settings.SSAO) {
				offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("offscreen.meshes.ssao"));
			} else {
				offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("offscreen.meshes"));
			}


			// for each model
			// model = group of meshes
			// todo: add skinned / animated model support
			for (auto &model : modelsDeferred) {

				// todo: fix
				//model->checkIfReady();
				if (!model->buffersReady) {
					continue;
				}

				// for each of the model's meshes
				for (auto &meshBuffer : model->meshBuffers) {


					// bind vertex & index buffers
					offscreenCmdBuffer.bindVertexBuffers(meshBuffer->vertexBufferBinding, meshBuffer->vertices.buffer, vk::DeviceSize());
					offscreenCmdBuffer.bindIndexBuffer(meshBuffer->indices.buffer, 0, vk::IndexType::eUint32);

					// descriptor set #
					uint32_t setNum;

					// bind scene descriptor set
					setNum = 0;
					offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen"), setNum, rscs.descriptorSets->get("offscreen.scene"), nullptr);


					//uint32_t offset1 = model->matrixIndex * alignedMatrixSize;
					uint32_t offset1 = model->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
					setNum = 1;
					offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen"), setNum, 1, &rscs.descriptorSets->get("offscreen.matrix"), 1, &offset1);


					// if we just bound this texture don't bind it again (this could be further optimized by ordering by textures used)
					if (lastMaterialName != meshBuffer->materialName) {

						lastMaterialName = meshBuffer->materialName;

						vkx::Material m = this->assetManager.materials.get(meshBuffer->materialName);
						//uint32_t materialIndex = this->assetManager.materials.get(mesh.meshBuffer.materialName).index;


						//uint32_t offset2 = m.index * static_cast<uint32_t>(alignedMaterialSize);
						// the third param is the set number!
						//setNum = 2;
						//offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.offscreen, setNum, 1, &descriptorSets[setNum], 1, &offset2);


						// bind material descriptor set containing texture:
						// todo: implement a better way to bind textures

						setNum = 2;
						offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen"), setNum, m.descriptorSet, nullptr);
					}


					// draw:
					offscreenCmdBuffer.drawIndexed(meshBuffer->indexCount, 1, 0, 0, 0);
				}

			}














			// SKINNED MESHES:

			// bind skinned mesh pipeline
			if (settings.SSAO) {
				offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("offscreen.skinnedMeshes.ssao"));
			} else {
				offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("offscreen.skinnedMeshes"));
			}
			for (auto &skinnedMesh : skinnedMeshesDeferred) {
				// bind vertex & index buffers
				offscreenCmdBuffer.bindVertexBuffers(skinnedMesh->vertexBufferBinding, skinnedMesh->meshBuffer->vertices.buffer, vk::DeviceSize());
				offscreenCmdBuffer.bindIndexBuffer(skinnedMesh->meshBuffer->indices.buffer, 0, vk::IndexType::eUint32);

				// descriptor set #
				uint32_t setNum;

				// bind scene descriptor set
				// Set 0: Binding 0:
				setNum = 0;
				offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen"), setNum, rscs.descriptorSets->get("offscreen.scene"), nullptr);

				// there is a bone uniform, set: 0, binding: 1


				// Set 1: Binding 0:
				uint32_t offset1 = skinnedMesh->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
				setNum = 1;
				offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen"), setNum, 1, &rscs.descriptorSets->get("offscreen.matrix"), 1, &offset1);


				// if we just bound this texture don't bind it again (this could be further optimized by ordering by textures used)
				if (lastMaterialName != skinnedMesh->meshBuffer->materialName) {
					lastMaterialName = skinnedMesh->meshBuffer->materialName;
					vkx::Material m = this->assetManager.materials.get(skinnedMesh->meshBuffer->materialName);


					// bind texture:
					// Set 2: Binding 0:
					setNum = 2;
					offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen"), setNum, m.descriptorSet, nullptr);
				}


				// draw:
				offscreenCmdBuffer.drawIndexed(skinnedMesh->meshBuffer->indexCount, 1, 0, 0, 0);
			}




			// end offscreen render pass

			offscreenCmdBuffer.endRenderPass();


		}



		if (!settings.SSAO) {
			// end early because we're not doing the SSAO passes
			offscreenCmdBuffer.end();
			return;
		}




		// SSAO Generation pass:
		{

			// Clear values for all attachments written in the fragment shader
			clearValues[0].color = vkx::clearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
			clearValues[1].depthStencil = { 1.0f, 0 };

			vk::RenderPassBeginInfo renderPassBeginInfo2;
			renderPassBeginInfo2.renderPass = offscreen.framebuffers[1].renderPass;
			renderPassBeginInfo2.framebuffer = offscreen.framebuffers[1].framebuffer;
			renderPassBeginInfo2.renderArea.extent.width = offscreen.size.x;
			renderPassBeginInfo2.renderArea.extent.height = offscreen.size.y;
			renderPassBeginInfo2.clearValueCount = 1;//2;//clearValues.size();//2
			renderPassBeginInfo2.pClearValues = clearValues.data();

			// begin SSAO render pass
			offscreenCmdBuffer.beginRenderPass(renderPassBeginInfo2, vk::SubpassContents::eInline);


			vk::Viewport viewport = vkx::viewport(offscreen.size);
			offscreenCmdBuffer.setViewport(0, viewport);
			vk::Rect2D scissor = vkx::rect2D(offscreen.size);
			offscreenCmdBuffer.setScissor(0, scissor);
			vk::DeviceSize offsets = { 0 };




			offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen.ssaoGenerate"), 0, 1, rscs.descriptorSets->getPtr("offscreen.ssao.generate"), 0, nullptr);
			offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("ssao.generate"));
			offscreenCmdBuffer.draw(3, 1, 0, 0);


			offscreenCmdBuffer.endRenderPass();
		}



		// Third pass: SSAO blur
		// -------------------------------------------------------------------------------------------------------
		{

			vk::RenderPassBeginInfo renderPassBeginInfo3;
			renderPassBeginInfo3.renderPass = offscreen.framebuffers[2].renderPass;
			renderPassBeginInfo3.framebuffer = offscreen.framebuffers[2].framebuffer;
			renderPassBeginInfo3.renderArea.extent.width = offscreen.size.x;
			renderPassBeginInfo3.renderArea.extent.height = offscreen.size.y;
			renderPassBeginInfo3.clearValueCount = 1;//2;//clearValues.size();//2
			renderPassBeginInfo3.pClearValues = clearValues.data();

			//renderPassBeginInfo.framebuffer = frameBuffers.ssaoBlur.frameBuffer;
			//renderPassBeginInfo.renderPass = frameBuffers.ssaoBlur.renderPass;
			//renderPassBeginInfo.renderArea.extent.width = frameBuffers.ssaoBlur.width;
			//renderPassBeginInfo.renderArea.extent.height = frameBuffers.ssaoBlur.height;

			offscreenCmdBuffer.beginRenderPass(renderPassBeginInfo3, vk::SubpassContents::eInline);

			vk::Viewport viewport = vkx::viewport(offscreen.size);
			offscreenCmdBuffer.setViewport(0, viewport);
			vk::Rect2D scissor = vkx::rect2D(offscreen.size);
			offscreenCmdBuffer.setScissor(0, scissor);



			offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rscs.pipelineLayouts->get("offscreen.ssaoBlur"), 0, 1, rscs.descriptorSets->getPtr("offscreen.ssao.blur"), 0, nullptr);
			offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rscs.pipelines->get("ssao.blur"));
			offscreenCmdBuffer.draw(3, 1, 0, 0);

			offscreenCmdBuffer.endRenderPass();

		}













		// end offscreen command buffer
		offscreenCmdBuffer.end();


	}










	void windowResized() {
		camera.updateViewMatrix();
		updateUniformBufferSSAOParams();

		//offscreen.destroy();
		//offscreen.prepare();
	}



	void viewChanged() override {
		camera.updateViewMatrix();
		updateSceneBufferDeferred();
		updateUniformBufferSSAOParams();
	}






	void loadTextures() {
		//textures.colorMap = textureLoader->loadTexture(
		//	getAssetPath() + "models/armor/colormap.ktx",
		//	vk::Format::eBc3UnormBlock);
		textures.colorMap = textureLoader->loadTexture(
			getAssetPath() + "models/kamen.ktx",
			vk::Format::eBc3UnormBlock);

		//// todo: replace with noise:
		//textures.ssaoNoise = textureLoader->loadTexture(
		//	getAssetPath() + "models/kamen.ktx",
		//	vk::Format::eBc3UnormBlock);
	}

	void generateQuads() {
		// Setup vertices for multiple screen aligned quads
		// Used for displaying final result and debug 
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

		// Last component of normal is used for debug display sampler index

		// top left:
		vertexBuffer.push_back({ { 1.0f, 1.0f, 0.0f },{ 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f } });// 0.0f
		vertexBuffer.push_back({ { 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f } });
		vertexBuffer.push_back({ { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f } });
		vertexBuffer.push_back({ { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f } });

		// top right:
		vertexBuffer.push_back({ { 2.0f, 1.0f, 0.0f },{ 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } });// 1.0f
		vertexBuffer.push_back({ { 1.0f, 1.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } });
		vertexBuffer.push_back({ { 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } });
		vertexBuffer.push_back({ { 2.0f, 0.0f, 0.0f },{ 1.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } });

		// bottom left:
		vertexBuffer.push_back({ { 1.0f, 2.0f, 0.0f },{ 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 2.0f } });// 2.0f
		vertexBuffer.push_back({ { 0.0f, 2.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 2.0f } });
		vertexBuffer.push_back({ { 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 2.0f } });
		vertexBuffer.push_back({ { 1.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 2.0f } });

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


		//offscreen.size = glm::uvec2(TEX_DIM);
		offscreen.size = glm::uvec2(settings.windowSize.width, settings.windowSize.height);

		vulkanApp::prepare();
		offscreen.prepare();

		//offscreen.depthFinalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

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


		{
			imGui = new ImGUI(&context);
			imGui->init((float)settings.windowSize.width, (float)settings.windowSize.height);
			imGui->initResources(renderPass, context.queue);
		}

		start();

		updateWorld();
		if (TEST_DEFINE) {
			updateDrawCommandBuffers();
		} else {
			buildDrawCommandBuffers();
		}

		buildOffscreenCommandBuffer();



		prepared = true;
	}

	void draw() override {
		if (TEST_DEFINE) {
			if (primaryCmdBuffersDirty) {
				buildPrimaryCommandBuffers();
			}
		}

		buildDrawCommandBuffers();

		prepareFrame();

		// draw current command buffers
		{
			// render to offscreen, then onscreen, use signal and wait semaphores to
			// ensure they happen in order


			// Offscreen rendering
			vk::SubmitInfo submitInfo;
			submitInfo.pWaitDstStageMask = this->submitInfo.pWaitDstStageMask;

			// Wait for swap chain presentation to finish
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &semaphores.presentComplete;

			// Signal ready with offscreen render complete semaphore
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &offscreen.renderComplete;

			// Submit work
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &offscreenCmdBuffer;

			// Submit
			//queue.submit(submitInfo, nullptr);
			context.queue.submit(submitInfo, renderFence);// temporary






														  // Scene rendering

														  ////vk::Fence deferredFence = swapChain.getSubmitFence();

														  // Wait for offscreen render complete
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &offscreen.renderComplete;

			// Signal ready with regular render complete semaphore
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &semaphores.renderComplete;

			// Submit work
			submitInfo.commandBufferCount = 1;
			//submitInfo.pCommandBuffers = &primaryCmdBuffers[currentBuffer];
			submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

			// Submit
			//queue.submit(submitInfo, deferredFence);
			context.queue.submit(submitInfo, nullptr);
		}


		// draw scene && wait for offscreen.rendercomplete semaphore
		if (TEST_DEFINE) {
			drawCurrentCommandBuffer(offscreen.renderComplete);
		}



		//// todo: fix / better solution
		// Wait for fence to signal that all command buffers are ready
		vk::Result fenceRes;
		do {
			fenceRes = context.device.waitForFences(renderFence, VK_TRUE, 100000000);
		} while (fenceRes == vk::Result::eTimeout);

		// reset fence for next submit
		context.device.resetFences(renderFence);




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
		ss.str(""); ss.clear();

		ss << std::fixed << std::setprecision(2) << (frameTimer) << "ms";
		textOverlay->addText(ss.str(), 5.0f, 45.0f, vkx::TextOverlay::alignLeft);
		ss.str(""); ss.clear();


		ss << std::setprecision(5) << "debug #1: " << debugValue1;
		textOverlay->addText(ss.str(), 5.0f, 65.0f, vkx::TextOverlay::alignLeft);
		ss.str(""); ss.clear();

		ss << std::setprecision(5) << "debug #2: " << debugValue2;
		textOverlay->addText(ss.str(), 5.0f, 85.0f, vkx::TextOverlay::alignLeft);
		ss.str(""); ss.clear();

		//ss << "GPU: ";
		//ss << context.deviceProperties.deviceName;
		//textOverlay->addText(ss.str(), 5.0f, 65.0f, vkx::TextOverlay::alignLeft);
		//ss.str(""); ss.clear();

		// vertical offset
		float vOffset = 160.0f;
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