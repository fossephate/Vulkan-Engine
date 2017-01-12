#pragma once


#include "vulkanContext.h"
#include "vulkanMesh.h"
#include "vulkanMeshLoader.h"
#include "vulkanAssetManager.h"
#include "Object3D.h"




// Maximum number of bones per mesh
// Must not be higher than same const in skinning shader
#define MAX_BONES 64
// Maximum number of bones per vertex
#define MAX_BONES_PER_VERTEX 4

namespace vkx {











	// Vertex layout used in this example
	//struct Vertex {
	struct tempVertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec3 color;
		// Max. four bones per vertex
		float boneWeights[4];
		uint32_t boneIDs[4];
	};















	// stores mesh info
	class SkinnedMesh : public Object3D {
	public:

		vkx::Mesh mesh;
		std::vector<vkx::Mesh> meshes;

		uint32_t matrixIndex;
		uint32_t vertexBufferBinding = 0;

		vk::Pipeline pipeline;

		







		// Bone related stuff
		// Maps bone name with index
		std::map<std::string, uint32_t> boneMapping;
		// Bone details
		std::vector<BoneInfo> boneInfo;
		// Number of bones present
		uint32_t numBones = 0;
		// Root inverse transform matrix
		aiMatrix4x4 globalInverseTransform;
		// Per-vertex bone info
		std::vector<VertexBoneData> bones;
		// Bone transformations
		std::vector<aiMatrix4x4> boneTransforms;

		// Currently active animation
		aiAnimation* pAnimation;




		// reference to meshLoader: // change from pointer to ref?
		vkx::MeshLoader *meshLoader;
		// reference to context:
		vkx::Context &context;
		// not constant// its members can change
		vkx::AssetManager &assetManager;













		SkinnedMesh();

		SkinnedMesh(vkx::Context &context, vkx::AssetManager &assetManager);




		// load model
		void load(const std::string &filename);
		// load model with custom flags
		void load(const std::string &filename, int flags);

		void createMeshes(const std::vector<VertexLayout> &layout, float scale, uint32_t binding);

		void setAnimation(uint32_t animationIndex);

		void loadBones(uint32_t meshIndex, const aiMesh * pMesh, std::vector<VertexBoneData>& Bones);

		void update(float time);




		private:
		
		const aiNodeAnim * findNodeAnim(const aiAnimation * animation, const std::string nodeName);

		aiMatrix4x4 interpolateTranslation(float time, const aiNodeAnim * pNodeAnim);

		aiMatrix4x4 interpolateRotation(float time, const aiNodeAnim * pNodeAnim);

		aiMatrix4x4 interpolateScale(float time, const aiNodeAnim * pNodeAnim);

		void readNodeHierarchy(float AnimationTime, const aiNode * pNode, const aiMatrix4x4 & ParentTransform);













































	//	// Set active animation by index
	//	void setAnimation(uint32_t animationIndex) {
	//		assert(animationIndex < meshLoader->pScene->mNumAnimations);
	//		pAnimation = meshLoader->pScene->mAnimations[animationIndex];
	//	}

	//	// Load bone information from ASSIMP mesh
	//	void loadBones(uint32_t meshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones) {
	//		for (uint32_t i = 0; i < pMesh->mNumBones; i++) {
	//			uint32_t index = 0;

	//			assert(pMesh->mNumBones <= MAX_BONES);

	//			std::string name(pMesh->mBones[i]->mName.data);

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

	//			for (uint32_t j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {
	//				uint32_t vertexID = meshLoader->m_Entries[meshIndex].vertexBase + pMesh->mBones[i]->mWeights[j].mVertexId;
	//				Bones[vertexID].add(index, pMesh->mBones[i]->mWeights[j].mWeight);
	//			}
	//		}
	//		boneTransforms.resize(numBones);
	//	}

	//	// Recursive bone transformation for given animation time
	//	void update(float time) {
	//		float TicksPerSecond = (float)(meshLoader->pScene->mAnimations[0]->mTicksPerSecond != 0 ? meshLoader->pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
	//		float TimeInTicks = time * TicksPerSecond;
	//		float AnimationTime = fmod(TimeInTicks, (float)meshLoader->pScene->mAnimations[0]->mDuration);

	//		aiMatrix4x4 identity = aiMatrix4x4();
	//		readNodeHierarchy(AnimationTime, meshLoader->pScene->mRootNode, identity);

	//		for (uint32_t i = 0; i < boneTransforms.size(); i++) {
	//			boneTransforms[i] = boneInfo[i].finalTransformation;
	//		}
	//	}

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

	//	// Returns a 4x4 matrix with interpolated translation between current and next frame
	//	aiMatrix4x4 interpolateTranslation(float time, const aiNodeAnim* pNodeAnim) {
	//		aiVector3D translation;

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

	//			aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
	//			aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

	//			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

	//			const aiVector3D& start = currentFrame.mValue;
	//			const aiVector3D& end = nextFrame.mValue;

	//			translation = (start + delta * (end - start));
	//		}

	//		aiMatrix4x4 mat;
	//		aiMatrix4x4::Translation(translation, mat);
	//		return mat;
	//	}

	//	// Returns a 4x4 matrix with interpolated rotation between current and next frame
	//	aiMatrix4x4 interpolateRotation(float time, const aiNodeAnim* pNodeAnim) {
	//		aiQuaternion rotation;

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

	//			aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
	//			aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

	//			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

	//			const aiQuaternion& start = currentFrame.mValue;
	//			const aiQuaternion& end = nextFrame.mValue;

	//			aiQuaternion::Interpolate(rotation, start, end, delta);
	//			rotation.Normalize();
	//		}

	//		aiMatrix4x4 mat(rotation.GetMatrix());
	//		return mat;
	//	}


	//	// Returns a 4x4 matrix with interpolated scaling between current and next frame
	//	aiMatrix4x4 interpolateScale(float time, const aiNodeAnim* pNodeAnim) {
	//		aiVector3D scale;

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

	//			aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
	//			aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

	//			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

	//			const aiVector3D& start = currentFrame.mValue;
	//			const aiVector3D& end = nextFrame.mValue;

	//			scale = (start + delta * (end - start));
	//		}

	//		aiMatrix4x4 mat;
	//		aiMatrix4x4::Scaling(scale, mat);
	//		return mat;
	//	}

	//	// Get node hierarchy for current animation time
	//	void readNodeHierarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform) {
	//		std::string NodeName(pNode->mName.data);

	//		aiMatrix4x4 NodeTransformation(pNode->mTransformation);

	//		const aiNodeAnim* pNodeAnim = findNodeAnim(pAnimation, NodeName);

	//		if (pNodeAnim) {
	//			// Get interpolated matrices between current and next frame
	//			aiMatrix4x4 matScale = interpolateScale(AnimationTime, pNodeAnim);
	//			aiMatrix4x4 matRotation = interpolateRotation(AnimationTime, pNodeAnim);
	//			aiMatrix4x4 matTranslation = interpolateTranslation(AnimationTime, pNodeAnim);

	//			NodeTransformation = matTranslation * matRotation * matScale;
	//		}

	//		aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

	//		if (boneMapping.find(NodeName) != boneMapping.end()) {
	//			uint32_t BoneIndex = boneMapping[NodeName];
	//			boneInfo[BoneIndex].finalTransformation = globalInverseTransform * GlobalTransformation * boneInfo[BoneIndex].offset;
	//		}

	//		for (uint32_t i = 0; i < pNode->mNumChildren; i++) {
	//			readNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
	//		}
	//	}



	};

}