#pragma once


#include <unordered_map>

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

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
		// name of the material
		std::string name;

		// index
		uint32_t index;

		// Material properties
		MaterialProperties properties;


		// Diffuse, specular, and bump channels
		//vkx::Texture diffuse;
		//vkx::Texture specular;
		//vkx::Texture bump;
		std::shared_ptr<vkx::Texture> diffuse;
		std::shared_ptr<vkx::Texture> specular;
		std::shared_ptr<vkx::Texture> bump;


		bool hasDiffuse = false;
		bool hasAlpha = false;
		bool hasBump = false;
		bool hasSpecular = false;


		// The material's descriptor set
		// this is inefficient, but works on all vulkan capable hardware
		
		vk::DescriptorSet descriptorSet;

		void destroy() {
			diffuse->destroy();
			specular->destroy();
			bump->destroy();
		}
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

	//struct SceneHolder {
	//	aiScene* pScene;
	//	std::vector<MeshEntry> m_Entries;
	//};

	struct MeshBuffer;// defined in vulkanMeshLoader.h






	template <typename T>
	class ResourceList {
		public:
			std::unordered_map<std::string, T> resources;
			const T get(std::string name) {
				return resources[name];
			}
			T *getPtr(std::string name) {
				return &resources[name];
			}
			bool present(std::string name) {
				return resources.find(name) != resources.end();
			}
	};

	template <typename T>
	class OrderedResourceList {
		public:
			std::map<std::string, T> resources;
			const T get(std::string name) {
				return resources[name];
			}
			T *getPtr(std::string name) {
				return &resources[name];
			}
			bool present(std::string name) {
				return resources.find(name) != resources.end();
			}
	};


	template <typename T>
	class OrderedVulkanResourceList {
		public:
			vk::Device &device;
			std::map<std::string, T> resources;
			OrderedVulkanResourceList(vk::Device &dev) : device(dev) {};
			const T get(std::string name) {
				if (!present(name)) {
					throw;
				}
				return resources[name];
			}
			T *getPtr(std::string name) {
				if (!present(name)) {
					throw;
				}
				return &resources[name];
			}
			bool present(std::string name) {
				return resources.find(name) != resources.end();
			}
	};




	template <typename T>
	class VulkanResourceList {
		public:
			vk::Device &device;
			std::unordered_map<std::string, T> resources;
			VulkanResourceList(vk::Device &dev) : device(dev) {};
			const T get(std::string name) {
				if (!present(name)) {
					throw;
				}
				return resources[name];
			}
			T *getPtr(std::string name) {
				if (!present(name)) {
					throw;
				}
				return &resources[name];
			}
			bool present(std::string name) {
				return resources.find(name) != resources.end();
			}
	};



	class PipelineLayoutList : public VulkanResourceList<vk::PipelineLayout> {

		public:

			PipelineLayoutList(vk::Device &dev) : VulkanResourceList(dev) {};

			~PipelineLayoutList() {
				//for (auto &pipelineLayout : resources) {
				//	device.destroyPipelineLayout(pipelineLayout.second, nullptr);
				//}
			}

			void destroy() {
				for (auto &pipelineLayout : resources) {
					device.destroyPipelineLayout(pipelineLayout.second, nullptr);
				}
			}

			vk::PipelineLayout add(std::string name, vk::PipelineLayoutCreateInfo &createInfo) {
				vk::PipelineLayout pipelineLayout = device.createPipelineLayout(createInfo, nullptr);
				resources[name] = pipelineLayout;
				return pipelineLayout;
			}
	};




	class PipelineList : public VulkanResourceList<vk::Pipeline> {

		public:

			PipelineList(vk::Device &dev) : VulkanResourceList(dev) {};

			~PipelineList() {
				//for (auto &pipeline : resources) {
				//	device.destroyPipeline(pipeline.second, nullptr);
				//}
			}

			void destroy() {
				for (auto &pipeline : resources) {
					device.destroyPipeline(pipeline.second, nullptr);
				}
			}

			//vk::Pipeline addGraphicsPipeline(std::string name, vk::PipelineCache &pipelineCache, vk::GraphicsPipelineCreateInfo &pipelineCreateInfo) {
			//	vk::Pipeline pipeline = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo, nullptr);
			//	resources[name] = pipeline;
			//	return pipeline;
			//}

			//void addGraphicsPipeline(std::string name, vk::Pipeline pipeline) {
			//	resources[name] = pipeline;
			//}

			void add(std::string name, vk::Pipeline pipeline) {
				resources[name] = pipeline;
			}

	};



	class DescriptorSetLayoutList : public VulkanResourceList<vk::DescriptorSetLayout> {

		public:
			DescriptorSetLayoutList(vk::Device &dev) : VulkanResourceList(dev) {};

			~DescriptorSetLayoutList() {
				//for (auto &descriptorSetLayout : resources) {
				//	device.destroyDescriptorSetLayout(descriptorSetLayout.second, nullptr);
				//}
			}

			void destroy() {
				for (auto &descriptorSetLayout : resources) {
					device.destroyDescriptorSetLayout(descriptorSetLayout.second, nullptr);
				}
			}

			vk::DescriptorSetLayout add(std::string name, vk::DescriptorSetLayoutCreateInfo createInfo) {
				vk::DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout(createInfo, nullptr);
				resources[name] = descriptorSetLayout;
				return descriptorSetLayout;
			}

			vk::DescriptorSetLayout add(std::string name, std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings) {

				vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
				descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();
				descriptorSetLayoutCreateInfo.bindingCount = descriptorSetLayoutBindings.size();

				vk::DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo, nullptr);
				resources[name] = descriptorSetLayout;
				return descriptorSetLayout;
			}

			void add(std::string name, vk::DescriptorSetLayout descriptorSetLayout) {
				resources[name] = descriptorSetLayout;
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
				// todo:
				//for (auto it = resources.begin(); it != resources.end(); ++it) {
				//	//device.destroyDescriptorPool(descriptorPool, nullptr);
				//	//device.freeDescriptorSets()
				//}
			}

			void destroy() {
				//for (auto &descriptorSet : resources) {
				//	device.freeDescriptorSets(descriptorPool, 1, &descriptorSet.second);
				//}
			}

			vk::DescriptorSet add(std::string name, vk::DescriptorSetAllocateInfo allocInfo) {
				vk::DescriptorSet descriptorSet = device.allocateDescriptorSets(allocInfo)[0];
				resources[name] = descriptorSet;
				return descriptorSet;
			}

			void add(std::string name, vk::DescriptorSet descriptorSet) {
				resources[name] = descriptorSet;
			}

	};


	class DescriptorPoolList : public VulkanResourceList<vk::DescriptorPool> {

		public:

			DescriptorPoolList(vk::Device &dev) : VulkanResourceList(dev) {};

			~DescriptorPoolList() {
				//for (auto &descriptorPool : resources) {
				//	device.destroyDescriptorPool(descriptorPool.second, nullptr);
				//}
			}

			void destroy() {
				for (auto &descriptorPool : resources) {
					device.destroyDescriptorPool(descriptorPool.second, nullptr);
				}
			}

			vk::DescriptorPool add(std::string name, vk::DescriptorPoolCreateInfo &createInfo) {
				vk::DescriptorPool descriptorPool = device.createDescriptorPool(createInfo, nullptr);
				resources[name] = descriptorPool;
				return descriptorPool;
			}

			// for convienience
			vk::DescriptorPool add(std::string name, std::vector<vk::DescriptorPoolSize> descriptorPoolSizes, uint32_t maxSets) {

				vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
				descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizes.size();
				descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
				descriptorPoolCreateInfo.maxSets = maxSets;

				vk::DescriptorPool descriptorPool = device.createDescriptorPool(descriptorPoolCreateInfo, nullptr);
				resources[name] = descriptorPool;
				return descriptorPool;
			}
	};


	class SceneList : public ResourceList</*const */aiScene*> {

		public:

			//DescriptorPoolList(vk::Device &dev) : VulResourceList(dev) {};

			~SceneList() {

			}

			void add(std::string name, /*const */aiScene* scene) {
				resources[name] = scene;
			}
	};

	
	// allows for different vertex layouts but is awful to use:
	//class MeshBuffersList : public ResourceList<std::vector<std::vector<MeshBuffer>>> {

	//	public:

	//		//DescriptorPoolList(vk::Device &dev) : VulResourceList(dev) {};

	//		~MeshBuffersList() {

	//		}

	//		void add(std::string name, std::vector<MeshBuffer> meshBuffers) {
	//			if (present(name)) {
	//				resources[name].push_back(meshBuffers);
	//			} else {
	//				std::vector<std::vector<MeshBuffer>> buffers;
	//				buffers.push_back(meshBuffers);
	//				resources[name] = buffers;
	//			}
	//		}
	//};

	//std::vector<std::shared_ptr<MeshBuffer>> meshesDeferred;

	// assumes correct vertex layout / will overwrite even if vertex layout is different:
	
	/*class MeshBuffersList : public ResourceList<std::vector<MeshBuffer>> {*/
	class MeshBuffersList : public ResourceList<std::vector<std::shared_ptr<MeshBuffer>>> {
		public:
			~MeshBuffersList() {
			}

			void add(std::string name, std::vector<std::shared_ptr<MeshBuffer>> meshBuffers) {
				resources[name] = meshBuffers;
			}
	};



	class TextureList : public ResourceList<vkx::Texture> {

		public:

			~TextureList() {
				//for (auto &texture : resources) {
				//	texture.second.destroy();
				//}
			}

			void destroy() {
				for (auto &texture : resources) {
					texture.second.destroy();
				}
			}

			const vkx::Texture get(std::string name) {
				return resources[name];
			}

			std::shared_ptr<vkx::Texture> getSharedPtr(std::string name) {
				auto texture = std::make_shared<vkx::Texture>(resources[name]);

				return texture;
			}

			std::shared_ptr<vkx::Texture> getOrLoad(std::string name, vkx::TextureLoader *textureLoader) {
				if (present(name)) {
					auto texture = std::make_shared<vkx::Texture>(resources[name]);
					return texture;
				} else {
					vkx::Texture tex = textureLoader->loadTexture(name, vk::Format::eBc2UnormBlock);
					add(name, tex);
					auto texture = std::make_shared<vkx::Texture>(resources[name]);
					return texture;
				}

			}

			void add(std::string name, vkx::Texture texture) {
				resources[name] = texture;
			}
	};


	//class MaterialList : public OrderedVulkanResourceList<Material> {

	//	public:
	//		~MaterialList() {
	//			//for (auto &texture : resources) {
	//			//	texture.second.destroy();
	//			//}
	//		}

	//		void destroy() {
	//			for (auto &material : resources) {
	//				material.second.destroy();
	//			}
	//		}

	//		const Material get(std::string name) {
	//			return resources[name];
	//		}

	//		void add(std::string name, Material material) {
	//			resources[name] = material;
	//			this->sync();
	//		}

	//		void sync() {

	//			uint32_t counter = 0;
	//			for (auto &iterator : resources) {
	//				iterator.second.index = counter;
	//				counter++;
	//			}
	//		}
	//};








	//struct Material;

	class AssetManager {

		public:
			
			
			//std::vector<Material> loadedMaterials;
			//std::vector<Texture> loadedTextures;

			


			vk::DescriptorSetLayout* materialDescriptorSetLayout{ nullptr };
			vk::DescriptorPool* materialDescriptorPool{ nullptr };


			vk::DescriptorSetLayout* materialDescriptorSetLayoutDeferred{ nullptr };
			vk::DescriptorPool* materialDescriptorPoolDeferred{ nullptr };






			struct MaterialList {

				std::map<std::string, Material> resources;

				void destroy() {
					for (auto &material : resources) {
						material.second.destroy();
					}
				}

				const Material get(std::string name) {
					return resources[name];
				}

				void add(std::string name, Material material) {
					resources[name] = material;
					this->sync();
				}

				bool present(std::string name) {
					return resources.find(name) != resources.end();
				}

				void sync() {
					uint32_t counter = 0;
					for (auto &iterator : resources) {
						iterator.second.index = counter;
						counter++;
					}
				}



			};







			MaterialList materials;

			TextureList textures;

			SceneList scenes;

			MeshBuffersList meshBuffers;

			void destroy() {
				//textures.destroy();
				//materials.destroy();
				//scenes.~SceneList();
			}


	};




}