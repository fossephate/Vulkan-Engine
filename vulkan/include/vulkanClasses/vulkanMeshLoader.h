/*
* Simple wrapper for getting an index buffer and vertices out of an assimp mesh
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdlib.h>
#include <string>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <algorithm>


#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#endif

#include <vulkan/vulkan.hpp>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

#include "vulkanTools.h"
#include "vulkanContext.h"
#include "vulkanTextureLoader.h"
#include "vulkanAssetManager.h"
#include "Object3D.h"










// Maximum number of bones per mesh
// Must not be higher than same const in skinning shader
#define MAX_BONES 64
// Maximum number of bones per vertex
#define MAX_BONES_PER_VERTEX 4



namespace vkx {

	class Model;
	class Mesh;
	class MeshLoader;
	class AssetManager;

	// vertex layout enums
	typedef enum VertexLayout {
		VERTEX_LAYOUT_POSITION = 0x0,
		VERTEX_LAYOUT_NORMAL = 0x1,
		VERTEX_LAYOUT_COLOR = 0x2,
		VERTEX_LAYOUT_UV = 0x3,
		VERTEX_LAYOUT_TANGENT = 0x4,
		VERTEX_LAYOUT_BITANGENT = 0x5,
		VERTEX_LAYOUT_DUMMY_FLOAT = 0x6,
		VERTEX_LAYOUT_DUMMY_VEC4 = 0x7
	} VertexLayout;


	/*extern */struct materialProperties;
	/*extern */struct Material;

	struct MeshBuffer {

		// vulkan buffers
		vkx::CreateBufferResult vertices;
		vkx::CreateBufferResult indices;

		// size? I'm not sure what this is
		glm::vec3 dim;

		uint32_t indexCount{ 0 };
		uint32_t materialIndex{ 0 };

		void destroy() {
			vertices.destroy();
			indices.destroy();
		}
	};




	// Get vertex size from vertex layout
	static uint32_t vertexSize(const std::vector<VertexLayout> &layout) {
		uint32_t vSize = 0;
		for (auto& layoutDetail : layout) {
			switch (layoutDetail) {
				// UV only has two components
			case VERTEX_LAYOUT_UV:
				vSize += 2 * sizeof(float);
				break;
			default:
				vSize += 3 * sizeof(float);
			}
		}
		return vSize;
	}











	// Simple mesh class for getting all the necessary stuff from models loaded via ASSIMP
	class MeshLoader {
		private:


			struct Vertex {
				glm::vec3 m_pos;
				glm::vec2 m_tex;
				glm::vec3 m_normal;
				glm::vec3 m_color;
				glm::vec3 m_tangent;
				glm::vec3 m_binormal;

				Vertex() {}

				Vertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec3& color) {
					m_pos = pos;
					m_tex = tex;
					m_normal = normal;
					m_color = color;
					m_tangent = tangent;
					m_binormal = bitangent;
				}
			};

			struct MeshEntry {
				uint32_t NumIndices;
				uint32_t MaterialIndex;
				uint32_t vertexBase;// offset (for indexed draw)? p sure

				std::vector<Vertex> Vertices;
				std::vector<uint32_t> Indices;
			};

		public:
			#if defined(__ANDROID__)
			AAssetManager* assetManager = nullptr;
			#endif

			// raw data
			std::vector<MeshEntry> m_Entries;
			

			// fitted to vertex layout && usable to draw
			MeshBuffer combinedBuffer;
			std::vector<MeshBuffer> meshBuffers;
			// temporary materials vector
			std::vector<Material> tempMaterials;

			struct Dimension {
				glm::vec3 min = glm::vec3(FLT_MAX);
				glm::vec3 max = glm::vec3(-FLT_MAX);
				glm::vec3 size;
			} dim;

			uint32_t numVertices{ 0 };




			// Optional
			struct {
				vk::Buffer buf;
				vk::DeviceMemory mem;
			} vertexBuffer;

			struct {
				vk::Buffer buf;
				vk::DeviceMemory mem;
				uint32_t count;
			} indexBuffer;

			

			vk::PipelineVertexInputStateCreateInfo vi;
			std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
			std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
			//vk::Pipeline pipeline;

			Assimp::Importer Importer;

			// not const
			vkx::AssetManager &assetManager;

			// also not const (needs to be modified: updatedescriptorsets)
			// circumvented by copying device
			const vkx::Context &context;

			// copy of device
			vk::Device device;
			vk::Queue queue;

			TextureLoader *textureLoader{ nullptr };
			const aiScene *pScene{ nullptr };

			MeshLoader(const vkx::Context &context, vkx::AssetManager &assetManager);

			~MeshLoader();

			// Loads the mesh with some default flags
			bool load(const std::string &filename);

			// Load the mesh with custom flags
			bool load(const std::string &filename, int flags);

			void loadMaterials(const aiScene *pScene);
			void loadMeshes(const aiScene *pScene);

			bool parse(const aiScene *pScene, const std::string &Filename);

			// Create vertex and index buffer with given layout
			// Note : Only does staging if a valid command buffer and transfer queue are passed
			void createMeshBuffer(const Context &context, const std::vector<VertexLayout> &layout, float scale);
			void createMeshBuffers(const Context &context, const std::vector<VertexLayout> &layout, float scale);
	};

}