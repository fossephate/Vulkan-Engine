#pragma once



#include <vulkan/vulkan.hpp>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkanContext.h"
#include "vulkanMesh.h"
#include "vulkanMeshLoader.h"
#include "vulkanAssetManager.h"

#include "Object3D.h"


namespace vkx {

	// group of meshes
	// scene?
	// todo: fix
	class Model : public Object3D {

		private:

		public:

			// todo: change to meshbuffers:
			std::vector<Mesh> meshes;

			uint32_t matrixIndex = -1;
			uint32_t vertexBufferBinding = 0;


			// pointer to meshLoader
			vkx::MeshLoader *meshLoader = nullptr;

			


			// constructors:

			// no default constructor
			Model();
			Model(vkx::Context *context, vkx::AssetManager *assetManager);

			//~Model();




			// load model
			void load(const std::string &filename);
			// load model with custom flags
			void load(const std::string &filename, int flags);

			void createMeshes(const std::vector<VertexLayout> &layout, float scale, uint32_t binding);

			void destroy();
	};

}