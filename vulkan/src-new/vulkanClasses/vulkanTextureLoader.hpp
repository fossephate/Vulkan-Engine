/*
* Texture loader for Vulkan
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vulkan/vulkan.h>
#pragma warning(disable: 4996 4244 4267)
#include <gli/gli.hpp>
#include "vulkanTools.h"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

namespace vkx {

    struct Texture {
        vk::Device device;
        vk::Image image;
        vk::DeviceMemory memory;
        vk::Sampler sampler;
        vk::ImageLayout imageLayout{ vk::ImageLayout::eShaderReadOnlyOptimal };
        vk::ImageView view;
        vk::Extent3D extent{ 0, 0, 1 };
        vk::DescriptorImageInfo descriptor;

        uint32_t mipLevels{ 1 };
        uint32_t layerCount{ 1 };

        Texture& operator=(const vkx::CreateImageResult& created) {
            device = created.device;
            image = created.image;
            memory = created.memory;
            return *this;
        }

        void destroy() {
            if (sampler) {
                device.destroySampler(sampler);
                sampler = vk::Sampler();
            }
            if (view) {
                device.destroyImageView(view);
                view = vk::ImageView();
            }
            if (image) {
                device.destroyImage(image);
                image = vk::Image();
            }
            if (memory) {
                device.freeMemory(memory);
                memory = vk::DeviceMemory();
            }
        }
    };

    class TextureLoader {
    private:
        Context context;
        vk::CommandBuffer cmdBuffer;

    public:

        TextureLoader(const Context& context) {
            this->context = context;

            // Create command buffer for submitting image barriers
            // and converting tilings
            vk::CommandBufferAllocateInfo cmdBufInfo;
            cmdBufInfo.commandPool = context.getCommandPool();
            cmdBufInfo.level = vk::CommandBufferLevel::ePrimary;
            cmdBufInfo.commandBufferCount = 1;

            cmdBuffer = context.device.allocateCommandBuffers(cmdBufInfo)[0];
        }

        ~TextureLoader() {
            context.device.freeCommandBuffers(context.getCommandPool(), cmdBuffer);
        }

#if defined(__ANDROID__)
        AAssetManager* assetManager = nullptr;
#endif

        // Load a 2D texture
        Texture loadTexture(const std::string& filename, vk::Format format, bool forceLinear = false, vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled) {
#if defined(__ANDROID__)
            assert(assetManager != nullptr);

            // Textures are stored inside the apk on Android (compressed)
            // So they need to be loaded via the asset manager
            AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
            assert(asset);
            size_t size = AAsset_getLength(asset);
            assert(size > 0);

            void *textureData = malloc(size);
            AAsset_read(asset, textureData, size);
            AAsset_close(asset);

            gli::texture2D tex2D(gli::load((const char*)textureData, size));

            free(textureData);
#else
            gli::texture2D tex2D(gli::load(filename.c_str()));
#endif        
            assert(!tex2D.empty());

            Texture texture;
            texture.device = context.device;
            texture.extent.width = (uint32_t)tex2D[0].dimensions().x;
            texture.extent.height = (uint32_t)tex2D[0].dimensions().y;
            texture.mipLevels = tex2D.levels();

            // Get device properites for the requested texture format
            vk::FormatProperties formatProperties;
            formatProperties = context.physicalDevice.getFormatProperties(format);

            // Only use linear tiling if requested (and supported by the device)
            // Support for linear tiling is mostly limited, so prefer to use
            // optimal tiling instead
            // On most implementations linear tiling will only support a very
            // limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
            vk::Bool32 useStaging = !forceLinear;

            // Use a separate command buffer for texture loading
            vk::CommandBufferBeginInfo cmdBufInfo;
            cmdBuffer.begin(cmdBufInfo);

            vk::ImageCreateInfo imageCreateInfo;
            imageCreateInfo.imageType = vk::ImageType::e2D;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.format = format;
            imageCreateInfo.extent = texture.extent;
            imageCreateInfo.usage = vk::ImageUsageFlagBits::eSampled;
            imageCreateInfo.initialLayout = vk::ImageLayout::ePreinitialized;

            if (useStaging) {
                // Create a host-visible staging buffer that contains the raw image data
                // Copy texture data into staging buffer
                auto staging = context.createBuffer(vk::BufferUsageFlagBits::eTransferSrc, tex2D);

                // Setup buffer copy regions for each mip level
                std::vector<vk::BufferImageCopy> bufferCopyRegions;
                uint32_t offset = 0;

                vk::BufferImageCopy bufferCopyRegion;
                bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.depth = 1;

                for (uint32_t i = 0; i < texture.mipLevels; i++) {
                    bufferCopyRegion.imageExtent.width = tex2D[i].dimensions().x;
                    bufferCopyRegion.imageExtent.height = tex2D[i].dimensions().y;
                    bufferCopyRegion.imageSubresource.mipLevel = i;
                    bufferCopyRegion.bufferOffset = offset;
                    bufferCopyRegions.push_back(bufferCopyRegion);
                    offset += tex2D[i].size();
                }

                // Create optimal tiled target image
                imageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferDst | imageUsageFlags;
                imageCreateInfo.mipLevels = texture.mipLevels;

                texture = context.createImage(imageCreateInfo, vk::MemoryPropertyFlagBits::eDeviceLocal);

                vk::ImageSubresourceRange subresourceRange;
                subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                subresourceRange.levelCount = texture.mipLevels;
                subresourceRange.layerCount = 1;

                // vk::Image barrier for optimal image (target)
                // Optimal image will be used as destination for the copy
                setImageLayout(
                    cmdBuffer,
                    texture.image,
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageLayout::ePreinitialized,
                    vk::ImageLayout::eTransferDstOptimal,
                    subresourceRange);

                // Copy mip levels from staging buffer
                cmdBuffer.copyBufferToImage(staging.buffer, texture.image, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);
                // Change texture image layout to shader read after all mip levels have been copied
                setImageLayout(
                    cmdBuffer,
                    texture.image,
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageLayout::eTransferDstOptimal,
                    texture.imageLayout,
                    subresourceRange);

                // Submit command buffer containing copy and image layout commands
                cmdBuffer.end();

                // Create a fence to make sure that the copies have finished before continuing
                vk::Fence copyFence;
                copyFence = context.device.createFence(vk::FenceCreateInfo());

                vk::SubmitInfo submitInfo;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &cmdBuffer;

                context.queue.submit(submitInfo, copyFence);
                context.device.waitForFences(copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
                context.device.destroyFence(copyFence);
                staging.destroy();

            } else {
                // Prefer using optimal tiling, as linear tiling 
                // may support only a small set of features 
                // depending on implementation (e.g. no mip maps, only one layer, etc.)

                // Check if this support is supported for linear tiling
                assert(formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);

                imageCreateInfo.mipLevels = 1;
                imageCreateInfo.tiling = vk::ImageTiling::eLinear;
                imageCreateInfo.initialLayout = vk::ImageLayout::ePreinitialized;

                auto mappable = context.createImage(imageCreateInfo, vk::MemoryPropertyFlagBits::eHostVisible);

                // Get sub resource layout
                // Mip map count, array layer, etc.
                vk::ImageSubresource subRes;
                subRes.aspectMask = vk::ImageAspectFlagBits::eColor;
                subRes.mipLevel = 0;

                // Map image memory
                mappable.map();
                // Copy image data into memory
                mappable.copy(tex2D[0].size(), tex2D[0].data());
                mappable.unmap();

                // Linear tiled images don't need to be staged
                // and can be directly used as textures
                texture = mappable;

                // Setup image memory barrier
                setImageLayout(
                    cmdBuffer,
                    texture.image,
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageLayout::ePreinitialized,
                    texture.imageLayout);

                // Submit command buffer containing copy and image layout commands
                cmdBuffer.end();

                vk::Fence nullFence = { VK_NULL_HANDLE };

                vk::SubmitInfo submitInfo;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &cmdBuffer;
                context.queue.submit(submitInfo, nullFence);
                context.queue.waitIdle();
            }

            // Create sampler
            {
                vk::SamplerCreateInfo sampler;
                sampler.magFilter = vk::Filter::eLinear;
                sampler.minFilter = vk::Filter::eLinear;
                sampler.mipmapMode = vk::SamplerMipmapMode::eLinear;
                // Max level-of-detail should match mip level count
                sampler.maxLod = (useStaging) ? (float)texture.mipLevels : 0.0f;
                // Enable anisotropic filtering
                sampler.maxAnisotropy = 8;
                sampler.anisotropyEnable = VK_TRUE;
                sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
                texture.sampler = context.device.createSampler(sampler);
            }

            // Create image view
            // Textures are not directly accessed by the shaders and
            // are abstracted by image views containing additional
            // information and sub resource ranges
            {
                vk::ImageViewCreateInfo view;
                view.viewType = vk::ImageViewType::e2D;
                view.format = format;
                view.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
                // Linear tiling usually won't support mip maps
                // Only set mip map count if optimal tiling is used
                view.subresourceRange.levelCount = (useStaging) ? texture.mipLevels : 1;
                view.image = texture.image;
                texture.view = context.device.createImageView(view);
            }
            texture.descriptor.imageLayout = texture.imageLayout;
            texture.descriptor.imageView = texture.view;
            texture.descriptor.sampler = texture.sampler;
            return texture;
        }

        // Load a cubemap texture (single file)
        Texture loadCubemap(const std::string& filename, vk::Format format) {
#if defined(__ANDROID__)
            assert(assetManager != nullptr);

            // Textures are stored inside the apk on Android (compressed)
            // So they need to be loaded via the asset manager
            AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
            assert(asset);
            size_t size = AAsset_getLength(asset);
            assert(size > 0);

            void *textureData = malloc(size);
            AAsset_read(asset, textureData, size);
            AAsset_close(asset);

            gli::textureCube texCube(gli::load((const char*)textureData, size));

            free(textureData);
#else
            gli::textureCube texCube(gli::load(filename));
#endif    
            Texture texture;
            assert(!texCube.empty());
            texture.extent.width = (uint32_t)texCube[0].dimensions().x;
            texture.extent.height = (uint32_t)texCube[0].dimensions().y;
            texture.mipLevels = texCube.levels();
            texture.layerCount = 6;

            // Create optimal tiled target image
            vk::ImageCreateInfo imageCreateInfo;
            imageCreateInfo.imageType = vk::ImageType::e2D;
            imageCreateInfo.format = format;
            imageCreateInfo.mipLevels = texture.mipLevels;
            imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
            imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
            imageCreateInfo.usage = vk::ImageUsageFlagBits::eSampled;
            imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
            imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
            imageCreateInfo.extent = texture.extent;
            imageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            // Cube faces count as array layers in Vulkan
            imageCreateInfo.arrayLayers = 6;
            // This flag is required for cube map images
            imageCreateInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
            texture = context.createImage(imageCreateInfo, vk::MemoryPropertyFlagBits::eDeviceLocal);

            auto staging = context.createBuffer(vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible, texCube.size(), texCube.data());

            // Setup buffer copy regions for the cube faces
            // As all faces of a cube map must have the same dimensions, we can do a single copy
            vk::BufferImageCopy bufferCopyRegion;
            bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 6;
            bufferCopyRegion.imageExtent = texture.extent;

            context.withPrimaryCommandBuffer([&](const vk::CommandBuffer& cmdBuffer) {
                // vk::Image barrier for optimal image (target)
                // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
                vk::ImageSubresourceRange subresourceRange;
                subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                subresourceRange.baseMipLevel = 0;
                subresourceRange.levelCount = texture.mipLevels;
                subresourceRange.layerCount = 6;
                setImageLayout(
                    cmdBuffer,
                    texture.image,
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eTransferDstOptimal,
                    subresourceRange);
                // Setup buffer copy regions for each face including all of it's miplevels
                std::vector<vk::BufferImageCopy> bufferCopyRegions;
                {
                    vk::BufferImageCopy bufferCopyRegion;
                    bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                    bufferCopyRegion.imageSubresource.layerCount = 1;
                    bufferCopyRegion.imageExtent.depth = 1;
                    for (uint32_t face = 0; face < 6; face++) {
                        bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                        for (uint32_t level = 0; level < texture.mipLevels; ++level) {
                            bufferCopyRegion.imageSubresource.mipLevel = level;
                            bufferCopyRegion.imageExtent.width = texCube[face][level].dimensions().x;
                            bufferCopyRegion.imageExtent.height = texCube[face][level].dimensions().y;
                            bufferCopyRegions.push_back(bufferCopyRegion);
                            // Increase offset into staging buffer for next level / face
                            bufferCopyRegion.bufferOffset += texCube[face][level].size();
                        }
                    }
                }

                // Copy the cube map faces from the staging buffer to the optimal tiled image
                cmdBuffer.copyBufferToImage(staging.buffer, texture.image, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);
                // Change texture image layout to shader read after all faces have been copied
                texture.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                setImageLayout(
                    cmdBuffer,
                    texture.image,
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    subresourceRange);
            });

            // Create sampler
            vk::SamplerCreateInfo sampler;
            sampler.magFilter = vk::Filter::eLinear;
            sampler.minFilter = vk::Filter::eLinear;
            sampler.mipmapMode = vk::SamplerMipmapMode::eLinear;
            sampler.addressModeU = vk::SamplerAddressMode::eClampToEdge;
            sampler.addressModeV = sampler.addressModeU;
            sampler.addressModeW = sampler.addressModeU;
            sampler.maxAnisotropy = 8.0f;
            sampler.maxLod = texture.mipLevels;
            sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
            texture.sampler = context.device.createSampler(sampler);

            // Create image view
            vk::ImageViewCreateInfo view;
            view.viewType = vk::ImageViewType::eCube;
            view.format = format;
            view.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
            view.subresourceRange.layerCount = 6;
            view.image = texture.image;
            texture.view = context.device.createImageView(view);
            // Clean up staging resources
            staging.destroy();
            return texture;
        }

        // Load an array texture (single file)
        Texture loadTextureArray(const std::string& filename, vk::Format format) {
#if defined(__ANDROID__)
            assert(assetManager != nullptr);

            // Textures are stored inside the apk on Android (compressed)
            // So they need to be loaded via the asset manager
            AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
            assert(asset);
            size_t size = AAsset_getLength(asset);
            assert(size > 0);

            void *textureData = malloc(size);
            AAsset_read(asset, textureData, size);
            AAsset_close(asset);

            gli::texture2DArray tex2DArray(gli::load((const char*)textureData, size));

            free(textureData);
#else
            gli::texture2DArray tex2DArray(gli::load(filename));
#endif    

            Texture texture;
            assert(!tex2DArray.empty());

            texture.extent.width = tex2DArray.dimensions().x;
            texture.extent.height = tex2DArray.dimensions().y;
            texture.layerCount = tex2DArray.layers();

            // This buffer is used as a transfer source for the buffer copy
            auto staging = context.createBuffer(vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible, tex2DArray.size(), tex2DArray.data());

            // Setup buffer copy regions for array layers
            std::vector<vk::BufferImageCopy> bufferCopyRegions;
            uint32_t offset = 0;

            // Check if all array layers have the same dimesions
            bool sameDims = true;
            for (uint32_t layer = 0; layer < texture.layerCount; layer++) {
                if (tex2DArray[layer].dimensions().x != texture.extent.width || tex2DArray[layer].dimensions().y != texture.extent.height) {
                    sameDims = false;
                    break;
                }
            }

            // If all layers of the texture array have the same dimensions, we only need to do one copy
            if (sameDims) {
                vk::BufferImageCopy bufferCopyRegion;
                bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                bufferCopyRegion.imageSubresource.mipLevel = 0;
                bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
                bufferCopyRegion.imageSubresource.layerCount = texture.layerCount;
                bufferCopyRegion.imageExtent.width = tex2DArray[0].dimensions().x;
                bufferCopyRegion.imageExtent.height = tex2DArray[0].dimensions().y;
                bufferCopyRegion.imageExtent.depth = 1;
                bufferCopyRegion.bufferOffset = offset;

                bufferCopyRegions.push_back(bufferCopyRegion);
            } else {
                // If dimensions differ, copy layer by layer and pass offsets
                for (uint32_t layer = 0; layer < texture.layerCount; layer++) {
                    vk::BufferImageCopy bufferCopyRegion;
                    bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                    bufferCopyRegion.imageSubresource.mipLevel = 0;
                    bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
                    bufferCopyRegion.imageSubresource.layerCount = 1;
                    bufferCopyRegion.imageExtent.width = tex2DArray[layer].dimensions().x;
                    bufferCopyRegion.imageExtent.height = tex2DArray[layer].dimensions().y;
                    bufferCopyRegion.imageExtent.depth = 1;
                    bufferCopyRegion.bufferOffset = offset;

                    bufferCopyRegions.push_back(bufferCopyRegion);

                    offset += tex2DArray[layer].size();
                }
            }

            // Create optimal tiled target image
            vk::ImageCreateInfo imageCreateInfo;
            imageCreateInfo.imageType = vk::ImageType::e2D;
            imageCreateInfo.format = format;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
            imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
            imageCreateInfo.usage = vk::ImageUsageFlagBits::eSampled;
            imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
            imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
            imageCreateInfo.extent = texture.extent;
            imageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            imageCreateInfo.arrayLayers = texture.layerCount;

            texture = context.createImage(imageCreateInfo, vk::MemoryPropertyFlagBits::eDeviceLocal);

            vk::CommandBufferBeginInfo cmdBufInfo;
            cmdBuffer.begin(cmdBufInfo);

            // vk::Image barrier for optimal image (target)
            // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
            vk::ImageSubresourceRange subresourceRange;
            subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = texture.layerCount;

            setImageLayout(
                cmdBuffer,
                texture.image,
                vk::ImageAspectFlagBits::eColor,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                subresourceRange);

            // Copy the cube map faces from the staging buffer to the optimal tiled image
            cmdBuffer.copyBufferToImage(staging.buffer, texture.image, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);

            // Change texture image layout to shader read after all faces have been copied
            texture.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            setImageLayout(
                cmdBuffer,
                texture.image,
                vk::ImageAspectFlagBits::eColor,
                vk::ImageLayout::eTransferDstOptimal,
                texture.imageLayout,
                subresourceRange);

            cmdBuffer.end();

            // Create a fence to make sure that the copies have finished before continuing
            vk::Fence copyFence;
            vk::FenceCreateInfo fenceCreateInfo;
            copyFence = context.device.createFence(fenceCreateInfo);

            vk::SubmitInfo submitInfo;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBuffer;

            context.queue.submit(submitInfo, copyFence);

            context.device.waitForFences(copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

            context.device.destroyFence(copyFence);

            // Create sampler
            vk::SamplerCreateInfo sampler;
            sampler.magFilter = vk::Filter::eLinear;
            sampler.minFilter = vk::Filter::eLinear;
            sampler.mipmapMode = vk::SamplerMipmapMode::eLinear;
            sampler.addressModeU = vk::SamplerAddressMode::eClampToEdge;
            sampler.addressModeV = sampler.addressModeU;
            sampler.addressModeW = sampler.addressModeU;
            sampler.mipLodBias = 0.0f;
            sampler.maxAnisotropy = 8;
            sampler.compareOp = vk::CompareOp::eNever;
            sampler.minLod = 0.0f;
            sampler.maxLod = 0.0f;
            sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
            texture.sampler = context.device.createSampler(sampler);

            // Create image view
            vk::ImageViewCreateInfo view;
            view.image;
            view.viewType = vk::ImageViewType::e2DArray;
            view.format = format;
            view.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
            view.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
            view.subresourceRange.layerCount = texture.layerCount;
            view.image = texture.image;
            texture.view = context.device.createImageView(view);

            // Clean up staging resources
            staging.destroy();
            return texture;
        }
    };
}
