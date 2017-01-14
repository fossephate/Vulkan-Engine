#include "vulkanSkinnedMesh.h"



namespace vkx {


	// don't ever use this constructor
	SkinnedMesh::SkinnedMesh() :
		context(vkx::Context()), assetManager(assetManager)
	{

	}

	// reference way:
	SkinnedMesh::SkinnedMesh(vkx::Context &context, vkx::AssetManager &assetManager) :
		context(context), assetManager(assetManager)// init context with reference
	{
		this->meshLoader = new vkx::MeshLoader(context, assetManager);
	}


	//void SkinnedMesh::load(const std::string &filename) {
	//	this->meshLoader->load(filename);
	//}

	//void SkinnedMesh::load(const std::string &filename, int flags) {
	//	this->meshLoader->load(filename, flags);
	//}

	//void SkinnedMesh::createMeshes(const std::vector<VertexLayout> &layout, float scale, uint32_t binding) {

	//	this->meshLoader->createMeshBuffers(this->context, layout, scale);


	//	std::vector<MeshBuffer> meshBuffers = this->meshLoader->meshBuffers;

	//	//this->mesh = vkx::Mesh(meshBuffers[0]);

	//	// copy vector of materials this class
	//	//this->materials = this->meshLoader->materials;

	//	for (int i = 0; i < meshBuffers.size(); ++i) {
	//		//vkx::Mesh m(meshBuffers[i]);
	//		//this->meshes.push_back(m);
	//	}

	//	//this->mesh = this->meshes[0];

	//	//this->materials = this->meshLoader->materials;

	//	//this->attributeDescriptions = this->meshLoader->attributeDescriptions;

	//	this->vertexBufferBinding = binding;// important

	//	//this->setupVertexInputState(layout);// doesn't seem to be necessary/used

	//	//this->bindingDescription = this->meshLoader->bindingDescriptions[0];// ?
	//	//this->pipeline = this->meshLoader->pipeline;// not needed?
	//}





































		//// Set active animation by index
		//void SkinnedMesh::setAnimation(uint32_t animationIndex) {
		//	assert(animationIndex < meshLoader->pScene->mNumAnimations);
		//	pAnimation = meshLoader->pScene->mAnimations[animationIndex];
		//}

		//// Load bone information from ASSIMP mesh
		//void SkinnedMesh::loadBones(uint32_t meshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones) {
		//	for (uint32_t i = 0; i < pMesh->mNumBones; i++) {
		//		uint32_t index = 0;

		//		assert(pMesh->mNumBones <= MAX_BONES);

		//		std::string name(pMesh->mBones[i]->mName.data);

		//		if (boneMapping.find(name) == boneMapping.end()) {
		//			// Bone not present, add new one
		//			index = numBones;
		//			numBones++;
		//			BoneInfo bone;
		//			boneInfo.push_back(bone);
		//			boneInfo[index].offset = pMesh->mBones[i]->mOffsetMatrix;
		//			boneMapping[name] = index;
		//		} else {
		//			index = boneMapping[name];
		//		}

		//		for (uint32_t j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {
		//			uint32_t vertexID = meshLoader->m_Entries[meshIndex].vertexBase + pMesh->mBones[i]->mWeights[j].mVertexId;
		//			Bones[vertexID].add(index, pMesh->mBones[i]->mWeights[j].mWeight);
		//		}
		//	}
		//	boneTransforms.resize(numBones);
		//}

		//// Recursive bone transformation for given animation time
		//void SkinnedMesh::update(float time) {
		//	float TicksPerSecond = (float)(meshLoader->pScene->mAnimations[0]->mTicksPerSecond != 0 ? meshLoader->pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
		//	float TimeInTicks = time * TicksPerSecond;
		//	float AnimationTime = fmod(TimeInTicks, (float)meshLoader->pScene->mAnimations[0]->mDuration);

		//	aiMatrix4x4 identity = aiMatrix4x4();
		//	readNodeHierarchy(AnimationTime, meshLoader->pScene->mRootNode, identity);

		//	for (uint32_t i = 0; i < boneTransforms.size(); i++) {
		//		boneTransforms[i] = boneInfo[i].finalTransformation;
		//	}
		//}



		//// Find animation for a given node
		//const aiNodeAnim* SkinnedMesh::findNodeAnim(const aiAnimation* animation, const std::string nodeName) {
		//	for (uint32_t i = 0; i < animation->mNumChannels; i++) {
		//		const aiNodeAnim* nodeAnim = animation->mChannels[i];
		//		if (std::string(nodeAnim->mNodeName.data) == nodeName) {
		//			return nodeAnim;
		//		}
		//	}
		//	return nullptr;
		//}

		//// Returns a 4x4 matrix with interpolated translation between current and next frame
		//aiMatrix4x4 SkinnedMesh::interpolateTranslation(float time, const aiNodeAnim* pNodeAnim) {
		//	aiVector3D translation;

		//	if (pNodeAnim->mNumPositionKeys == 1) {
		//		translation = pNodeAnim->mPositionKeys[0].mValue;
		//	} else {
		//		uint32_t frameIndex = 0;
		//		for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
		//			if (time < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
		//				frameIndex = i;
		//				break;
		//			}
		//		}

		//		aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
		//		aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

		//		float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

		//		const aiVector3D& start = currentFrame.mValue;
		//		const aiVector3D& end = nextFrame.mValue;

		//		translation = (start + delta * (end - start));
		//	}

		//	aiMatrix4x4 mat;
		//	aiMatrix4x4::Translation(translation, mat);
		//	return mat;
		//}

		//// Returns a 4x4 matrix with interpolated rotation between current and next frame
		//aiMatrix4x4 SkinnedMesh::interpolateRotation(float time, const aiNodeAnim* pNodeAnim) {
		//	aiQuaternion rotation;

		//	if (pNodeAnim->mNumRotationKeys == 1) {
		//		rotation = pNodeAnim->mRotationKeys[0].mValue;
		//	} else {
		//		uint32_t frameIndex = 0;
		//		for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
		//			if (time < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
		//				frameIndex = i;
		//				break;
		//			}
		//		}

		//		aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
		//		aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

		//		float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

		//		const aiQuaternion& start = currentFrame.mValue;
		//		const aiQuaternion& end = nextFrame.mValue;

		//		aiQuaternion::Interpolate(rotation, start, end, delta);
		//		rotation.Normalize();
		//	}

		//	aiMatrix4x4 mat(rotation.GetMatrix());
		//	return mat;
		//}


		//// Returns a 4x4 matrix with interpolated scaling between current and next frame
		//aiMatrix4x4 SkinnedMesh::interpolateScale(float time, const aiNodeAnim* pNodeAnim) {
		//	aiVector3D scale;

		//	if (pNodeAnim->mNumScalingKeys == 1) {
		//		scale = pNodeAnim->mScalingKeys[0].mValue;
		//	} else {
		//		uint32_t frameIndex = 0;
		//		for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
		//			if (time < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
		//				frameIndex = i;
		//				break;
		//			}
		//		}

		//		aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
		//		aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

		//		float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

		//		const aiVector3D& start = currentFrame.mValue;
		//		const aiVector3D& end = nextFrame.mValue;

		//		scale = (start + delta * (end - start));
		//	}

		//	aiMatrix4x4 mat;
		//	aiMatrix4x4::Scaling(scale, mat);
		//	return mat;
		//}






		//// Get node hierarchy for current animation time
		//void SkinnedMesh::readNodeHierarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform) {
		//	std::string NodeName(pNode->mName.data);

		//	aiMatrix4x4 NodeTransformation(pNode->mTransformation);

		//	const aiNodeAnim* pNodeAnim = findNodeAnim(pAnimation, NodeName);

		//	if (pNodeAnim) {
		//		// Get interpolated matrices between current and next frame
		//		aiMatrix4x4 matScale = interpolateScale(AnimationTime, pNodeAnim);
		//		aiMatrix4x4 matRotation = interpolateRotation(AnimationTime, pNodeAnim);
		//		aiMatrix4x4 matTranslation = interpolateTranslation(AnimationTime, pNodeAnim);

		//		NodeTransformation = matTranslation * matRotation * matScale;
		//	}

		//	aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

		//	if (boneMapping.find(NodeName) != boneMapping.end()) {
		//		uint32_t BoneIndex = boneMapping[NodeName];
		//		boneInfo[BoneIndex].finalTransformation = globalInverseTransform * GlobalTransformation * boneInfo[BoneIndex].offset;
		//	}

		//	for (uint32_t i = 0; i < pNode->mNumChildren; i++) {
		//		readNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
		//	}
		//}














}