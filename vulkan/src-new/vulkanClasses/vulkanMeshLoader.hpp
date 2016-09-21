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
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#endif

#include <vulkan/vulkan.h>

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

namespace vkx {
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

    using MeshLayout = std::vector<VertexLayout>;

    using MeshBufferInfo = CreateBufferResult;

    struct MeshBuffer {
        MeshBufferInfo vertices;
        MeshBufferInfo indices;
        uint32_t indexCount{ 0 };
        glm::vec3 dim;

        void destroy() {
            vertices.destroy();
            indices.destroy();
        }
    };

    // Get vertex size from vertex layout
    static uint32_t vertexSize(const MeshLayout& layout) {
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

    // Stores some additonal info and functions for 
    // specifying pipelines, vertex bindings, etc.
    class Mesh {
    public:
        MeshBuffer buffers;

        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
        vk::DescriptorSet descriptorSet;

        uint32_t vertexBufferBinding = 0;

        vk::PipelineVertexInputStateCreateInfo vertexInputState;
        vk::VertexInputBindingDescription bindingDescription;
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

        void setupVertexInputState(const std::vector<VertexLayout>& layout) {
            bindingDescription = vertexInputBindingDescription(
                vertexBufferBinding,
                vertexSize(layout),
                vk::VertexInputRate::eVertex);

            attributeDescriptions.clear();
            uint32_t offset = 0;
            uint32_t binding = 0;
            for (auto& layoutDetail : layout) {
                // vk::Format (layout)
                vk::Format format = (layoutDetail == VERTEX_LAYOUT_UV) ? vk::Format::eR32G32Sfloat : vk::Format::eR32G32B32Sfloat;

                attributeDescriptions.push_back(
                    vertexInputAttributeDescription(
                        vertexBufferBinding,
                        binding,
                        format,
                        offset));

                // Offset
                offset += (layoutDetail == VERTEX_LAYOUT_UV) ? (2 * sizeof(float)) : (3 * sizeof(float));
                binding++;
            }

            vertexInputState = vk::PipelineVertexInputStateCreateInfo();
            vertexInputState.vertexBindingDescriptionCount = 1;
            vertexInputState.pVertexBindingDescriptions = &bindingDescription;
            vertexInputState.vertexAttributeDescriptionCount = attributeDescriptions.size();
            vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
        }

        void drawIndexed(const vk::CommandBuffer& cmdBuffer) {
            if (pipeline) {
                cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
            }
            if ((pipelineLayout) && (descriptorSet)) {
                cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
            }
            cmdBuffer.bindVertexBuffers(vertexBufferBinding, buffers.vertices.buffer, vk::DeviceSize());
            cmdBuffer.bindIndexBuffer(buffers.indices.buffer, 0, vk::IndexType::eUint32);
            cmdBuffer.drawIndexed(buffers.indexCount, 1, 0, 0, 0);
        }
    };


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
            uint32_t vertexBase;
            std::vector<Vertex> Vertices;
            std::vector<unsigned int> Indices;
        };

    public:
#if defined(__ANDROID__)
        AAssetManager* assetManager = nullptr;
#endif

        std::vector<MeshEntry> m_Entries;

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
        vk::Pipeline pipeline;

        Assimp::Importer Importer;
        const aiScene* pScene{ nullptr };

        ~MeshLoader() {
            m_Entries.clear();
        }

        // Loads the mesh with some default flags
        bool load(const std::string& filename) {
            int flags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

            return load(filename, flags);
        }

        // Load the mesh with custom flags
        bool load(const std::string& filename, int flags) {
#if defined(__ANDROID__)
            // Meshes are stored inside the apk on Android (compressed)
            // So they need to be loaded via the asset manager

            AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
            assert(asset);
            size_t size = AAsset_getLength(asset);

            assert(size > 0);

            void *meshData = malloc(size);
            AAsset_read(asset, meshData, size);
            AAsset_close(asset);

            pScene = Importer.ReadFileFromMemory(meshData, size, flags);

            free(meshData);
#else
            pScene = Importer.ReadFile(filename.c_str(), flags);
#endif
            if (!pScene) {
                throw std::runtime_error("Unable to parse " + filename);
            }
            return parse(pScene, filename);
        }

    private:
        bool parse(const aiScene* pScene, const std::string& Filename) {
            m_Entries.resize(pScene->mNumMeshes);

            // Counters
            for (unsigned int i = 0; i < m_Entries.size(); i++) {
                m_Entries[i].vertexBase = numVertices;
                numVertices += pScene->mMeshes[i]->mNumVertices;
            }

            // Initialize the meshes in the scene one by one
            for (unsigned int i = 0; i < m_Entries.size(); i++) {
                const aiMesh* paiMesh = pScene->mMeshes[i];
                InitMesh(i, paiMesh, pScene);
            }

            return true;
        }

        void InitMesh(unsigned int index, const aiMesh* paiMesh, const aiScene* pScene) {
            m_Entries[index].MaterialIndex = paiMesh->mMaterialIndex;

            aiColor3D pColor(0.f, 0.f, 0.f);
            pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

            aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

            for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
                aiVector3D* pPos = &(paiMesh->mVertices[i]);
                aiVector3D* pNormal = &(paiMesh->mNormals[i]);
                aiVector3D *pTexCoord;
                if (paiMesh->HasTextureCoords(0)) {
                    pTexCoord = &(paiMesh->mTextureCoords[0][i]);
                } else {
                    pTexCoord = &Zero3D;
                }
                aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[i]) : &Zero3D;
                aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[i]) : &Zero3D;

                Vertex v(glm::vec3(pPos->x, -pPos->y, pPos->z),
                    glm::vec2(pTexCoord->x, pTexCoord->y),
                    glm::vec3(pNormal->x, pNormal->y, pNormal->z),
                    glm::vec3(pTangent->x, pTangent->y, pTangent->z),
                    glm::vec3(pBiTangent->x, pBiTangent->y, pBiTangent->z),
                    glm::vec3(pColor.r, pColor.g, pColor.b)
                    );

                dim.max.x = fmax(pPos->x, dim.max.x);
                dim.max.y = fmax(pPos->y, dim.max.y);
                dim.max.z = fmax(pPos->z, dim.max.z);

                dim.min.x = fmin(pPos->x, dim.min.x);
                dim.min.y = fmin(pPos->y, dim.min.y);
                dim.min.z = fmin(pPos->z, dim.min.z);

                m_Entries[index].Vertices.push_back(v);
            }

            dim.size = dim.max - dim.min;

            for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
                const aiFace& Face = paiMesh->mFaces[i];
                if (Face.mNumIndices != 3)
                    continue;
                m_Entries[index].Indices.push_back(Face.mIndices[0]);
                m_Entries[index].Indices.push_back(Face.mIndices[1]);
                m_Entries[index].Indices.push_back(Face.mIndices[2]);
            }
        }

    public:
        // Create vertex and index buffer with given layout
        // Note : Only does staging if a valid command buffer and transfer queue are passed
        MeshBuffer createBuffers(const Context& context, const std::vector<VertexLayout>& layout, float scale) {

            std::vector<float> vertexBuffer;
            for (int m = 0; m < m_Entries.size(); m++) {
                for (int i = 0; i < m_Entries[m].Vertices.size(); i++) {
                    // Push vertex data depending on layout
                    for (auto& layoutDetail : layout) {
                        // Position
                        if (layoutDetail == VERTEX_LAYOUT_POSITION) {
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.x * scale);
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.y * scale);
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.z * scale);
                        }
                        // Normal
                        if (layoutDetail == VERTEX_LAYOUT_NORMAL) {
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.x);
                            vertexBuffer.push_back(-m_Entries[m].Vertices[i].m_normal.y);
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.z);
                        }
                        // Texture coordinates
                        if (layoutDetail == VERTEX_LAYOUT_UV) {
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.s);
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.t);
                        }
                        // Color
                        if (layoutDetail == VERTEX_LAYOUT_COLOR) {
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.r);
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.g);
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.b);
                        }
                        // Tangent
                        if (layoutDetail == VERTEX_LAYOUT_TANGENT) {
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.x);
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.y);
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.z);
                        }
                        // Bitangent
                        if (layoutDetail == VERTEX_LAYOUT_BITANGENT) {
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.x);
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.y);
                            vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.z);
                        }
                        // Dummy layout components for padding
                        if (layoutDetail == VERTEX_LAYOUT_DUMMY_FLOAT) {
                            vertexBuffer.push_back(0.0f);
                        }
                        if (layoutDetail == VERTEX_LAYOUT_DUMMY_VEC4) {
                            vertexBuffer.push_back(0.0f);
                            vertexBuffer.push_back(0.0f);
                            vertexBuffer.push_back(0.0f);
                            vertexBuffer.push_back(0.0f);
                        }
                    }
                }
            }
            MeshBuffer meshBuffer;
            meshBuffer.vertices.size = vertexBuffer.size() * sizeof(float);

            dim.min *= scale;
            dim.max *= scale;
            dim.size *= scale;

            std::vector<uint32_t> indexBuffer;
            for (uint32_t m = 0; m < m_Entries.size(); m++) {
                uint32_t indexBase = (uint32_t)indexBuffer.size();
                for (uint32_t i = 0; i < m_Entries[m].Indices.size(); i++) {
                    indexBuffer.push_back(m_Entries[m].Indices[i] + indexBase);
                }
            }

            meshBuffer.indexCount = (uint32_t)indexBuffer.size();
            // Use staging buffer to move vertex and index buffer to device local memory
            // Vertex buffer
            meshBuffer.vertices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
            // Index buffer
            meshBuffer.indices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
            meshBuffer.dim = dim.size;
            return meshBuffer;
        }
    };
}
