#pragma once


#include "vulkanContext.h"
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


			//uint32_t matrixIndex;

			// Mesh buffer
			vkx::MeshBuffer meshBuffer;


			SkinnedMesh();

			SkinnedMesh(vkx::MeshBuffer meshBuffer);


















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



			// reference to meshLoader: // change from pointer to ref?
			vkx::MeshLoader *meshLoader;
			// reference to context:
			vkx::Context &context;
			// not constant// its members can change
			vkx::AssetManager &assetManager;


			// load model
			void load(const std::string &filename);
			// load model with custom flags
			void load(const std::string &filename, int flags);

			void createMeshes(const std::vector<VertexLayout> &layout, float scale, uint32_t binding);


	};

}