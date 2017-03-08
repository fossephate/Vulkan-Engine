#include "vulkanTextureLoader.h"

void vkx::Texture::destroy() {
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

//vkx::TextureLoader::TextureLoader(const Context &context) {
//	this->context = &context;
//
//	// Create command buffer for submitting image barriers
//	// and converting tilings
//	vk::CommandBufferAllocateInfo cmdBufInfo;
//	cmdBufInfo.commandPool = context.getCommandPool();
//	cmdBufInfo.level = vk::CommandBufferLevel::ePrimary;
//	cmdBufInfo.commandBufferCount = 1;
//
//	cmdBuffer = context.device.allocateCommandBuffers(cmdBufInfo)[0];
//}


vkx::TextureLoader::TextureLoader(const Context &context)
	: context(context)
{

	// Create command buffer for submitting image barriers
	// and converting tilings
	vk::CommandBufferAllocateInfo cmdBufInfo;
	cmdBufInfo.commandPool = context.getCommandPool();
	cmdBufInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufInfo.commandBufferCount = 1;

	cmdBuffer = context.device.allocateCommandBuffers(cmdBufInfo)[0];
}

vkx::TextureLoader::TextureLoader(const Context &context, vk::Queue queue, vk::CommandPool cmdPool)
	: context(context)
{

	// Create command buffer for submitting image barriers
	// and converting tilings
	vk::CommandBufferAllocateInfo cmdBufInfo;
	cmdBufInfo.commandPool = context.getCommandPool();
	cmdBufInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufInfo.commandBufferCount = 1;

	cmdBuffer = context.device.allocateCommandBuffers(cmdBufInfo)[0];

	this->queue = queue;
	this->cmdPool = cmdPool;
}

//vkx::TextureLoader::TextureLoader(Context *context)
//{
//
//	// Create command buffer for submitting image barriers
//	// and converting tilings
//	vk::CommandBufferAllocateInfo cmdBufInfo;
//	cmdBufInfo.commandPool = context.getCommandPool();
//	cmdBufInfo.level = vk::CommandBufferLevel::ePrimary;
//	cmdBufInfo.commandBufferCount = 1;
//
//	cmdBuffer = context.device.allocateCommandBuffers(cmdBufInfo)[0];
//}

vkx::TextureLoader::~TextureLoader() {
	context.device.freeCommandBuffers(context.getCommandPool(), cmdBuffer);
}

// Load a 2D texture

vkx::Texture vkx::TextureLoader::loadTexture(const std::string & filename, vk::Format format, bool forceLinear, vk::ImageUsageFlags imageUsageFlags) {
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

	gli::texture2d tex2D(gli::load((const char*)textureData, size));

	free(textureData);
	#else

	std::string ext = filename.substr(filename.length()-3, 3);
	std::string filenameKTX = filename;
	if (ext == "png") {
		filenameKTX = filename.substr(0, filename.length() - 3);
		filenameKTX = filenameKTX + "ktx";
	}

	gli::texture2d tex2D(gli::load(filenameKTX.c_str()));

	#endif
	assert(!tex2D.empty());

	Texture texture;
	texture.device = this->context.device;

	texture.extent.width = (uint32_t)tex2D[0].extent().x;
	texture.extent.height = (uint32_t)tex2D[0].extent().y;
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
			bufferCopyRegion.imageExtent.width = tex2D[i].extent().x;
			bufferCopyRegion.imageExtent.height = tex2D[i].extent().y;
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

vkx::Texture vkx::TextureLoader::loadCubemap(const std::string & filename, vk::Format format) {
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
	gli::texture_cube texCube(gli::load(filename));
	#endif    
	Texture texture;
	assert(!texCube.empty());
	texture.extent.width = (uint32_t)texCube[0].extent().x;
	texture.extent.height = (uint32_t)texCube[0].extent().y;
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
					bufferCopyRegion.imageExtent.width = texCube[face][level].extent().x;
					bufferCopyRegion.imageExtent.height = texCube[face][level].extent().y;
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

vkx::Texture vkx::TextureLoader::loadTextureArray(const std::string & filename, vk::Format format) {
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

	gli::texture2dArray tex2DArray(gli::load((const char*)textureData, size));

	free(textureData);
	#else
	gli::texture2d_array tex2DArray(gli::load(filename));
	#endif

	

	Texture texture;
	assert(!tex2DArray.empty());

	texture.extent.width = tex2DArray.extent().x;
	texture.extent.height = tex2DArray.extent().y;
	texture.layerCount = tex2DArray.layers();

	// This buffer is used as a transfer source for the buffer copy
	auto staging = context.createBuffer(vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible, tex2DArray.size(), tex2DArray.data());

	// Setup buffer copy regions for array layers
	std::vector<vk::BufferImageCopy> bufferCopyRegions;
	uint32_t offset = 0;

	// Check if all array layers have the same dimesions
	bool sameDims = true;
	for (uint32_t layer = 0; layer < texture.layerCount; layer++) {
		if (tex2DArray[layer].extent().x != texture.extent.width || tex2DArray[layer].extent().y != texture.extent.height) {
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
		bufferCopyRegion.imageExtent.width = tex2DArray[0].extent().x;
		bufferCopyRegion.imageExtent.height = tex2DArray[0].extent().y;
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
			bufferCopyRegion.imageExtent.width = tex2DArray[layer].extent().x;
			bufferCopyRegion.imageExtent.height = tex2DArray[layer].extent().y;
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















uint32_t getMemoryType(vk::PhysicalDeviceMemoryProperties memoryProperties, uint32_t typeBits, vk::MemoryPropertyFlags properties, vk::Bool32 *memTypeFound = nullptr) {
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((typeBits & 1) == 1) {
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				if (memTypeFound) {
					*memTypeFound = true;
				}
				return i;
			}
		}
		typeBits >>= 1;
	}

#if defined(__ANDROID__)
	//todo : Exceptions are disabled by default on Android (need to add LOCAL_CPP_FEATURES += exceptions to Android.mk), so for now just return zero
	if (memTypeFound) {
		*memTypeFound = false;
	}
	return 0;
#else
	if (memTypeFound) {
		*memTypeFound = false;
		return 0;
	} else {
		throw std::runtime_error("Could not find a matching memory type");
	}
#endif
}


















/**
* Creates a 2D texture from a buffer
*
* @param buffer Buffer containing texture data to upload
* @param bufferSize Size of the buffer in machine units
* @param width Width of the texture to create
* @param hidth Height of the texture to create
* @param format Vulkan format of the image data stored in the file
* @param texture Pointer to the texture object to load the image into
* @param (Optional) filter Texture filtering for the sampler (defaults to VK_FILTER_LINEAR)
* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
*/
void vkx::TextureLoader::createTexture(void* buffer, vk::DeviceSize bufferSize, vk::Format format, uint32_t width, uint32_t height, vkx::Texture *texture, vk::Filter filter, vk::ImageUsageFlags imageUsageFlags) {

	

	assert(buffer);

	//texture->width = width;
	//texture->height = height;
	texture->extent.setWidth(width);
	texture->extent.setHeight(height);
	texture->mipLevels = 1;

	vk::MemoryAllocateInfo memAllocInfo;/* = vkTools::initializers::memoryAllocateInfo();*/
	vk::MemoryRequirements memReqs;

	// Use a separate command buffer for texture loading
	vk::CommandBufferBeginInfo cmdBufInfo;/* = vkTools::initializers::commandBufferBeginInfo();*/
	//VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	cmdBuffer.begin(cmdBufInfo);

	// Create a host-visible staging buffer that contains the raw image data
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingMemory;

	vk::BufferCreateInfo bufferCreateInfo;/* = vkTools::initializers::bufferCreateInfo();*/
	bufferCreateInfo.size = bufferSize;
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	//VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));
	stagingBuffer = context.device.createBuffer(bufferCreateInfo, nullptr);

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	//vkGetBufferMemoryRequirements(vulkanDevice->logicalDevice, stagingBuffer, &memReqs);
	memReqs = context.device.getBufferMemoryRequirements(stagingBuffer);

	memAllocInfo.allocationSize = memReqs.size;
	// Get memory type index for a host visible buffer
	//memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	memAllocInfo.memoryTypeIndex = getMemoryType(context.deviceMemoryProperties, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	


	//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
	stagingMemory = context.device.allocateMemory(memAllocInfo, nullptr);
	//VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice->logicalDevice, stagingBuffer, stagingMemory, 0));
	context.device.bindBufferMemory(stagingBuffer, stagingMemory, 0);
	

	// Copy texture data into staging buffer
	uint8_t *data;
	//VK_CHECK_RESULT(vkMapMemory(vulkanDevice->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
	data = (uint8_t *)context.device.mapMemory(stagingMemory, 0, memReqs.size, vk::MemoryMapFlags());

	memcpy(data, buffer, bufferSize);
	//vkUnmapMemory(vulkanDevice->logicalDevice, stagingMemory);

	vk::BufferImageCopy bufferCopyRegion;
	bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = width;
	bufferCopyRegion.imageExtent.height = height;
	bufferCopyRegion.imageExtent.depth = 1;
	bufferCopyRegion.bufferOffset = 0;

	// Create optimal tiled target image
	vk::ImageCreateInfo imageCreateInfo;
	imageCreateInfo.imageType = vk::ImageType::e2D;
	imageCreateInfo.format = format;
	imageCreateInfo.mipLevels = texture->mipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
	imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
	imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
	/*imageCreateInfo.extent = { texture->width, texture->height, 1 };*/
	imageCreateInfo.extent = texture->extent;
	imageCreateInfo.usage = imageUsageFlags;
	// Ensure that the TRANSFER_DST bit is set for staging
	if (!(imageCreateInfo.usage & vk::ImageUsageFlagBits::eTransferDst)) {
		imageCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
	}
	//VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &imageCreateInfo, nullptr, &texture->image));
	texture->image = context.device.createImage(imageCreateInfo, nullptr);

	//vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, texture->image, &memReqs);
	memReqs = context.device.getImageMemoryRequirements(texture->image);

	memAllocInfo.allocationSize = memReqs.size;

	//memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	memAllocInfo.memoryTypeIndex = getMemoryType(context.deviceMemoryProperties, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

	//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAllocInfo, nullptr, &texture->deviceMemory));
	texture->memory = context.device.allocateMemory(memAllocInfo, nullptr);
	//VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, texture->image, texture->deviceMemory, 0));
	context.device.bindImageMemory(texture->image, texture->memory, 0);

	vk::ImageSubresourceRange subresourceRange;
	subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = texture->mipLevels;
	subresourceRange.layerCount = 1;

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	vkx::setImageLayout(
		cmdBuffer,
		texture->image,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal,
		subresourceRange);

	// Copy mip levels from staging buffer
	//vkCmdCopyBufferToImage(
	//	cmdBuffer,
	//	stagingBuffer,
	//	texture->image,
	//	vk::ImageLayout::eTransferDstOptimal,
	//	1,
	//	&bufferCopyRegion
	//);

	cmdBuffer.copyBufferToImage(stagingBuffer, texture->image, vk::ImageLayout::eTransferDstOptimal, 1, &bufferCopyRegion);

	// Change texture image layout to shader read after all mip levels have been copied
	texture->imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	vkx::setImageLayout(
		cmdBuffer,
		texture->image,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageLayout::eTransferDstOptimal,
		texture->imageLayout,
		subresourceRange);

	// Submit command buffer containing copy and image layout commands
	//VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	cmdBuffer.end();

	// Create a fence to make sure that the copies have finished before continuing
	vk::Fence copyFence;
	vk::FenceCreateInfo fenceCreateInfo;/* = vkTools::initializers::fenceCreateInfo(VK_FLAGS_NONE);*/
	//VK_CHECK_RESULT(vkCreateFence(vulkanDevice->logicalDevice, &fenceCreateInfo, nullptr, &copyFence));
	copyFence = context.device.createFence(fenceCreateInfo, nullptr);

	vk::SubmitInfo submitInfo;/* = vkTools::initializers::submitInfo();*/
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, copyFence));
	queue.submit(1, &submitInfo, copyFence);

	//VK_CHECK_RESULT(vkWaitForFences(vulkanDevice->logicalDevice, 1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
	context.device.waitForFences(1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

	//vkDestroyFence(vulkanDevice->logicalDevice, copyFence, nullptr);
	context.device.destroyFence(copyFence, nullptr);

	// Clean up staging resources
	//vkFreeMemory(vulkanDevice->logicalDevice, stagingMemory, nullptr);
	context.device.freeMemory(stagingMemory, nullptr);
	//vkDestroyBuffer(vulkanDevice->logicalDevice, stagingBuffer, nullptr);
	context.device.destroyBuffer(stagingBuffer, nullptr);

	// Create sampler
	vk::SamplerCreateInfo sampler;
	sampler.magFilter = filter;
	sampler.minFilter = filter;
	sampler.mipmapMode = vk::SamplerMipmapMode::eLinear;
	sampler.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampler.addressModeV = vk::SamplerAddressMode::eRepeat;
	sampler.addressModeW = vk::SamplerAddressMode::eRepeat;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = vk::CompareOp::eNever;
	sampler.minLod = 0.0f;
	sampler.maxLod = 0.0f;
	//VK_CHECK_RESULT(vkCreateSampler(vulkanDevice->logicalDevice, &sampler, nullptr, &texture->sampler));
	texture->sampler = context.device.createSampler(sampler, nullptr);

	// Create image view
	vk::ImageViewCreateInfo view;
	view.pNext = NULL;
	view.image = VK_NULL_HANDLE;
	view.viewType = vk::ImageViewType::e2D;
	view.format = format;
	view.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
	view.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
	view.subresourceRange.levelCount = 1;
	view.image = texture->image;
	//VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &view, nullptr, &texture->view));
	texture->view = context.device.createImageView(view, nullptr);

	// Fill descriptor image info that can be used for setting up descriptor sets
	texture->descriptor.imageLayout = vk::ImageLayout::eGeneral;
	//texture->descriptor.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	texture->descriptor.imageView = texture->view;
	texture->descriptor.sampler = texture->sampler;
}

