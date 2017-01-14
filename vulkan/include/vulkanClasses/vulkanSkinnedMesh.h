#pragma once

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

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
	//struct tempVertex {
	//	glm::vec3 pos;
	//	glm::vec3 normal;
	//	glm::vec2 uv;
	//	glm::vec3 color;
	//	// Max. four bones per vertex
	//	float boneWeights[4];
	//	uint32_t boneIDs[4];
	//};








	// Per-vertex bone IDs and weights
	struct VertexBoneData {
		std::array<uint32_t, MAX_BONES_PER_VERTEX> IDs;
		std::array<float, MAX_BONES_PER_VERTEX> weights;

		// Add bone weighting to vertex info
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






	class SkinnedMesh : public Object3D {

		private:

		public:

			vkx::Mesh mesh{};

			uint32_t matrixIndex;
			uint32_t vertexBufferBinding = 0;
			vk::Pipeline pipeline;

			// if there are too many vector member variables
			// the program crashes for some reason



			// Bone related stuff
			// Maps bone name with index
			std::map<std::string, uint32_t> boneMapping;
			// Bone details
			//std::vector<BoneInfo> boneInfo;
			// Number of bones present
			uint32_t numBones = 0;
			// Root inverse transform matrix
			aiMatrix4x4 globalInverseTransform;
			// Per-vertex bone info
			//std::vector<VertexBoneData> bones;
			// Bone transformations
			//std::vector<aiMatrix4x4> boneTransforms;
			// Currently active animation
			aiAnimation *pAnimation;




			// pointer to meshLoader
			vkx::MeshLoader *meshLoader;
			// reference to context
			vkx::Context &context;
			// reference to assetManager
			vkx::AssetManager &assetManager;













			SkinnedMesh();
			SkinnedMesh(vkx::Context &context, vkx::AssetManager &assetManager);




			//// load model
			//void load(const std::string &filename);
			//// load model with custom flags
			//void load(const std::string &filename, int flags);

			//void createMeshes(const std::vector<VertexLayout> &layout, float scale, uint32_t binding);




			//void setAnimation(uint32_t animationIndex);

			//void loadBones(uint32_t meshIndex, const aiMesh * pMesh, std::vector<VertexBoneData>& Bones);

			//void update(float time);




			//private:

			//const aiNodeAnim * findNodeAnim(const aiAnimation * animation, const std::string nodeName);

			//aiMatrix4x4 interpolateTranslation(float time, const aiNodeAnim * pNodeAnim);

			//aiMatrix4x4 interpolateRotation(float time, const aiNodeAnim * pNodeAnim);

			//aiMatrix4x4 interpolateScale(float time, const aiNodeAnim * pNodeAnim);

			//void readNodeHierarchy(float AnimationTime, const aiNode * pNode, const aiMatrix4x4 & ParentTransform);









	};

}