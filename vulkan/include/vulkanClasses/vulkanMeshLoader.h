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
	class SkinnedMesh;
	class MeshLoader;
	class AssetManager;


	struct MaterialProperties;
	struct Material;
	struct BoneInfo;
	struct VertexBoneData;



	// vertex layout enums
	typedef enum VertexComponent {
		VERTEX_COMPONENT_POSITION = 0x0,
		VERTEX_COMPONENT_NORMAL = 0x1,
		VERTEX_COMPONENT_COLOR = 0x2,
		VERTEX_COMPONENT_UV = 0x3,
		VERTEX_COMPONENT_TANGENT = 0x4,
		VERTEX_COMPONENT_BITANGENT = 0x5,
		VERTEX_COMPONENT_DUMMY_FLOAT = 0x6,
		VERTEX_COMPONENT_DUMMY_VEC4 = 0x7,
		//VERTEX_COMPONENT_BONE_ID = 0x8,// added 1/20/17
		//VERTEX_COMPONENT_BONE_WEIGHT = 0x9,// added 1/20/17
	} VertexComponent;

	//std::vector<vkx::VertexComponent> defaultLayout =
	//{
	//	vkx::VertexComponent::VERTEX_COMPONENT_POSITION,
	//	vkx::VertexComponent::VERTEX_COMPONENT_UV,
	//	vkx::VertexComponent::VERTEX_COMPONENT_COLOR,
	//	vkx::VertexComponent::VERTEX_COMPONENT_NORMAL,
	//	vkx::VertexComponent::VERTEX_COMPONENT_TANGENT,
	//	vkx::VertexComponent::VERTEX_COMPONENT_DUMMY_VEC4,
	//	vkx::VertexComponent::VERTEX_COMPONENT_DUMMY_VEC4
	//};


	
	struct MeshBuffer {

		// vulkan buffers
		vkx::CreateBufferResult vertices;
		vkx::CreateBufferResult indices;

		uint32_t vertexBufferBinding = 0;// 5/4/17

		std::vector<VertexComponent> vertexLayout;// 4/26/17

		// dimensions of the mesh?
		glm::vec3 dim;

		uint32_t indexCount{ 0 };
		uint32_t materialIndex{ 0 };

		std::string materialName;

		void destroy() {
			vertices.destroy();
			indices.destroy();
		}

		~MeshBuffer() {
			vertices.destroy();
			indices.destroy();
		}
	};






	struct Vertex {
		glm::vec3 m_pos;
		glm::vec2 m_tex;
		glm::vec3 m_normal;
		glm::vec3 m_color;
		glm::vec3 m_tangent;
		glm::vec3 m_binormal;

		Vertex() {}

		Vertex(const glm::vec3 &pos, const glm::vec2 &tex, const glm::vec3 &normal, const glm::vec3 &tangent, const glm::vec3 &bitangent, const glm::vec3 &color) {
			m_pos = pos;
			m_tex = tex;
			m_normal = normal;
			m_color = color;
			m_tangent = tangent;
			m_binormal = bitangent;
		}
	};

	struct skinnedMeshVertex {
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 color;
		glm::vec3 normal;
		glm::vec3 tangent;// added 3/1/17
		// Max. four bones per vertex
		float boneWeights[4];
		uint32_t boneIDs[4];
	};

	struct MeshEntry {
		uint32_t numIndices;
		uint32_t vertexBase;// offset (for indexed draw)? p sure

		uint32_t materialIndex;
		std::string materialName;

		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;
	};









	// Get vertex size from vertex layout
	static uint32_t vertexSize(const std::vector<VertexComponent> &layout) {
		uint32_t vSize = 0;
		for (auto& layoutDetail : layout) {
			switch (layoutDetail) {
				// UV only has two components
				case VERTEX_COMPONENT_UV:
					vSize += 2 * sizeof(float);
					break;
				case VERTEX_COMPONENT_DUMMY_VEC4:
					vSize += 4 * sizeof(float);
					break;
				//case VERTEX_COMPONENT_BONE_ID:
				//	vSize += 4 * sizeof(float);
				//	break;
				//case VERTEX_COMPONENT_BONE_WEIGHT:
				//	vSize += 4 * sizeof(float);
				//	break;
				case VERTEX_COMPONENT_DUMMY_FLOAT:
					vSize += 1 * sizeof(float);
					break;
				default:
					vSize += 3 * sizeof(float);
					break;
			}
		}
		return vSize;
	}











	// Simple mesh class for getting all the necessary stuff from models loaded via ASSIMP
	class MeshLoader {
		private:



		public:

			#if defined(__ANDROID__)
			AAssetManager* assetManager = nullptr;
			#endif

			// raw data
			std::vector<MeshEntry> m_Entries;



			struct {
				// Bone related stuff
				// Maps bone name with index
				std::map<std::string, uint32_t> boneMapping;
				// Bone details
				std::vector<BoneInfo> boneInfo;

				// Number of bones present
				uint32_t numBones = 0;
				// Root inverse transform matrix
				aiMatrix4x4 globalInverseTransform;
				// Per-vertex bone info
				std::vector<VertexBoneData> bones;
				// Bone transformations
				std::vector<aiMatrix4x4> boneTransforms;
				// Currently active animation
				aiAnimation *pAnimation;

				// when the last update was
				std::chrono::steady_clock::time_point tLastUpdate = std::chrono::high_resolution_clock::now();
				// the current time
				std::chrono::steady_clock::time_point tNow = std::chrono::high_resolution_clock::now();
				// time to wait in ms to update bones
				float waitTimeMS = 90.0f;// 90.0f
			} boneData;



			
			// name of file that is loaded
			std::string filename = "";



			

			// fitted to vertex layout && usable to draw
			//MeshBuffer combinedBuffer;
			//std::vector<MeshBuffer> meshBuffers;
			std::shared_ptr<MeshBuffer> combinedBuffer = nullptr;
			std::vector<std::shared_ptr<MeshBuffer>> meshBuffers;

			//std::vector<Mesh> meshes;

			// temporary materials vector
			//std::vector<Material> tempMaterials;

			struct Dimension {
				glm::vec3 min = glm::vec3(FLT_MAX);
				glm::vec3 max = glm::vec3(-FLT_MAX);
				glm::vec3 size;
			} dim;

			uint32_t numVertices{ 0 };




			//// Optional
			//struct {
			//	vk::Buffer buf;
			//	vk::DeviceMemory mem;
			//} vertexBuffer;

			//struct {
			//	vk::Buffer buf;
			//	vk::DeviceMemory mem;
			//	uint32_t count;
			//} indexBuffer;

			Assimp::Importer Importer;

			// pointer to assetManager and context
			vkx::AssetManager *assetManager = nullptr;
			vkx::Context *context = nullptr;

			TextureLoader *textureLoader = nullptr;
			//const aiScene *pScene = nullptr;
			aiScene *pScene = nullptr;

			MeshLoader(vkx::Context *context, vkx::AssetManager *assetManager);

			~MeshLoader();

			// Loads the mesh with some default flags
			bool load(const std::string &filename);
			// Load the mesh with custom flags
			bool load(const std::string &filename, int flags);

			void loadMaterials(const aiScene *pScene);
			void loadMeshes(const aiScene *pScene);

			bool parse(const aiScene *pScene, const std::string &filename);

			// Create vertex and index buffer with given layout

			// for single meshes (multiple meshes are combined into a single buffer with only one material)
			void createMeshBuffer(const std::vector<VertexComponent> &layout, float scale);
			// for groups of meshes (models) with multiple buffers and materials
			void createMeshBuffers(const std::vector<VertexComponent> &layout, float scale);

			void destroy();

			/* Skinned Meshes */
			// for skinned meshes (with bones)
			void createSkinnedMeshBuffer(const std::vector<VertexComponent> &layout, float scale);

			void setAnimation(uint32_t animationIndex);
			void loadBones(uint32_t meshIndex, const aiMesh *pMesh, std::vector<VertexBoneData>& Bones);
			void update(float time);
			const aiNodeAnim* findNodeAnim(const aiAnimation *animation, const std::string nodeName);
			aiMatrix4x4 interpolateTranslation(float time, const aiNodeAnim *pNodeAnim);
			aiMatrix4x4 interpolateRotation(float time, const aiNodeAnim *pNodeAnim);
			aiMatrix4x4 interpolateScale(float time, const aiNodeAnim *pNodeAnim);
			void readNodeHierarchy(float AnimationTime, const aiNode *pNode, const aiMatrix4x4 &ParentTransform);
	};

}