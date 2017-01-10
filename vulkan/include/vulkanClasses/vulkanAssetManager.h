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



//namespace vkx {
//
//
//
//	extern std::vector<Material> globalMaterials;
//	extern std::vector<Texture> globalTextures;
//
//
//}






namespace vkx {





	struct materialProperties {
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		float opacity;
	};

	// Stores info on the materials used in the scene
	struct Material {
		// name
		std::string name;
		// Material properties
		materialProperties properties;
		// The example only uses a diffuse channel
		// todo: add more
		Texture diffuse;
		// The material's descriptor set
		// this is inefficient, but works on all vulkan capable hardware
		
		vk::DescriptorSet descriptorSet;
	};








	//struct Material;

	class AssetManager {

		public:
			
			
			std::vector<Material> loadedMaterials;
			//std::vector<Texture> textures;
			std::vector<Texture> loadedTextures;

			vk::DescriptorSetLayout *materialDescriptorSetLayout{ nullptr };
			vk::DescriptorPool *materialDescriptorPool{ nullptr };

			//vkx::Context &context;


			//AssetManager(vkx::Context &context) :
			//	context(context)
			//{

			//}



	};




}