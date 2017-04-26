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

	class SkinnedMesh : public Object3D {

		private:

		public:

			//vkx::Mesh mesh;

			//vkx::MeshBuffer meshBuffer;
			std::shared_ptr<MeshBuffer> meshBuffer = nullptr;

			uint32_t matrixIndex;
			uint32_t boneIndex;
			uint32_t vertexBufferBinding = 0;
			//vk::Pipeline pipeline;




			float animationSpeed = 0.75f;


			//http://stackoverflow.com/questions/15648844/using-smart-pointers-for-class-members



			// pointer to meshLoader
			//vkx::MeshLoader /*const */ *meshLoader = nullptr;
			vkx::MeshLoader *meshLoader = nullptr;

			vkx::Context *context = nullptr;




			//struct skinnedMeshVertex {
			//	glm::vec3 pos;
			//	glm::vec3 normal;
			//	glm::vec2 uv;
			//	glm::vec3 color;
			//	// Max. four bones per vertex
			//	float boneWeights[4];
			//	uint32_t boneIDs[4];
			//};






			SkinnedMesh();
			SkinnedMesh(vkx::Context *context, vkx::AssetManager *assetManager);
			//SkinnedMesh(vkx::Context *context, vkx::AssetManager *assetManager);




			// load model
			void load(const std::string &filename);
			// load model with custom flags
			void load(const std::string &filename, int flags);

			void createSkinnedMeshBuffer(const std::vector<VertexComponent> &layout, float scale);

			//void setup(const std::vector<VertexLayout> &layout, float scale);
			//void setup(float scale);

			void setAnimation(uint32_t animationIndex);

			void update(float time);

			void destroy();


	};

}