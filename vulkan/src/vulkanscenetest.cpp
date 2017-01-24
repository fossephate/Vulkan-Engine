/*
* Vulkan Example - Skeletal animation
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/


#include "vulkanApp.h"
#include <glm/gtc/type_ptr.hpp>


// Vertex layout used in this example
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec3 color;
	// Max. four bones per vertex
	float boneWeights[4];
	uint32_t boneIDs[4];
};

std::vector<vkx::VertexLayout> vertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4
};

// Maximum number of bones per mesh
// Must not be higher than same const in skinning shader
#define MAX_BONES 64
// Maximum number of bones per vertex
#define MAX_BONES_PER_VERTEX 4

// Skinned mesh class

// Per-vertex bone IDs and weights
struct VertexBoneData {
	std::array<uint32_t, MAX_BONES_PER_VERTEX> IDs;
	std::array<float, MAX_BONES_PER_VERTEX> weights;

	// Ad bone weighting to vertex info
	void add(uint32_t boneID, float weight) {
		for (uint32_t i = 0; i < MAX_BONES_PER_VERTEX; i++) {
			if (weights[i] == 0.0f) {
				IDs[i] = boneID;
				weights[i] = weight;
				return;
			}
		}
	}
};

// Stores information on a single bone
struct BoneInfo {
	aiMatrix4x4 offset;
	aiMatrix4x4 finalTransformation;

	BoneInfo() {
		offset = aiMatrix4x4();
		finalTransformation = aiMatrix4x4();
	};
};

//class SkinnedMesh {
//public:
//	// Bone related stuff
//	// Maps bone name with index
//	std::map<std::string, uint32_t> boneMapping;
//	// Bone details
//	std::vector<BoneInfo> boneInfo;
//	// Number of bones present
//	uint32_t numBones = 0;
//	// Root inverese transform matrix
//	aiMatrix4x4 globalInverseTransform;
//	// Per-vertex bone info
//	std::vector<VertexBoneData> bones;
//	// Bone transformations
//	std::vector<aiMatrix4x4> boneTransforms;
//
//	// Modifier for the animation 
//	float animationSpeed = 0.75f;
//	// Currently active animation
//	aiAnimation* pAnimation;
//
//	// Vulkan buffers
//	vkx::MeshBuffer meshBuffer;
//	// Reference to assimp mesh
//	// Required for animation
//	vkx::MeshLoader* meshLoader;
//
//	// Set active animation by index
//	void setAnimation(uint32_t animationIndex) {
//		assert(animationIndex < meshLoader->pScene->mNumAnimations);
//		pAnimation = meshLoader->pScene->mAnimations[animationIndex];
//	}
//
//	// Load bone information from ASSIMP mesh
//	void loadBones(uint32_t meshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones) {
//		for (uint32_t i = 0; i < pMesh->mNumBones; i++) {
//			uint32_t index = 0;
//
//			assert(pMesh->mNumBones <= MAX_BONES);
//
//			std::string name(pMesh->mBones[i]->mName.data);
//
//			if (boneMapping.find(name) == boneMapping.end()) {
//				// Bone not present, add new one
//				index = numBones;
//				numBones++;
//				BoneInfo bone;
//				boneInfo.push_back(bone);
//				boneInfo[index].offset = pMesh->mBones[i]->mOffsetMatrix;
//				boneMapping[name] = index;
//			} else {
//				index = boneMapping[name];
//			}
//
//			for (uint32_t j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {
//				uint32_t vertexID = meshLoader->m_Entries[meshIndex].vertexBase + pMesh->mBones[i]->mWeights[j].mVertexId;
//				Bones[vertexID].add(index, pMesh->mBones[i]->mWeights[j].mWeight);
//			}
//		}
//		boneTransforms.resize(numBones);
//	}
//
//	// Recursive bone transformation for given animation time
//	void update(float time) {
//		float TicksPerSecond = (float)(meshLoader->pScene->mAnimations[0]->mTicksPerSecond != 0 ? meshLoader->pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
//		float TimeInTicks = time * TicksPerSecond;
//		float AnimationTime = fmod(TimeInTicks, (float)meshLoader->pScene->mAnimations[0]->mDuration);
//
//		aiMatrix4x4 identity = aiMatrix4x4();
//		readNodeHierarchy(AnimationTime, meshLoader->pScene->mRootNode, identity);
//
//		for (uint32_t i = 0; i < boneTransforms.size(); i++) {
//			boneTransforms[i];
//			boneInfo[i].finalTransformation;
//
//
//			boneTransforms[i] = boneInfo[i].finalTransformation;
//		}
//
//		// note:
//		// this is vulkanscenetest.cpp
//		// i.e. this class is local and not vulkanSkinnedMesh.cpp, just for testing
//
//		this->boneInfo;
//		this->bones;
//		this->boneMapping;
//		this->boneTransforms;
//		std::cout << "test" << std::endl;
//
//	}
//
//private:
//	// Find animation for a given node
//	const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string nodeName) {
//		for (uint32_t i = 0; i < animation->mNumChannels; i++) {
//			const aiNodeAnim* nodeAnim = animation->mChannels[i];
//			if (std::string(nodeAnim->mNodeName.data) == nodeName) {
//				return nodeAnim;
//			}
//		}
//		return nullptr;
//	}
//
//	// Returns a 4x4 matrix with interpolated translation between current and next frame
//	aiMatrix4x4 interpolateTranslation(float time, const aiNodeAnim* pNodeAnim) {
//		aiVector3D translation;
//
//		if (pNodeAnim->mNumPositionKeys == 1) {
//			translation = pNodeAnim->mPositionKeys[0].mValue;
//		} else {
//			uint32_t frameIndex = 0;
//			for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
//				if (time < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
//					frameIndex = i;
//					break;
//				}
//			}
//
//			aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
//			aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];
//
//			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);
//
//			const aiVector3D& start = currentFrame.mValue;
//			const aiVector3D& end = nextFrame.mValue;
//
//			translation = (start + delta * (end - start));
//		}
//
//		aiMatrix4x4 mat;
//		aiMatrix4x4::Translation(translation, mat);
//		return mat;
//	}
//
//	// Returns a 4x4 matrix with interpolated rotation between current and next frame
//	aiMatrix4x4 interpolateRotation(float time, const aiNodeAnim* pNodeAnim) {
//		aiQuaternion rotation;
//
//		if (pNodeAnim->mNumRotationKeys == 1) {
//			rotation = pNodeAnim->mRotationKeys[0].mValue;
//		} else {
//			uint32_t frameIndex = 0;
//			for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
//				if (time < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
//					frameIndex = i;
//					break;
//				}
//			}
//
//			aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
//			aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];
//
//			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);
//
//			const aiQuaternion& start = currentFrame.mValue;
//			const aiQuaternion& end = nextFrame.mValue;
//
//			aiQuaternion::Interpolate(rotation, start, end, delta);
//			rotation.Normalize();
//		}
//
//		aiMatrix4x4 mat(rotation.GetMatrix());
//		return mat;
//	}
//
//
//	// Returns a 4x4 matrix with interpolated scaling between current and next frame
//	aiMatrix4x4 interpolateScale(float time, const aiNodeAnim* pNodeAnim) {
//		aiVector3D scale;
//
//		if (pNodeAnim->mNumScalingKeys == 1) {
//			scale = pNodeAnim->mScalingKeys[0].mValue;
//		} else {
//			uint32_t frameIndex = 0;
//			for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
//				if (time < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
//					frameIndex = i;
//					break;
//				}
//			}
//
//			aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
//			aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];
//
//			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);
//
//			const aiVector3D& start = currentFrame.mValue;
//			const aiVector3D& end = nextFrame.mValue;
//
//			scale = (start + delta * (end - start));
//		}
//
//		aiMatrix4x4 mat;
//		aiMatrix4x4::Scaling(scale, mat);
//		return mat;
//	}
//
//	// Get node hierarchy for current animation time
//	void readNodeHierarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform) {
//		std::string NodeName(pNode->mName.data);
//
//		aiMatrix4x4 NodeTransformation(pNode->mTransformation);
//
//		const aiNodeAnim* pNodeAnim = findNodeAnim(pAnimation, NodeName);
//
//		if (pNodeAnim) {
//			// Get interpolated matrices between current and next frame
//			aiMatrix4x4 matScale = interpolateScale(AnimationTime, pNodeAnim);
//			aiMatrix4x4 matRotation = interpolateRotation(AnimationTime, pNodeAnim);
//			aiMatrix4x4 matTranslation = interpolateTranslation(AnimationTime, pNodeAnim);
//
//			NodeTransformation = matTranslation * matRotation * matScale;
//		}
//
//		aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;
//
//		if (boneMapping.find(NodeName) != boneMapping.end()) {
//			uint32_t BoneIndex = boneMapping[NodeName];
//			boneInfo[BoneIndex].finalTransformation = globalInverseTransform * GlobalTransformation * boneInfo[BoneIndex].offset;
//		}
//
//		for (uint32_t i = 0; i < pNode->mNumChildren; i++) {
//			readNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
//		}
//	}
//};

class VulkanExample : public vkx::vulkanApp {
public:
	struct {
		vkx::Texture colorMap;
		vkx::Texture floor;
	} textures;

	struct {
		vk::PipelineVertexInputStateCreateInfo inputState;
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	vkx::SkinnedMesh *skinnedMesh;

	struct {
		vkx::UniformData vsScene;
		vkx::UniformData floor;
	} uniformData;

	struct UboVS {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 bones[MAX_BONES];
		glm::vec4 lightPos = glm::vec4(0.0f, -250.0f, 250.0f, 1.0);
		glm::vec4 viewPos;
	} uboVS;

	struct UboFloor {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(0.0, 0.0f, -25.0f, 1.0);
		glm::vec4 viewPos;
		glm::vec2 uvOffset;
	} uboFloor;

	struct {
		vk::Pipeline skinning;
		vk::Pipeline texture;
	} pipelines;

	struct {
		vkx::MeshBuffer floor;
	} meshes;

	vk::PipelineLayout pipelineLayout;
	vk::DescriptorSet descriptorSet;
	vk::DescriptorSetLayout descriptorSetLayout;

	struct {
		vk::DescriptorSet skinning;
		vk::DescriptorSet floor;
	} descriptorSets;

	float runningTime = 0.0f;

	VulkanExample() : vkx::vulkanApp(ENABLE_VALIDATION) {
		camera.type = camera.lookat;
		//camera.setZoom(-150.0f);
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -150.0f));
		zoomSpeed = 2.5f;
		rotationSpeed = 0.5f;
		//camera.setRotation({ -25.5f, 128.5f, 180.0f });
		title = "Vulkan Example - Skeletal animation";
	}

	~VulkanExample() {
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		device.destroyPipeline(pipelines.skinning);

		device.destroyPipelineLayout(pipelineLayout);
		device.destroyDescriptorSetLayout(descriptorSetLayout);


		textures.colorMap.destroy();

		uniformData.vsScene.destroy();

		// Destroy and free mesh resources 
		skinnedMesh->meshBuffer.destroy();
		delete(skinnedMesh->meshLoader);
		delete(skinnedMesh);
	}

	void updateDrawCommandBuffer(const vk::CommandBuffer& cmdBuffer) {
		cmdBuffer.setViewport(0, vkx::viewport(size));
		cmdBuffer.setScissor(0, vkx::rect2D(size));

		// Skinned mesh
		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skinning);
		cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, skinnedMesh->meshBuffer.vertices.buffer, { 0 });
		cmdBuffer.bindIndexBuffer(skinnedMesh->meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);
		cmdBuffer.drawIndexed(skinnedMesh->meshBuffer.indexCount, 1, 0, 0, 0);

		// Floor
		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets.floor, nullptr);
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.texture);
		cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshes.floor.vertices.buffer, { 0 });
		cmdBuffer.bindIndexBuffer(meshes.floor.indices.buffer, 0, vk::IndexType::eUint32);
		cmdBuffer.drawIndexed(meshes.floor.indexCount, 1, 0, 0, 0);
	}

	// Load a mesh based on data read via assimp 
	// The other example will use the VulkanMesh loader which has some additional functionality for loading meshes
	void loadMesh() {
		skinnedMesh = new vkx::SkinnedMesh(&context, &assetManager);
		//skinnedMesh->meshLoader = new vkx::MeshLoader(&context, &assetManager);
#if defined(__ANDROID__)
		skinnedMesh->meshLoader->assetManager = androidApp->activity->assetManager;
#endif
		//skinnedMesh->meshLoader->load(getAssetPath() + "models/goblin.dae", 0);
		skinnedMesh->load(getAssetPath() + "models/goblin.dae", 0);
		skinnedMesh->setAnimation(0);

		//skinnedMesh->setup(0.0005f);
		skinnedMesh->setup(1.0f);

		//// Setup bones
		//// One vertex bone info structure per vertex
		//skinnedMesh->bones.resize(skinnedMesh->meshLoader->numVertices);
		//// Store global inverse transform matrix of root node 
		//skinnedMesh->globalInverseTransform = skinnedMesh->meshLoader->pScene->mRootNode->mTransformation;
		//skinnedMesh->globalInverseTransform.Inverse();
		//// Load bones (weights and IDs)
		//for (uint32_t m = 0; m < skinnedMesh->meshLoader->m_Entries.size(); m++) {
		//	aiMesh *paiMesh = skinnedMesh->meshLoader->pScene->mMeshes[m];
		//	if (paiMesh->mNumBones > 0) {
		//		skinnedMesh->loadBones(m, paiMesh, skinnedMesh->bones);
		//	}
		//}

		//// Generate vertex buffer
		//std::vector<Vertex> vertexBuffer;
		//// Iterate through all meshes in the file
		//// and extract the vertex information used in this demo
		//for (uint32_t m = 0; m < skinnedMesh->meshLoader->m_Entries.size(); m++) {
		//	for (uint32_t i = 0; i < skinnedMesh->meshLoader->m_Entries[m].Vertices.size(); i++) {
		//		Vertex vertex;

		//		vertex.pos = skinnedMesh->meshLoader->m_Entries[m].Vertices[i].m_pos;
		//		vertex.pos.y = -vertex.pos.y;
		//		vertex.normal = skinnedMesh->meshLoader->m_Entries[m].Vertices[i].m_normal;
		//		vertex.uv = skinnedMesh->meshLoader->m_Entries[m].Vertices[i].m_tex;
		//		vertex.color = skinnedMesh->meshLoader->m_Entries[m].Vertices[i].m_color;

		//		// Fetch bone weights and IDs
		//		for (uint32_t j = 0; j < MAX_BONES_PER_VERTEX; j++) {
		//			vertex.boneWeights[j] = skinnedMesh->bones[skinnedMesh->meshLoader->m_Entries[m].vertexBase + i].weights[j];
		//			vertex.boneIDs[j] = skinnedMesh->bones[skinnedMesh->meshLoader->m_Entries[m].vertexBase + i].IDs[j];
		//		}

		//		vertexBuffer.push_back(vertex);
		//	}
		//}
		//uint32_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

		//// Generate index buffer from loaded mesh file
		//std::vector<uint32_t> indexBuffer;
		//for (uint32_t m = 0; m < skinnedMesh->meshLoader->m_Entries.size(); m++) {
		//	uint32_t indexBase = indexBuffer.size();
		//	for (uint32_t i = 0; i < skinnedMesh->meshLoader->m_Entries[m].Indices.size(); i++) {
		//		indexBuffer.push_back(skinnedMesh->meshLoader->m_Entries[m].Indices[i] + indexBase);
		//	}
		//}
		//uint32_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
		//skinnedMesh->meshBuffer.indexCount = indexBuffer.size();
		//skinnedMesh->meshBuffer.vertices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
		//skinnedMesh->meshBuffer.indices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
	}

	void loadTextures() {
		textures.colorMap = textureLoader->loadTexture(
			getAssetPath() + "textures/goblin_bc3.ktx",
			vk::Format::eBc3UnormBlock);

		textures.floor = textureLoader->loadTexture(
			getAssetPath() + "textures/pattern_35_bc3.ktx",
			vk::Format::eBc3UnormBlock);
	}

	void loadMeshes() {
		meshes.floor = vulkanApp::loadMesh(getAssetPath() + "models/plane_z.obj", vertexLayout, 512.0f);
	}

	void setupVertexDescriptions() {
		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Vertex), vk::VertexInputRate::eVertex);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(6);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);
		// Location 1 : Normal
		vertices.attributeDescriptions[1] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3);
		// Location 2 : Texture coordinates
		vertices.attributeDescriptions[2] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32Sfloat, sizeof(float) * 6);
		// Location 3 : Color
		vertices.attributeDescriptions[3] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat, sizeof(float) * 8);
		// Location 4 : Bone weights
		vertices.attributeDescriptions[4] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 11);
		// Location 5 : Bone IDs
		vertices.attributeDescriptions[5] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 5, vk::Format::eR32G32B32A32Sint, sizeof(float) * 15);

		vertices.inputState = vk::PipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool() {
		// Example uses one ubo and one combined image sampler
		std::vector<vk::DescriptorPoolSize> poolSizes =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 2),
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
			// Binding 1 : Fragment shader combined sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1),
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

		vk::DescriptorImageInfo texDescriptor =
			vkx::descriptorImageInfo(textures.colorMap.sampler, textures.colorMap.view, vk::ImageLayout::eGeneral);

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				descriptorSet,
				vk::DescriptorType::eUniformBuffer,
				0,
				&uniformData.vsScene.descriptor),
			// Binding 1 : Color map 
			vkx::writeDescriptorSet(
				descriptorSet,
				vk::DescriptorType::eCombinedImageSampler,
				1,
				&texDescriptor)
		};

		device.updateDescriptorSets(writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Floor
		descriptorSets.floor = device.allocateDescriptorSets(allocInfo)[0];

		texDescriptor.imageView = textures.floor.view;
		texDescriptor.sampler = textures.floor.sampler;

		writeDescriptorSets.clear();

		// Binding 0 : Vertex shader uniform buffer
		writeDescriptorSets.push_back(
			vkx::writeDescriptorSet(descriptorSets.floor, vk::DescriptorType::eUniformBuffer, 0, &uniformData.floor.descriptor));
		// Binding 1 : Color map 
		writeDescriptorSets.push_back(
			vkx::writeDescriptorSet(descriptorSets.floor, vk::DescriptorType::eCombinedImageSampler, 1, &texDescriptor));

		device.updateDescriptorSets(writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines() {
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkx::pipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology::eTriangleList, vk::PipelineInputAssemblyStateCreateFlags(), VK_FALSE);

		vk::PipelineRasterizationStateCreateInfo rasterizationState =
			vkx::pipelineRasterizationStateCreateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise);

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
			vkx::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());

		// Skinned rendering pipeline
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;

		//shaderStages[0] = context.loadShader(getAssetPath() + "shaders/skeletalanimation/mesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		//shaderStages[1] = context.loadShader(getAssetPath() + "shaders/skeletalanimation/mesh.frag.spv", vk::ShaderStageFlagBits::eFragment);

		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/skinnedMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/skinnedMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo =
			vkx::pipelineCreateInfo(pipelineLayout, renderPass);

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

		pipelines.skinning = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];

		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/skeletalanimation/texture.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/skeletalanimation/texture.frag.spv", vk::ShaderStageFlagBits::eFragment);
		pipelines.texture = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers() {
		// Vertex shader uniform buffer block
		uniformData.vsScene = context.createUniformBuffer(uboVS);

		// Floor
		uniformData.floor = context.createUniformBuffer(uboFloor);

		updateUniformBuffers(true);
	}

	void updateUniformBuffers(bool viewChanged) {
		if (viewChanged) {
			uboFloor.projection = uboVS.projection = camera.matrices.projection;
			uboFloor.model = uboVS.model = glm::scale(glm::rotate(camera.matrices.view, glm::radians(90.0f), glm::vec3(1, 0, 0)), glm::vec3(0.025f));
			uboFloor.viewPos = uboVS.viewPos = glm::vec4(-camera.transform.translation.x, -camera.transform.translation.y, -camera.transform.translation.z, 0.0f);

			uboFloor.model = glm::translate(uboFloor.model, glm::vec3(0.0f, 0.0f, -1800.0f));
		}

		// Update bones
		skinnedMesh->update(runningTime);
		//skinnedMesh->update(0.0f);
		for (uint32_t i = 0; i < skinnedMesh->boneTransforms.size(); i++) {
			uboVS.bones[i] = glm::transpose(glm::make_mat4(&skinnedMesh->boneTransforms[i].a1));
			//std::ofstream log("logfile.txt", std::ios_base::app | std::ios_base::out);
			//log << skinnedMesh->boneTransforms[i].a1 << "\n";
		}

		uniformData.vsScene.copy(uboVS);

		// Update floor animation
		uboFloor.uvOffset.t -= 0.5f * skinnedMesh->animationSpeed * frameTimer;
		uniformData.floor.copy(uboFloor);
	}

	void prepare() {
		vulkanApp::prepare();
		loadTextures();
		loadMesh();
		loadMeshes();
		setupVertexDescriptions();
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
		if (!paused) {
			runningTime += frameTimer * skinnedMesh->animationSpeed;
			updateUniformBuffers(false);
		}
	}

	virtual void viewChanged() {
		updateUniformBuffers(true);
	}

	void changeAnimationSpeed(float delta) {
		skinnedMesh->animationSpeed += delta;
		std::cout << "Animation speed = " << skinnedMesh->animationSpeed << std::endl;
	}


};

VulkanExample *vulkanExample;

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	VulkanExample* example = new VulkanExample();
	example->run();
	delete(example);
	return 0;
}
