#pragma once


#include "vulkanContext.h"
#include "vulkanAssetManager.h"
#include "Object3D.h"


namespace vkx {

	//extern struct MaterialProperties;
	//extern struct Material;



	// stores mesh info
	class Mesh : public Object3D {
		public:

			// todo: make this class like the model class
			
			uint32_t matrixIndex;

			uint32_t vertexBufferBinding = 0;

			// Mesh buffer
			vkx::MeshBuffer meshBuffer;

			Mesh();
			Mesh(vkx::MeshBuffer meshBuffer);

			void destroy();
	};

}