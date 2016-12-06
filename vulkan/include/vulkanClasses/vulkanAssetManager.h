#pragma once


#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

#include "vulkanTools.h"
#include "vulkanContext.h"
#include "vulkanTextureLoader.h"
#include "vulkanMeshLoader.h"

#include "Object3D.h"






namespace vkx {

	//struct Material;

	class AssetManager {

		public:
			

			std::vector<Material> materials;
			std::vector<Texture> textures;




	};




}