/*
* Vulkan Example - Instanced mesh rendering, uses a separate vertex buffer for instanced data
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanExampleBase.h"
#include "shapes.h"
#include "easings.hpp"
#include <glm/gtc/quaternion.hpp>

#define SHAPES_COUNT 5
#define INSTANCES_PER_SHAPE 4000
#define INSTANCE_COUNT (INSTANCES_PER_SHAPE * SHAPES_COUNT)
using namespace vk;

class VulkanExample : public vkx::ExampleBase {
public:
    CreateBufferResult meshes;

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

    VulkanExample() : ExampleBase(ENABLE_VALIDATION) {
        camera.setZoom(-1.0f);
        rotationSpeed = 0.25f;
        title = "Vulkan Example - Instanced mesh rendering";
        srand(time(NULL));
    }

    ~VulkanExample() {
        device.destroyPipeline(pipelines.solid);
        device.destroyPipelineLayout(pipelineLayout);
        device.destroyDescriptorSetLayout(descriptorSetLayout);
        instanceBuffer.destroy();
        indirectBuffer.destroy();
        uniformData.vsScene.destroy();
        meshes.destroy();
    }

    void updateDrawCommandBuffer(const vk::CommandBuffer& cmdBuffer) override {
        cmdBuffer.setViewport(0, vkx::viewport(size));
        cmdBuffer.setScissor(0, vkx::rect2D(size));
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.solid);
        // Binding point 0 : Mesh vertex buffer
        cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshes.buffer, { 0 });
        // Binding point 1 : Instance data buffer
        cmdBuffer.bindVertexBuffers(INSTANCE_BUFFER_BIND_ID, instanceBuffer.buffer, { 0 });
        // Equivlant non-indirect commands:
        //for (size_t j = 0; j < SHAPES_COUNT; ++j) {
        //    auto shape = shapes[j];
        //    cmdBuffer.draw(shape.vertices, INSTANCES_PER_SHAPE, shape.baseVertex, j * INSTANCES_PER_SHAPE);
        //}
        cmdBuffer.drawIndirect(indirectBuffer.buffer, 0, SHAPES_COUNT, sizeof(vk::DrawIndirectCommand));
    }

    template<size_t N>
    void appendShape(const geometry::Solid<N>& solid, std::vector<Vertex>& vertices) {
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

    void loadShapes() {
        std::vector<Vertex> vertexData;
        size_t vertexCount = 0;
        appendShape<>(geometry::tetrahedron(), vertexData);
        appendShape<>(geometry::octahedron(), vertexData);
        appendShape<>(geometry::cube(), vertexData);
        appendShape<>(geometry::dodecahedron(), vertexData);
        appendShape<>(geometry::icosahedron(), vertexData);
        for (auto& vertex : vertexData) {
            vertex.position *= 0.2f;
        }
        meshes = stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexData);
    }

    void setupDescriptorPool() {
        // Example uses one ubo 
        std::vector<vk::DescriptorPoolSize> poolSizes =
        {
            vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
        };

        vk::DescriptorPoolCreateInfo descriptorPoolInfo =
            vkx::descriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 1);

        descriptorPool = device.createDescriptorPool(descriptorPoolInfo);
    }

    void setupDescriptorSetLayout() {
        // Binding 0 : Vertex shader uniform buffer
        std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings =
        {
            vkx::descriptorSetLayoutBinding(vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 0),
        };

        descriptorSetLayout = device.createDescriptorSetLayout(
            vk::DescriptorSetLayoutCreateInfo()
            .setBindingCount(setLayoutBindings.size())
            .setPBindings(setLayoutBindings.data()));

        pipelineLayout = device.createPipelineLayout(
            vk::PipelineLayoutCreateInfo()
            .setPSetLayouts(&descriptorSetLayout)
            .setSetLayoutCount(1));
    }

    void setupDescriptorSet() {
        vk::DescriptorSetAllocateInfo allocInfo =
            vkx::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

        descriptorSet = device.allocateDescriptorSets(allocInfo)[0];

        // Binding 0 : Vertex shader uniform buffer
        vk::WriteDescriptorSet writeDescriptorSet;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pBufferInfo = &uniformData.vsScene.descriptor;
        writeDescriptorSet.descriptorCount = 1;

        device.updateDescriptorSets(writeDescriptorSet, nullptr);
    }

    void preparePipelines() {
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState =
            vkx::pipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology::eTriangleList);

        vk::PipelineRasterizationStateCreateInfo rasterizationState =
            vkx::pipelineRasterizationStateCreateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise);

        vk::PipelineColorBlendAttachmentState blendAttachmentState =
            vkx::pipelineColorBlendAttachmentState();

        vk::PipelineColorBlendStateCreateInfo colorBlendState =
            vkx::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

        vk::PipelineDepthStencilStateCreateInfo depthStencilState =
            vkx::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, vk::CompareOp::eLessOrEqual);

        vk::PipelineViewportStateCreateInfo viewportState =
            vkx::pipelineViewportStateCreateInfo(1, 1);

        vk::PipelineMultisampleStateCreateInfo multisampleState =
            vkx::pipelineMultisampleStateCreateInfo(vk::SampleCountFlagBits::e1);

        std::vector<vk::DynamicState> dynamicStateEnables = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamicState =
            vkx::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());

        // Instacing pipeline
        // Load shaders
        std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
        {
            vkx::shader::initGlsl();
            shaderStages[0] = loadGlslShader(getAssetPath() + "shaders/indirect/indirect.vert", vk::ShaderStageFlagBits::eVertex);
            shaderStages[1] = loadGlslShader(getAssetPath() + "shaders/indirect/indirect.frag", vk::ShaderStageFlagBits::eFragment);
            vkx::shader::finalizeGlsl();
        }

        std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

        // Binding description
        bindingDescriptions.resize(2);

        // Mesh vertex buffer (description) at binding point 0
        bindingDescriptions[0] =
            vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Vertex), vk::VertexInputRate::eVertex);
        bindingDescriptions[1] =
            vkx::vertexInputBindingDescription(INSTANCE_BUFFER_BIND_ID, sizeof(InstanceData), vk::VertexInputRate::eInstance);

        // Attribute descriptions
        // Describes memory layout and shader positions
        attributeDescriptions.clear();

        // Per-Vertex attributes
        // Location 0 : Position
        attributeDescriptions.push_back(
            vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0,  vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)));
        // Location 1 : Color
        attributeDescriptions.push_back(
            vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1,  vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)));
        // Location 2 : Normal
        attributeDescriptions.push_back(
            vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2,  vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)));

        // Instanced attributes
        // Location 4 : Position
        attributeDescriptions.push_back(
            vkx::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 4,  vk::Format::eR32G32B32Sfloat, offsetof(InstanceData, pos)));
        // Location 5 : Rotation
        attributeDescriptions.push_back(
            vkx::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 5,  vk::Format::eR32G32B32A32Sfloat, offsetof(InstanceData, rot)));
        // Location 6 : Scale
        attributeDescriptions.push_back(
            vkx::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 6,  vk::Format::eR32Sfloat, offsetof(InstanceData, scale)));


        vk::GraphicsPipelineCreateInfo pipelineCreateInfo =
            vkx::pipelineCreateInfo(pipelineLayout, renderPass);

        vk::PipelineVertexInputStateCreateInfo vertexInputState;
        vertexInputState.vertexBindingDescriptionCount = bindingDescriptions.size();
        vertexInputState.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputState.vertexAttributeDescriptionCount = attributeDescriptions.size();
        vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

        pipelineCreateInfo.pVertexInputState = &vertexInputState;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.pMultisampleState = &multisampleState;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pDepthStencilState = &depthStencilState;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.stageCount = shaderStages.size();
        pipelineCreateInfo.pStages = shaderStages.data();

        pipelines.solid = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];
    }

    void prepareIndirectData() {
        std::vector<vk::DrawIndirectCommand> indirectData;
        indirectData.resize(SHAPES_COUNT);
        for (auto i = 0; i < SHAPES_COUNT; ++i) {
            auto& drawIndirectCommand = indirectData[i];
            const auto& shapeData = shapes[i];
            drawIndirectCommand.firstInstance = i * INSTANCES_PER_SHAPE;
            drawIndirectCommand.instanceCount = INSTANCES_PER_SHAPE;
            drawIndirectCommand.firstVertex = shapeData.baseVertex;
            drawIndirectCommand.vertexCount = shapeData.vertices;
        }
        indirectBuffer = stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndirectBuffer, indirectData);
    }


    void prepareInstanceData() {
        std::vector<InstanceData> instanceData;
        instanceData.resize(INSTANCE_COUNT);

        std::mt19937 rndGenerator(time(nullptr));
        std::uniform_real_distribution<float> uniformDist(0.0, 1.0);
        std::exponential_distribution<float> expDist(1);

        for (auto i = 0; i < INSTANCE_COUNT; i++) {
            auto& instance = instanceData[i];
            instance.rot = glm::vec3(M_PI * uniformDist(rndGenerator), M_PI * uniformDist(rndGenerator), M_PI * uniformDist(rndGenerator));
            float theta = 2 * M_PI * uniformDist(rndGenerator);
            float phi = acos(1 - 2 * uniformDist(rndGenerator));
            instance.scale = 0.1f + expDist(rndGenerator) * 3.0f;
            instance.pos = glm::normalize(glm::vec3(sin(phi) * cos(theta), sin(theta), cos(phi)));
            instance.pos *= instance.scale * (1.0f + expDist(rndGenerator) / 2.0f) * 4.0f;
        }

        instanceBuffer = stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, instanceData);
    }

    void prepareUniformBuffers() {
        uniformData.vsScene = createUniformBuffer(uboVS);
        updateUniformBuffer(true);
    }

    void updateUniformBuffer(bool viewChanged) {
        if (viewChanged) {
            uboVS.projection = getProjection();
            uboVS.view = camera.matrices.view;
        }

        if (!paused) {
            uboVS.time += frameTimer * 0.05f;
        }

        memcpy(uniformData.vsScene.mapped, &uboVS, sizeof(uboVS));
    }


    void prepare() {
        ExampleBase::prepare();
        loadShapes();
        prepareInstanceData();
        prepareIndirectData();
        prepareUniformBuffers();
        setupDescriptorSetLayout();
        preparePipelines();
        setupDescriptorPool();
        setupDescriptorSet();
        updateDrawCommandBuffers();
        prepared = true;
    }

    const float duration = 4.0f;
    const float interval = 6.0f;
    float zoomDelta = 135;
    float zoomStart;
    float accumulator = FLT_MAX;

    void update(float delta) override {
        ExampleBase::update(delta);
        if (!paused) {
            accumulator += delta;
            if (accumulator < duration) {
                camera.position.z = easings::inOutQuint(accumulator, duration, zoomStart, zoomDelta);
                camera.setTranslation(camera.position);
                updateUniformBuffer(true);
            } else {
                updateUniformBuffer(false);
            }

            if (accumulator >= interval) {
                accumulator = 0;
                zoomStart = camera.position.z;
                if (camera.position.z < -2) {
                    zoomDelta = 135;
                } else {
                    zoomDelta = -135;
                }
            }
        }
    }

    virtual void viewChanged() {
        updateUniformBuffer(true);
    }
};

RUN_EXAMPLE(VulkanExample)
