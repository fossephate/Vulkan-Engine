#pragma once


#include <unordered_map>

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



// Maximum number of bones per mesh
// Must not be higher than same const in skinning shader
#define MAX_BONES 64
// Maximum number of bones per vertex
#define MAX_BONES_PER_VERTEX 4



namespace vkx {





	struct MaterialProperties {
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
		MaterialProperties properties;


		// Diffuse, specular, and bump channels
		vkx::Texture diffuse;
		vkx::Texture specular;
		vkx::Texture bump;

		bool hasAlpha = false;
		bool hasBump = false;
		bool hasSpecular = false;


		// The material's descriptor set
		// this is inefficient, but works on all vulkan capable hardware
		
		vk::DescriptorSet descriptorSet;
	};



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









	//template <typename T>
	//class VulkanResourceList
	//{
	//public:
	//	vk::Device &device;
	//	std::unordered_map<std::string, T> resources;
	//	VulkanResourceList(VkDevice &dev) : device(dev) {};
	//	const T get(std::string name)
	//	{
	//		return resources[name];
	//	}
	//	T *getPtr(std::string name)
	//	{
	//		return &resources[name];
	//	}
	//	bool present(std::string name)
	//	{
	//		return resources.find(name) != resources.end();
	//	}
	//};














	//struct Material;

	class AssetManager {

		public:
			
			
			//std::vector<Material> loadedMaterials;
			//std::vector<Texture> loadedTextures;

			


			vk::DescriptorSetLayout *materialDescriptorSetLayout{ nullptr };
			vk::DescriptorPool *materialDescriptorPool{ nullptr };


			vk::DescriptorSetLayout *materialDescriptorSetLayoutDeferred{ nullptr };
			vk::DescriptorPool *materialDescriptorPoolDeferred{ nullptr };


			struct {
				//std::unordered_map<std::string, Material> loadedMaterials;
				// need order
				std::map<std::string, Material> loadedMaterials;

				const Material get(std::string name) {
					return loadedMaterials[name];
				}

				void add(std::string name, Material material) {
					loadedMaterials[name] = material;
				}

				Material* getPtr(std::string name) {
					return &loadedMaterials[name];
				}

				bool doExist(std::string name) {
					return loadedMaterials.find(name) != loadedMaterials.end();
				}
			} materials;



			struct {
				std::unordered_map<std::string, vkx::Texture> loadedTextures;

				const vkx::Texture get(std::string name) {
					return loadedTextures[name];
				}

				void add(std::string name, vkx::Texture texture) {
					loadedTextures[name] = texture;
				}

				vkx::Texture* getPtr(std::string name) {
					return &loadedTextures[name];
				}

				bool doExist(std::string name) {
					return loadedTextures.find(name) != loadedTextures.end();
				}
			} textures;




	};




}