/*
* Mesh loader for creating Vulkan resources from models loaded with ASSIMP
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

#if defined(_WIN32)
	#include <windows.h>
	#include <fcntl.h>
	#include <io.h>
#endif

#if defined(__ANDROID__)
	#include <android/asset_manager.h>
#endif

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vulkan/vulkan.hpp>
#include "./vulkanDevice.h"



namespace vkMeshLoader
{
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

	struct MeshBufferInfo
	{
		VkBuffer buf = VK_NULL_HANDLE;
		VkDeviceMemory mem = VK_NULL_HANDLE;
		size_t size = 0;
	};

	/** @brief Stores a mesh's vertex and index descriptions */
	struct MeshDescriptor
	{
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
	};

	/** @brief Mesh representation storing all data required to generate buffers */
	struct MeshBuffer
	{
		std::vector<MeshDescriptor> meshDescriptors;
		MeshBufferInfo vertices;
		MeshBufferInfo indices;
		uint32_t indexCount;
		glm::vec3 dim;
	};

	/** @brief Holds parameters for mesh creation */
	struct MeshCreateInfo
	{
		glm::vec3 center;
		glm::vec3 scale;
		glm::vec2 uvscale;
	};

	/**
	* Get the size of a vertex layout
	*
	* @param layout VertexLayout to get the size for
	*
	* @return Size of the vertex layout in bytes
	*/
	// was static
	uint32_t vertexSize(std::vector<vkMeshLoader::VertexLayout> layout);

	// Note: Always assumes float formats
	/**
	* Generate vertex attribute descriptions for a layout at the given binding point
	*
	* @param layout VertexLayout from which to generate the descriptions
	* @param attributeDescriptions Refernce to a vector of the descriptions to generate
	* @param binding Index of the attribute description binding point
	*
	* @note Always assumes float formats
	*/
	// was originally static
	void getVertexInputAttributeDescriptions(std::vector<vkMeshLoader::VertexLayout> layout, std::vector<VkVertexInputAttributeDescription> &attributeDescriptions, uint32_t binding);

	// Stores some additonal info and functions for 
	// specifying pipelines, vertex bindings, etc.
	class Mesh
	{
		public:
			MeshBuffer buffers;

			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkPipeline pipeline = VK_NULL_HANDLE;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

			uint32_t vertexBufferBinding = 0;

			VkPipelineVertexInputStateCreateInfo vertexInputState;
			VkVertexInputBindingDescription bindingDescription;
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

			void setupVertexInputState(std::vector<vkMeshLoader::VertexLayout> layout);

			void drawIndexed(VkCommandBuffer cmdBuffer);
	};

	// was static
	void freeMeshBufferResources(VkDevice device, vkMeshLoader::MeshBuffer *meshBuffer);
}





// Simple mesh class for getting all the necessary stuff from models loaded via ASSIMP
class VulkanMeshLoader
{
	private:
		vkx::VulkanDevice *vulkanDevice;

		static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

		struct Vertex
		{
			glm::vec3 m_pos;
			glm::vec2 m_tex;
			glm::vec3 m_normal;
			glm::vec3 m_color;
			glm::vec3 m_tangent;
			glm::vec3 m_binormal;

			Vertex() {};

			Vertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec3& color);
		};

		struct MeshEntry {
			uint32_t NumIndices;
			uint32_t MaterialIndex;
			uint32_t vertexBase;
			std::vector<Vertex> Vertices;
			std::vector<unsigned int> Indices;
		};

	public:
		#if defined(__ANDROID__)
			AAssetManager* assetManager = nullptr;
		#endif

		std::vector<MeshEntry> m_Entries;

		struct Dimension
		{
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
		} dim;

		uint32_t numVertices = 0;

		Assimp::Importer Importer;
		const aiScene* pScene;

		/**
		* Default constructor
		*
		* @param vulkanDevice Pointer to a valid VulkanDevice
		*/
		VulkanMeshLoader(vkx::VulkanDevice * vulkanDevice);

		/**
		* Default destructor
		*
		* @note Does not free any Vulkan resources
		*/
		~VulkanMeshLoader();

		/**
		* Load a scene from a supported 3D file format
		*
		* @param filename Name of the file (or asset) to load
		* @param flags (Optional) Set of ASSIMP processing flags
		*
		* @return Returns true if the scene has been loaded
		*/
		bool LoadMesh(const std::string& filename, int flags = defaultFlags);

		/**
		* Read mesh data from ASSIMP mesh to an internal mesh representation that can be used to generate Vulkan buffers
		*
		* @param meshEntry Pointer to the target MeshEntry strucutre for the mesh data
		* @param paiMesh ASSIMP mesh to get the data from
		* @param pScene Scene file of the ASSIMP mesh
		*/
		void InitMesh(MeshEntry *meshEntry, const aiMesh* paiMesh, const aiScene* pScene);

		/**
		* Create Vulkan buffers for the index and vertex buffer using a vertex layout
		*
		* @note Only does staging if a valid command buffer and transfer queue are passed
		*
		* @param meshBuffer Pointer to the mesh buffer containing buffer handles and memory
		* @param layout Vertex layout for the vertex buffer
		* @param createInfo Structure containing information for mesh creation time (center, scaling, etc.)
		* @param useStaging If true, buffers are staged to device local memory
		* @param copyCmd (Required for staging) Command buffer to put the copy commands into
		* @param copyQueue (Required for staging) Queue to put copys into
		*/
		void createBuffers(
			vkMeshLoader::MeshBuffer *meshBuffer,
			std::vector<vkMeshLoader::VertexLayout> layout,
			vkMeshLoader::MeshCreateInfo *createInfo,
			bool useStaging,
			VkCommandBuffer copyCmd,
			VkQueue copyQueue);
};