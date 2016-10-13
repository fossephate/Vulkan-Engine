/*
* Vulkan Example - Animated gears using multiple uniform buffers
*
* See readme.md for details
*
* Copyright (C) 2015 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include "vulkanOffscreen.h"
#include "vulkanTools.h"
#include "shapes.h"
#include "easings.h"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1

namespace vkx {
    class ShapesRenderer : public vkx::OffscreenRenderer {
        using Parent = vkx::OffscreenRenderer;
    public:
        static const uint32_t SHAPES_COUNT{ 5 };
        static const uint32_t INSTANCES_PER_SHAPE{ 4000 };
        static const uint32_t INSTANCE_COUNT{ (INSTANCES_PER_SHAPE * SHAPES_COUNT) };

        const bool stereo;
        vkx::CreateBufferResult meshes;

        // Per-instance data block
        struct InstanceData {
            glm::vec3 pos;
            glm::vec3 rot;
            float scale;
        };

        struct ShapeVertexData {
            size_t baseVertex;
            size_t vertices;
        };

        struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec3 color;
        };

        // Contains the instanced data
        using InstanceBuffer = vkx::CreateBufferResult;
        InstanceBuffer instanceBuffer;

        // Contains the instanced data
        using IndirectBuffer = vkx::CreateBufferResult;
        IndirectBuffer indirectBuffer;

        struct UboVS {
            glm::mat4 projection;
            glm::mat4 view;
            float time = 0.0f;
        } uboVS;

        struct {
            vkx::UniformData vsScene;
        } uniformData;

        struct {
            vk::Pipeline solid;
        } pipelines;

        std::vector<ShapeVertexData> shapes;
        vk::PipelineLayout pipelineLayout;
        vk::DescriptorSet descriptorSet;
        vk::DescriptorSetLayout descriptorSetLayout;
        vk::CommandBuffer cmdBuffer;
        const vk::DescriptorType uniformType{ stereo ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer };
        const float duration = 4.0f;
        const float interval = 6.0f;

        std::array<glm::mat4, 2> eyePoses;
#if 0
        float zoom{ -1.0f };
        float zoomDelta{ 135 };
        float zoomStart{ 0 };
        float accumulator{ FLT_MAX };
        float frameTimer{ 0 };
        bool paused{ false };
        glm::quat orientation;
#endif

		ShapesRenderer(const vkx::Context& context, bool stereo = false);

		~ShapesRenderer();

		void buildCommandBuffer();

        template<size_t N>
		void appendShape(const geometry::Solid<N>& solid, std::vector<Vertex>& vertices);

		void loadShapes();

		void setupDescriptorPool();

		void setupDescriptorSetLayout();

		void setupDescriptorSet();

		void preparePipelines();

		void prepareIndirectData();

		void prepareInstanceData();

		void prepareUniformBuffers();

		void prepare();

		void update(float deltaTime, const glm::mat4& projection, const glm::mat4& view);


		void update(float deltaTime, const std::array<glm::mat4, 2>& projections, const std::array<glm::mat4, 2>& views);

		void render();
    };

	template<size_t N>
	void ShapesRenderer::appendShape(const geometry::Solid<N>& solid, std::vector<Vertex>& vertices) {
		using namespace geometry;
		using namespace glm;
		using namespace std;
		ShapeVertexData shape;
		shape.baseVertex = vertices.size();

		auto faceCount = solid.faces.size();
		// FIXME triangulate the faces
		auto faceTriangles = triangulatedFaceTriangleCount<N>();
		vertices.reserve(vertices.size() + 3 * faceTriangles);


		vec3 color = vec3(rand(), rand(), rand()) / (float)RAND_MAX;
		color = vec3(0.3f) + (0.7f * color);
		for (size_t f = 0; f < faceCount; ++f) {
			const Face<N>& face = solid.faces[f];
			vec3 normal = solid.getFaceNormal(f);
			for (size_t ft = 0; ft < faceTriangles; ++ft) {
				// Create the vertices for the face
				vertices.push_back({ vec3(solid.vertices[face[0]]), normal, color });
				vertices.push_back({ vec3(solid.vertices[face[2 + ft]]), normal, color });
				vertices.push_back({ vec3(solid.vertices[face[1 + ft]]), normal, color });
			}
		}
		shape.vertices = vertices.size() - shape.baseVertex;
		shapes.push_back(shape);
	}

}
