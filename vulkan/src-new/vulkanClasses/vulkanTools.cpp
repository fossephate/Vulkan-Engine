/*
* Assorted commonly used Vulkan helper functions
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanTools.h"
#include <iterator>
#include <iostream>
#include <fstream>

namespace vkx {

    vk::Bool32 checkGlobalExtensionPresent(const char* extensionName) {
        uint32_t extensionCount = 0;
        std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();
        for (auto& ext : extensions) {
            if (!strcmp(extensionName, ext.extensionName)) {
                return true;
            }
        }
        return false;
    }

    vk::Bool32 checkDeviceExtensionPresent(vk::PhysicalDevice physicalDevice, const char* extensionName) {
        uint32_t extensionCount = 0;
        std::vector<vk::ExtensionProperties> extensions = physicalDevice.enumerateDeviceExtensionProperties();
        for (auto& ext : extensions) {
            if (!strcmp(extensionName, ext.extensionName)) {
                return true;
            }
        }
        return false;
    }

    vk::Format getSupportedDepthFormat(vk::PhysicalDevice physicalDevice) {
        // Since all depth formats may be optional, we need to find a suitable depth format to use
        // Start with the highest precision packed format
        std::vector<vk::Format> depthFormats = {
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD32Sfloat,
            vk::Format::eD24UnormS8Uint,
            vk::Format::eD16UnormS8Uint,
            vk::Format::eD16Unorm
        };

        for (auto& format : depthFormats) {
            vk::FormatProperties formatProps;
            formatProps = physicalDevice.getFormatProperties(format);
            // vk::Format must support depth stencil attachment for optimal tiling
            if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                return format;
            }
        }

        throw std::runtime_error("No supported depth format");
    }


    vk::AccessFlags accessFlagsForLayout(vk::ImageLayout layout) {
        switch (layout) {
        case vk::ImageLayout::ePreinitialized:
            return vk::AccessFlagBits::eHostWrite;
        case vk::ImageLayout::eTransferDstOptimal:
            return vk::AccessFlagBits::eTransferWrite;
        case vk::ImageLayout::eTransferSrcOptimal:
            return vk::AccessFlagBits::eTransferRead;
        case vk::ImageLayout::eColorAttachmentOptimal:
            return vk::AccessFlagBits::eColorAttachmentWrite;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return vk::AccessFlagBits::eShaderRead;
        default:
            return vk::AccessFlags();
        }
    }

// Create an image memory barrier for changing the layout of
// an image and put it into an active command buffer
// See chapter 11.4 "vk::Image Layout" for details

void setImageLayout(
    vk::CommandBuffer cmdbuffer,
    vk::Image image,
    vk::ImageAspectFlags aspectMask,
    vk::ImageLayout oldImageLayout,
    vk::ImageLayout newImageLayout,
    vk::ImageSubresourceRange subresourceRange) {

    // Create an image barrier object
    vk::ImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    imageMemoryBarrier.srcAccessMask = accessFlagsForLayout(oldImageLayout);
    imageMemoryBarrier.dstAccessMask = accessFlagsForLayout(newImageLayout);

    // Put barrier on top
    // Put barrier inside setup command buffer
    cmdbuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::DependencyFlags(),
        nullptr, nullptr, imageMemoryBarrier);
}

// Fixed sub resource on first mip level and layer
void setImageLayout(
    vk::CommandBuffer cmdbuffer,
    vk::Image image,
    vk::ImageAspectFlags aspectMask,
    vk::ImageLayout oldImageLayout,
    vk::ImageLayout newImageLayout) {
    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = aspectMask;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;
    setImageLayout(cmdbuffer, image, aspectMask, oldImageLayout, newImageLayout, subresourceRange);
}

void exitFatal(std::string message, std::string caption) {
#ifdef _WIN32
    MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
#else
    // TODO : Linux
#endif
    std::cerr << message << "\n";
    exit(1);
}

std::vector<uint8_t> readBinaryFile(const std::string& filename) {
    // open the file:
    std::ifstream file(filename, std::ios::binary);
    // Stop eating new lines in binary mode!!!
    file.unsetf(std::ios::skipws);

    // get its size:
    std::streampos fileSize;

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // reserve capacity
    std::vector<uint8_t> vec;
    vec.reserve(fileSize);

    // read the data:
    vec.insert(vec.begin(),
        std::istream_iterator<uint8_t>(file),
        std::istream_iterator<uint8_t>());

    return vec;
}

std::string readTextFile(const std::string& fileName) {
    std::string fileContent;
    std::ifstream fileStream(fileName, std::ios::in);

    if (!fileStream.is_open()) {
        throw std::runtime_error("File " + fileName + " not found");
    }
    std::string line = "";
    while (!fileStream.eof()) {
        getline(fileStream, line);
        fileContent.append(line + "\n");
    }
    fileStream.close();
    return fileContent;
}

#if defined(__ANDROID__)
// Android shaders are stored as assets in the apk
// So they need to be loaded via the asset manager
vk::ShaderModule loadShader(AAssetManager* assetManager, const char *fileName, vk::Device device, vk::ShaderStageFlagBits stage) {
    // Load shader from compressed asset
    AAsset* asset = AAssetManager_open(assetManager, fileName, AASSET_MODE_STREAMING);
    assert(asset);
    size_t size = AAsset_getLength(asset);
    assert(size > 0);

    char *shaderCode = new char[size];
    AAsset_read(asset, shaderCode, size);
    AAsset_close(asset);

    vk::ShaderModule shaderModule;
    vk::ShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = (uint32_t*)shaderCode;
    moduleCreateInfo.flags = 0;

    shaderModule = device.createShaderModule(moduleCreateInfo, NULL);

    delete[] shaderCode;

    return shaderModule;
}
#else
vk::ShaderModule loadShader(const std::string& filename, vk::Device device, vk::ShaderStageFlagBits stage) {
    std::vector<uint8_t> binaryData = readBinaryFile(filename);
    vk::ShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.codeSize = binaryData.size();
    moduleCreateInfo.pCode = (uint32_t*)binaryData.data();
    return device.createShaderModule(moduleCreateInfo);
}
#endif

vk::ShaderModule loadShaderGLSL(const std::string& filename, vk::Device device, vk::ShaderStageFlagBits stage) {
    std::string shaderSrc = readTextFile(filename);

    vk::ShaderModule shaderModule;
    vk::ShaderModuleCreateInfo moduleCreateInfo;
    std::vector<uint8_t> textData;
    moduleCreateInfo.codeSize = 3 * sizeof(uint32_t) + shaderSrc.size() + 2;
    textData.resize(moduleCreateInfo.codeSize);
    uint32_t* textDataPointer{ nullptr };
    moduleCreateInfo.pCode = textDataPointer = (uint32_t *)textData.data();

    // Magic SPV number
    textDataPointer[0] = 0x07230203;
    textDataPointer[1] = 0;
    textDataPointer[2] = (uint32_t)stage;
    memcpy(textDataPointer + 3, shaderSrc.data(), shaderSrc.size());
    textData[moduleCreateInfo.codeSize - 1] = 0;
    textData[moduleCreateInfo.codeSize - 2] = 0;
    shaderModule = device.createShaderModule(moduleCreateInfo, NULL);
    return shaderModule;
}

vk::CommandBufferAllocateInfo commandBufferAllocateInfo(vk::CommandPool commandPool, vk::CommandBufferLevel level, uint32_t bufferCount) {
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = level;
    commandBufferAllocateInfo.commandBufferCount = bufferCount;
    return commandBufferAllocateInfo;
}

vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlags flags) {
    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.flags = flags;
    return fenceCreateInfo;
}

vk::Viewport viewport(
    float width,
    float height,
    float minDepth,
    float maxDepth) {
    vk::Viewport viewport;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    return viewport;
}

vk::Viewport viewport(
    const glm::uvec2& size,
    float minDepth,
    float maxDepth) {
    return viewport((float)size.x, (float)size.y, minDepth, maxDepth);
}

vk::Viewport viewport(
    const vk::Extent2D& size,
    float minDepth,
    float maxDepth) {
    return viewport((float)size.width, (float)size.height, minDepth, maxDepth);
}


vk::Rect2D rect2D(
    uint32_t width,
    uint32_t height,
    int32_t offsetX,
    int32_t offsetY) {
    vk::Rect2D rect2D;
    rect2D.extent.width = width;
    rect2D.extent.height = height;
    rect2D.offset.x = offsetX;
    rect2D.offset.y = offsetY;
    return rect2D;
}

vk::Rect2D rect2D(const glm::uvec2& size, const glm::ivec2& offset) {
    return rect2D(size.x, size.y, offset.x, offset.y);
}

vk::Rect2D rect2D(const vk::Extent2D& size, const vk::Offset2D& offset) {
    return rect2D(size.width, size.height, offset.x, offset.y);
}

vk::BufferCreateInfo bufferCreateInfo(
    vk::BufferUsageFlags usage,
    vk::DeviceSize size) {
    vk::BufferCreateInfo bufCreateInfo;
    bufCreateInfo.usage = usage;
    bufCreateInfo.size = size;
    return bufCreateInfo;
}

vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(
    uint32_t poolSizeCount,
    vk::DescriptorPoolSize* pPoolSizes,
    uint32_t maxSets) {
    vk::DescriptorPoolCreateInfo descriptorPoolInfo;
    descriptorPoolInfo.poolSizeCount = poolSizeCount;
    descriptorPoolInfo.pPoolSizes = pPoolSizes;
    descriptorPoolInfo.maxSets = maxSets;
    return descriptorPoolInfo;
}

vk::DescriptorPoolSize descriptorPoolSize(
    vk::DescriptorType type,
    uint32_t descriptorCount) {
    vk::DescriptorPoolSize descriptorPoolSize;
    descriptorPoolSize.type = type;
    descriptorPoolSize.descriptorCount = descriptorCount;
    return descriptorPoolSize;
}

vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding(
    vk::DescriptorType type,
    vk::ShaderStageFlags stageFlags,
    uint32_t binding) {
    vk::DescriptorSetLayoutBinding setLayoutBinding;
    setLayoutBinding.descriptorType = type;
    setLayoutBinding.stageFlags = stageFlags;
    setLayoutBinding.binding = binding;
    // Default value in all examples
    setLayoutBinding.descriptorCount = 1;
    return setLayoutBinding;
}

vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
    const vk::DescriptorSetLayoutBinding* pBindings,
    uint32_t bindingCount) {
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.pBindings = pBindings;
    descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
    return descriptorSetLayoutCreateInfo;
}

vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
    const vk::DescriptorSetLayout* pSetLayouts,
    uint32_t setLayoutCount) {
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
    pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
    return pipelineLayoutCreateInfo;
}

vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(
    vk::DescriptorPool descriptorPool,
    const vk::DescriptorSetLayout* pSetLayouts,
    uint32_t descriptorSetCount) {
    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
    descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
    return descriptorSetAllocateInfo;
}

vk::DescriptorImageInfo descriptorImageInfo(vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout imageLayout) {
    vk::DescriptorImageInfo descriptorImageInfo;
    descriptorImageInfo.sampler = sampler;
    descriptorImageInfo.imageView = imageView;
    descriptorImageInfo.imageLayout = imageLayout;
    return descriptorImageInfo;
}

vk::WriteDescriptorSet writeDescriptorSet(
    vk::DescriptorSet dstSet,
    vk::DescriptorType type,
    uint32_t binding,
    vk::DescriptorBufferInfo* bufferInfo) {
    vk::WriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pBufferInfo = bufferInfo;
    // Default value in all examples
    writeDescriptorSet.descriptorCount = 1;
    return writeDescriptorSet;
}

vk::WriteDescriptorSet writeDescriptorSet(
    vk::DescriptorSet dstSet,
    vk::DescriptorType type,
    uint32_t binding,
    vk::DescriptorImageInfo * imageInfo) {
    vk::WriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pImageInfo = imageInfo;
    // Default value in all examples
    writeDescriptorSet.descriptorCount = 1;
    return writeDescriptorSet;
}

vk::VertexInputBindingDescription vertexInputBindingDescription(
    uint32_t binding,
    uint32_t stride,
    vk::VertexInputRate inputRate) {
    vk::VertexInputBindingDescription vInputBindDescription;
    vInputBindDescription.binding = binding;
    vInputBindDescription.stride = stride;
    vInputBindDescription.inputRate = inputRate;
    return vInputBindDescription;
}

vk::VertexInputAttributeDescription vertexInputAttributeDescription(
    uint32_t binding,
    uint32_t location,
    vk::Format format,
    uint32_t offset) {
    vk::VertexInputAttributeDescription vInputAttribDescription;
    vInputAttribDescription.location = location;
    vInputAttribDescription.binding = binding;
    vInputAttribDescription.format = format;
    vInputAttribDescription.offset = offset;
    return vInputAttribDescription;
}

vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
    vk::PrimitiveTopology topology,
    vk::PipelineInputAssemblyStateCreateFlags flags,
    vk::Bool32 primitiveRestartEnable) {
    vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
    pipelineInputAssemblyStateCreateInfo.topology = topology;
    pipelineInputAssemblyStateCreateInfo.flags = flags;
    pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
    return pipelineInputAssemblyStateCreateInfo;
}

vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
    vk::PolygonMode polygonMode,
    vk::CullModeFlags cullMode,
    vk::FrontFace frontFace,
    vk::PipelineRasterizationStateCreateFlags flags) {
    vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
    pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
    pipelineRasterizationStateCreateInfo.cullMode = cullMode;
    pipelineRasterizationStateCreateInfo.frontFace = frontFace;
    pipelineRasterizationStateCreateInfo.flags = flags;
    pipelineRasterizationStateCreateInfo.depthClampEnable = VK_TRUE;
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
    return pipelineRasterizationStateCreateInfo;
}

vk::ColorComponentFlags fullColorWriteMask() {
    return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
}

vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
    vk::ColorComponentFlags colorWriteMask,
    vk::Bool32 blendEnable) {
    vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
    pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
    pipelineColorBlendAttachmentState.blendEnable = blendEnable;
    return pipelineColorBlendAttachmentState;
}

vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
    uint32_t attachmentCount,
    const vk::PipelineColorBlendAttachmentState * pAttachments) {
    vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
    pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
    pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
    return pipelineColorBlendStateCreateInfo;
}

vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
    vk::Bool32 depthTestEnable,
    vk::Bool32 depthWriteEnable,
    vk::CompareOp depthCompareOp) {
    vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo;
    pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
    pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
    pipelineDepthStencilStateCreateInfo.front = pipelineDepthStencilStateCreateInfo.back;
    pipelineDepthStencilStateCreateInfo.back.compareOp = vk::CompareOp::eAlways;
    return pipelineDepthStencilStateCreateInfo;
}

vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
    uint32_t viewportCount,
    uint32_t scissorCount,
    vk::PipelineViewportStateCreateFlags flags) {
    vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
    pipelineViewportStateCreateInfo.viewportCount = viewportCount;
    pipelineViewportStateCreateInfo.scissorCount = scissorCount;
    pipelineViewportStateCreateInfo.flags = flags;
    return pipelineViewportStateCreateInfo;
}

vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
    vk::SampleCountFlagBits rasterizationSamples,
    vk::PipelineMultisampleStateCreateFlags flags) {
    vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
    return pipelineMultisampleStateCreateInfo;
}

vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
    const vk::DynamicState * pDynamicStates,
    uint32_t dynamicStateCount,
    vk::PipelineDynamicStateCreateFlags flags) {
    vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
    pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
    pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
    return pipelineDynamicStateCreateInfo;
}

vk::PipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(uint32_t patchControlPoints) {
    vk::PipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo;
    pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
    return pipelineTessellationStateCreateInfo;
}

vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
    vk::PipelineLayout layout,
    vk::RenderPass renderPass,
    vk::PipelineCreateFlags flags) {
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.layout = layout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.flags = flags;
    return pipelineCreateInfo;
}

vk::ComputePipelineCreateInfo computePipelineCreateInfo(vk::PipelineLayout layout, vk::PipelineCreateFlags flags) {
    vk::ComputePipelineCreateInfo computePipelineCreateInfo;
    computePipelineCreateInfo.layout = layout;
    computePipelineCreateInfo.flags = flags;
    return computePipelineCreateInfo;
}

vk::PushConstantRange pushConstantRange(
    vk::ShaderStageFlags stageFlags,
    uint32_t size,
    uint32_t offset) {
    vk::PushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = stageFlags;
    pushConstantRange.offset = offset;
    pushConstantRange.size = size;
    return pushConstantRange;
}

vk::ClearColorValue clearColor(const glm::vec4& v = glm::vec4(0)) {
    vk::ClearColorValue result;
    memcpy(&result.float32, &v, sizeof(result.float32));
    return result;
}

const std::string& getAssetPath() {
#if defined(__ANDROID__)
    return "";
#else
    static std::string path;
    static std::once_flag once;

    std::call_once(once, [] {
        std::string file(__FILE__);
        std::replace(file.begin(), file.end(), '\\', '/');
        std::string::size_type lastSlash = file.rfind("/");
        file = file.substr(0, lastSlash);
        path = file + "/../data/";
    });
    return path;
#endif
    }

}

