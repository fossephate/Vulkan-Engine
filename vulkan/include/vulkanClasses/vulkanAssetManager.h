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
		// index
		uint32_t index;

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









	template <typename T>
	class VulkanResourceList {
		public:
			vk::Device &device;
			std::unordered_map<std::string, T> resources;
			VulkanResourceList(vk::Device &dev) : device(dev) {};
			const T get(std::string name)
			{
				return resources[name];
			}
			T *getPtr(std::string name)
			{
				return &resources[name];
			}
			bool present(std::string name)
			{
				return resources.find(name) != resources.end();
			}
	};

	class PipelineLayoutList : public VulkanResourceList<vk::PipelineLayout> {
		public:
			PipelineLayoutList(vk::Device &dev) : VulkanResourceList(dev) {};

			~PipelineLayoutList() {
				for (auto &pipelineLayout : resources) {
					device.destroyPipelineLayout(pipelineLayout.second, nullptr);
				}
			}
			vk::PipelineLayout add(std::string name, vk::PipelineLayoutCreateInfo &createInfo) {				vk::PipelineLayout pipelineLayout = device.createPipelineLayout(createInfo, nullptr);
				resources[name] = pipelineLayout;
				return pipelineLayout;
			}
	};




	class PipelineList : public VulkanResourceList<vk::Pipeline> {
		public:
			PipelineList(vk::Device &dev) : VulkanResourceList(dev) {};

			~PipelineList() {
				for (auto &pipeline : resources) {					device.destroyPipeline(pipeline.second, nullptr);
				}
			}
			vk::Pipeline addGraphicsPipeline(std::string name, vk::GraphicsPipelineCreateInfo &pipelineCreateInfo, vk::PipelineCache &pipelineCache) {				vk::Pipeline pipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
				resources[name] = pipeline;
				return pipeline;
			}
	};



	class DescriptorSetLayoutList : public VulkanResourceList<vk::DescriptorSetLayout> {
		public:
			DescriptorSetLayoutList(vk::Device &dev) : VulkanResourceList(dev) {};

			~DescriptorSetLayoutList() {
				for (auto &descriptorSetLayout : resources) {					device.destroyDescriptorSetLayout(descriptorSetLayout.second, nullptr);
				}
			}

			vk::DescriptorSetLayout add(std::string name, vk::DescriptorSetLayoutCreateInfo createInfo) {				vk::DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout(createInfo, nullptr);
				resources[name] = descriptorSetLayout;
				return descriptorSetLayout;
			}
	};

	class DescriptorSetList : public VulkanResourceList<vk::DescriptorSet> {
		private:
			//vk::DescriptorPool descriptorPool;
		public:
			DescriptorSetList(vk::Device &dev/*, vk::DescriptorPool pool*/) : VulkanResourceList(dev)/*, descriptorPool(pool)*/ {};

			~DescriptorSetList() {
				//for (auto& descriptorSet : resources) {
				//	vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet.second);
				//	device.freeDescriptorSets(descriptorPool, 1, &descriptorSet.second);
				//}
			}
			vk::DescriptorSet add(std::string name, vk::DescriptorSetAllocateInfo allocInfo) {				vk::DescriptorSet descriptorSet = device.allocateDescriptorSets(allocInfo)[0];
				resources[name] = descriptorSet;
				return descriptorSet;
		}
	};


	class DescriptorPoolList : public VulkanResourceList<vk::DescriptorPool> {
		public:
		DescriptorPoolList(vk::Device &dev) : VulkanResourceList(dev) {};

		~DescriptorPoolList() {
			for (auto &descriptorPool : resources) {
				//device.destroyPipelineLayout(pipelineLayout.second, nullptr);
			}
		}
		vk::DescriptorPool add(std::string name, vk::DescriptorPoolCreateInfo &createInfo) {			vk::DescriptorPool descriptorPool = device.createDescriptorPool(createInfo, nullptr);
			resources[name] = descriptorPool;
			return descriptorPool;
		}
	};










	//struct Material;

	class AssetManager {

		public:
			
			
			//std::vector<Material> loadedMaterials;
			//std::vector<Texture> loadedTextures;

			


			vk::DescriptorSetLayout* materialDescriptorSetLayout{ nullptr };
			vk::DescriptorPool* materialDescriptorPool{ nullptr };


			vk::DescriptorSetLayout* materialDescriptorSetLayoutDeferred{ nullptr };
			vk::DescriptorPool* materialDescriptorPoolDeferred{ nullptr };


			struct {
				//std::unordered_map<std::string, Material> loadedMaterials;

				// need order
				std::map<std::string, Material> loadedMaterials;

				const Material get(std::string name) {
					return loadedMaterials[name];
				}

				void add(std::string name, Material material) {
					loadedMaterials[name] = material;
					this->sync();
				}

				Material* getPtr(std::string name) {
					return &loadedMaterials[name];
				}

				bool doExist(std::string name) {
					return loadedMaterials.find(name) != loadedMaterials.end();
				}

				void sync() {
					uint32_t counter = 0;
					for (auto &iterator : loadedMaterials) {
						iterator.second.index = counter;
						counter++;
					}
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