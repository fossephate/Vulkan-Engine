#include "vulkanTextureLoader.h"

/**
* Default constructor
*
* @param vulkanDevice Pointer to a valid VulkanDevice
* @param queue Queue for the copy commands when using staging (queue must support transfers)
* @param cmdPool Commandpool used to get command buffers for copies and layout transitions
*/

vkTools::VulkanTextureLoader::VulkanTextureLoader(vkx::VulkanDevice * vulkanDevice, vk::Queue queue, vk::CommandPool cmdPool)
{
	this->vulkanDevice = vulkanDevice;
	this->queue = queue;
	this->cmdPool = cmdPool;

	// Create command buffer for submitting image barriers
	// and converting tilings
	vk::CommandBufferAllocateInfo cmdBufInfo;
	cmdBufInfo.commandPool = cmdPool;
	cmdBufInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufInfo.commandBufferCount = 1;

	//VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice->logicalDevice, &cmdBufInfo, &cmdBuffer));
	vk::Device(vulkanDevice->logicalDevice).allocateCommandBuffers(&cmdBufInfo, &cmdBuffer);
}

/**
* Default destructor
*
* @note Does not free texture resources
*/

vkTools::VulkanTextureLoader::~VulkanTextureLoader()
{
	//vkFreeCommandBuffers(vulkanDevice->logicalDevice, cmdPool, 1, &cmdBuffer);
	vk::Device(vulkanDevice->logicalDevice).freeCommandBuffers(cmdPool, 1, &cmdBuffer);
}

/**
* Load a 2D texture including all mip levels
*
* @param filename File to load
* @param format Vulkan format of the image data stored in the file
* @param texture Pointer to the texture object to load the image into
* @param (Optional) forceLinear Force linear tiling (not advised, defaults to false)
* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
*
* @note Only supports .ktx and .dds
*/

void vkTools::VulkanTextureLoader::loadTexture(std::string filename, vk::Format format, VulkanTexture * texture, bool forceLinear, vk::ImageUsageFlags imageUsageFlags)
{
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

	texture->width = static_cast<uint32_t>(tex2D[0].dimensions().x);
	texture->height = static_cast<uint32_t>(tex2D[0].dimensions().y);
	texture->mipLevels = static_cast<uint32_t>(tex2D.levels());

	// Get device properites for the requested texture format
	vk::FormatProperties formatProperties;
	//vkGetPhysicalDeviceFormatProperties(vulkanDevice->physicalDevice, format, &formatProperties);
	vk::PhysicalDevice(vulkanDevice->physicalDevice).getFormatProperties(format, &formatProperties);

	// Only use linear tiling if requested (and supported by the device)
	// Support for linear tiling is mostly limited, so prefer to use
	// optimal tiling instead
	// On most implementations linear tiling will only support a very
	// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
	vk::Bool32 useStaging = !forceLinear;

	vk::MemoryAllocateInfo memAllocInfo;
	vk::MemoryRequirements memReqs;

	// Use a separate command buffer for texture loading
	vk::CommandBufferBeginInfo cmdBufInfo;
	//VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	cmdBuffer.begin(&cmdBufInfo);

	if (useStaging) {
		// Create a host-visible staging buffer that contains the raw image data
		vk::Buffer stagingBuffer;
		vk::DeviceMemory stagingMemory;

		vk::BufferCreateInfo bufferCreateInfo;
		bufferCreateInfo.size = tex2D.size();
		// This buffer is used as a transfer source for the buffer copy
		bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
		bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

		//VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));
		vk::Device(vulkanDevice->logicalDevice).createBuffer(&bufferCreateInfo, nullptr, &stagingBuffer);

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		//vkGetBufferMemoryRequirements(vulkanDevice->logicalDevice, stagingBuffer, &memReqs);
		vk::Device(vulkanDevice->logicalDevice).getBufferMemoryRequirements(stagingBuffer, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;
		// Get memory type index for a host visible buffer
		memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
		vk::Device(vulkanDevice->logicalDevice).allocateMemory(&memAllocInfo, nullptr, &stagingMemory);

		//VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice->logicalDevice, stagingBuffer, stagingMemory, 0));
		vk::Device(vulkanDevice->logicalDevice).bindBufferMemory(stagingBuffer, stagingMemory, 0);

		// Copy texture data into staging buffer
		uint8_t *data;

		//VK_CHECK_RESULT(vkMapMemory(vulkanDevice->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
		vk::Device(vulkanDevice->logicalDevice).mapMemory(stagingMemory, 0, memReqs.size, vk::MemoryMapFlagBits());//what?

		memcpy(data, tex2D.data(), tex2D.size());
		//vkUnmapMemory(vulkanDevice->logicalDevice, stagingMemory);
		vk::Device(vulkanDevice->logicalDevice).unmapMemory(stagingMemory);

		// Setup buffer copy regions for each mip level
		std::vector<vk::BufferImageCopy> bufferCopyRegions;
		uint32_t offset = 0;

		for (uint32_t i = 0; i < texture->mipLevels; i++) {
			vk::BufferImageCopy bufferCopyRegion;
			bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			bufferCopyRegion.imageSubresource.mipLevel = i;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex2D[i].dimensions().x);
			bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex2D[i].dimensions().y);
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;

			bufferCopyRegions.push_back(bufferCopyRegion);

			offset += static_cast<uint32_t>(tex2D[i].size());
		}

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
		imageCreateInfo.extent = { texture->width, texture->height, 1 };
		imageCreateInfo.usage = imageUsageFlags;
		// Ensure that the TRANSFER_DST bit is set for staging
		if (!(imageCreateInfo.usage & vk::ImageUsageFlagBits::eTransferDst)) {
			imageCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
		}
		//VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &imageCreateInfo, nullptr, &texture->image));
		vk::Device(vulkanDevice->logicalDevice).createImage(&imageCreateInfo, nullptr, &texture->image);

		//vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, texture->image, &memReqs);
		vk::Device(vulkanDevice->logicalDevice).getImageMemoryRequirements(texture->image, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;

		memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
		//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAllocInfo, nullptr, &texture->deviceMemory));
		vk::Device(vulkanDevice->logicalDevice).allocateMemory(&memAllocInfo, nullptr, &texture->deviceMemory);
		//VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, texture->image, texture->deviceMemory, 0));
		vk::Device(vulkanDevice->logicalDevice).bindImageMemory(texture->image, texture->deviceMemory, 0);

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
		//	static_cast<uint32_t>(bufferCopyRegions.size()),
		//	bufferCopyRegions.data()
		//);

		cmdBuffer.copyBufferToImage(
			stagingBuffer,
			texture->image,
			vk::ImageLayout::eTransferDstOptimal,
			static_cast<uint32_t>(bufferCopyRegions.size()),
			bufferCopyRegions.data()
		);

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
		vk::FenceCreateInfo fenceCreateInfo;// = vkx::fenceCreateInfo(VK_FLAGS_NONE);
		//VK_CHECK_RESULT(vkCreateFence(vulkanDevice->logicalDevice, &fenceCreateInfo, nullptr, &copyFence));
		vk::Device(vulkanDevice->logicalDevice).createFence(&fenceCreateInfo, nullptr, &copyFence);

		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, copyFence));
		queue.submit(1, &submitInfo, copyFence);

		//VK_CHECK_RESULT(vkWaitForFences(vulkanDevice->logicalDevice, 1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
		vk::Device(vulkanDevice->logicalDevice).waitForFences(1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

		//vkDestroyFence(vulkanDevice->logicalDevice, copyFence, nullptr);
		vk::Device(vulkanDevice->logicalDevice).destroyFence(copyFence, nullptr);

		// Clean up staging resources
		//vkFreeMemory(vulkanDevice->logicalDevice, stagingMemory, nullptr);
		vk::Device(vulkanDevice->logicalDevice).freeMemory(stagingMemory, nullptr);
		//vkDestroyBuffer(vulkanDevice->logicalDevice, stagingBuffer, nullptr);
		vk::Device(vulkanDevice->logicalDevice).destroyBuffer(stagingBuffer, nullptr);
	} else {
		// Prefer using optimal tiling, as linear tiling 
		// may support only a small set of features 
		// depending on implementation (e.g. no mip maps, only one layer, etc.)

		// Check if this support is supported for linear tiling
		assert(formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);

		vk::Image mappableImage;
		vk::DeviceMemory mappableMemory;

		vk::ImageCreateInfo imageCreateInfo;
		imageCreateInfo.imageType = vk::ImageType::e2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { texture->width, texture->height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
		imageCreateInfo.tiling = vk::ImageTiling::eLinear;
		imageCreateInfo.usage = imageUsageFlags;
		imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		imageCreateInfo.initialLayout = vk::ImageLayout::ePreinitialized;

		// Load mip map level 0 to linear tiling image
		//VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &imageCreateInfo, nullptr, &mappableImage));
		vk::Device(vulkanDevice->logicalDevice).createImage(&imageCreateInfo, nullptr, &mappableImage);

		// Get memory requirements for this image 
		// like size and alignment
		//vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, mappableImage, &memReqs);
		vk::Device(vulkanDevice->logicalDevice).getImageMemoryRequirements(mappableImage, &memReqs);

		// Set memory allocation size to required memory size
		memAllocInfo.allocationSize = memReqs.size;

		// Get memory type that can be mapped to host memory
		memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		// Allocate host memory
		//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAllocInfo, nullptr, &mappableMemory));
		vk::Device(vulkanDevice->logicalDevice).allocateMemory(&memAllocInfo, nullptr, &mappableMemory);

		// Bind allocated image for use
		//VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, mappableImage, mappableMemory, 0));
		vk::Device(vulkanDevice->logicalDevice).bindImageMemory(mappableImage, mappableMemory, 0);

		// Get sub resource layout
		// Mip map count, array layer, etc.
		vk::ImageSubresource subRes;
		subRes.aspectMask = vk::ImageAspectFlagBits::eColor;
		subRes.mipLevel = 0;

		vk::SubresourceLayout subResLayout;
		void *data;

		// Get sub resources layout 
		// Includes row pitch, size offsets, etc.
		//vkGetImageSubresourceLayout(vulkanDevice->logicalDevice, mappableImage, &subRes, &subResLayout);
		vulkanDevice->logicalDevice.getImageSubresourceLayout(mappableImage, &subRes, &subResLayout);

		// Map image memory
		//VK_CHECK_RESULT(vkMapMemory(vulkanDevice->logicalDevice, mappableMemory, 0, memReqs.size, 0, &data));
		//vulkanDevice->logicalDevice.mapMemory(mappableMemory, 0, memReqs.size, vk::MemoryMapFlags(), &data);
		data = vulkanDevice->logicalDevice.mapMemory(mappableMemory, 0, memReqs.size, vk::MemoryMapFlags());

		// Copy image data into memory
		memcpy(data, tex2D[subRes.mipLevel].data(), tex2D[subRes.mipLevel].size());

		//vkUnmapMemory(vulkanDevice->logicalDevice, mappableMemory);
		vk::Device(vulkanDevice->logicalDevice).unmapMemory(mappableMemory);

		// Linear tiled images don't need to be staged
		// and can be directly used as textures
		texture->image = mappableImage;
		texture->deviceMemory = mappableMemory;
		texture->imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		// Setup image memory barrier
		vkx::setImageLayout(
			cmdBuffer,
			texture->image,
			vk::ImageAspectFlagBits::eColor,
			vk::ImageLayout::ePreinitialized,
			texture->imageLayout);

		// Submit command buffer containing copy and image layout commands
		//VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
		cmdBuffer.end();

		vk::Fence nullFence = { VK_NULL_HANDLE };

		vk::SubmitInfo submitInfo;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, nullFence));
		queue.submit(1, &submitInfo, nullFence);
		//VK_CHECK_RESULT(vkQueueWaitIdle(queue));
		queue.waitIdle();
	}

	// Create sampler
	vk::SamplerCreateInfo sampler;
	sampler.magFilter = vk::Filter::eLinear;
	sampler.minFilter = vk::Filter::eLinear;
	sampler.mipmapMode = vk::SamplerMipmapMode::eLinear;
	sampler.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampler.addressModeV = vk::SamplerAddressMode::eRepeat;
	sampler.addressModeW = vk::SamplerAddressMode::eRepeat;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = vk::CompareOp::eNever;
	sampler.minLod = 0.0f;
	// Max level-of-detail should match mip level count
	sampler.maxLod = (useStaging) ? (float)texture->mipLevels : 0.0f;
	// Enable anisotropic filtering
	sampler.maxAnisotropy = 8;
	sampler.anisotropyEnable = VK_TRUE;
	sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	//VK_CHECK_RESULT(vkCreateSampler(vulkanDevice->logicalDevice, &sampler, nullptr, &texture->sampler));
	vk::Device(vulkanDevice->logicalDevice).createSampler(&sampler, nullptr, &texture->sampler);

	// Create image view
	// Textures are not directly accessed by the shaders and
	// are abstracted by image views containing additional
	// information and sub resource ranges
	vk::ImageViewCreateInfo view;
	view.pNext = NULL;
	view.image = VK_NULL_HANDLE;
	view.viewType = vk::ImageViewType::e2D;
	view.format = format;
	view.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
	view.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
	// Linear tiling usually won't support mip maps
	// Only set mip map count if optimal tiling is used
	view.subresourceRange.levelCount = (useStaging) ? texture->mipLevels : 1;
	view.image = texture->image;
	//VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &view, nullptr, &texture->view));
	vk::Device(vulkanDevice->logicalDevice).createImageView(&view, nullptr, &texture->view);

	// Fill descriptor image info that can be used for setting up descriptor sets
	texture->descriptor.imageLayout = vk::ImageLayout::eGeneral;
	texture->descriptor.imageView = texture->view;
	texture->descriptor.sampler = texture->sampler;
}

/**
* Load a cubemap texture including all mip levels from a single file
*
* @param filename File to load
* @param format Vulkan format of the image data stored in the file
* @param texture Pointer to the texture object to load the image into
*
* @note Only supports .ktx and .dds
*/

void vkTools::VulkanTextureLoader::loadCubemap(std::string filename, vk::Format format, VulkanTexture * texture, vk::ImageUsageFlags imageUsageFlags)
{
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
	assert(!texCube.empty());

	texture->width = static_cast<uint32_t>(texCube.dimensions().x);
	texture->height = static_cast<uint32_t>(texCube.dimensions().y);
	texture->mipLevels = static_cast<uint32_t>(texCube.levels());

	vk::MemoryAllocateInfo memAllocInfo;
	vk::MemoryRequirements memReqs;

	// Create a host-visible staging buffer that contains the raw image data
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingMemory;

	vk::BufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.size = texCube.size();
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	//VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));
	vk::Device(vulkanDevice->logicalDevice).createBuffer(&bufferCreateInfo, nullptr, &stagingBuffer);

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	//vkGetBufferMemoryRequirements(vulkanDevice->logicalDevice, stagingBuffer, &memReqs);
	vk::Device(vulkanDevice->logicalDevice).getBufferMemoryRequirements(stagingBuffer, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	// Get memory type index for a host visible buffer
	memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
	vk::Device(vulkanDevice->logicalDevice).allocateMemory(&memAllocInfo, nullptr, &stagingMemory);
	//VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice->logicalDevice, stagingBuffer, stagingMemory, 0));
	vk::Device(vulkanDevice->logicalDevice).bindBufferMemory(stagingBuffer, stagingMemory, 0);

	// Copy texture data into staging buffer
	uint8_t *data;
	//VK_CHECK_RESULT(vkMapMemory(vulkanDevice->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
	//vulkanDevice->logicalDevice.mapMemory(stagingMemory, 0, memReqs.size, vk::MemoryMapFlags());//what?
	(uint8_t *)data = (uint8_t *)vulkanDevice->logicalDevice.mapMemory(stagingMemory, 0, memReqs.size, vk::MemoryMapFlags());

	memcpy(data, texCube.data(), texCube.size());
	//vkUnmapMemory(vulkanDevice->logicalDevice, stagingMemory);
	vk::Device(vulkanDevice->logicalDevice).unmapMemory(stagingMemory);

	// Setup buffer copy regions for each face including all of it's miplevels
	std::vector<vk::BufferImageCopy> bufferCopyRegions;
	size_t offset = 0;

	for (uint32_t face = 0; face < 6; face++) {
		for (uint32_t level = 0; level < texture->mipLevels; level++) {
			vk::BufferImageCopy bufferCopyRegion;
			bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(texCube[face][level].dimensions().x);
			bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(texCube[face][level].dimensions().y);
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;

			bufferCopyRegions.push_back(bufferCopyRegion);

			// Increase offset into staging buffer for next level / face
			offset += texCube[face][level].size();
		}
	}

	// Create optimal tiled target image
	vk::ImageCreateInfo imageCreateInfo;
	imageCreateInfo.imageType = vk::ImageType::e2D;
	imageCreateInfo.format = format;
	imageCreateInfo.mipLevels = texture->mipLevels;
	imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
	imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
	imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageCreateInfo.extent = { texture->width, texture->height, 1 };
	imageCreateInfo.usage = imageUsageFlags;
	// Ensure that the TRANSFER_DST bit is set for staging
	if (!(imageCreateInfo.usage & vk::ImageUsageFlagBits::eTransferDst)) {
		imageCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
	}
	// Cube faces count as array layers in Vulkan
	imageCreateInfo.arrayLayers = 6;
	// This flag is required for cube map images
	imageCreateInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;


	//VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &imageCreateInfo, nullptr, &texture->image));
	vk::Device(vulkanDevice->logicalDevice).createImage(&imageCreateInfo, nullptr, &texture->image);

	//vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, texture->image, &memReqs);
	vk::Device(vulkanDevice->logicalDevice).getImageMemoryRequirements(texture->image, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

	//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAllocInfo, nullptr, &texture->deviceMemory));
	vk::Device(vulkanDevice->logicalDevice).allocateMemory(&memAllocInfo, nullptr, &texture->deviceMemory);
	//VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, texture->image, texture->deviceMemory, 0));
	vk::Device(vulkanDevice->logicalDevice).bindImageMemory(texture->image, texture->deviceMemory, 0);

	vk::CommandBufferBeginInfo cmdBufInfo;
	//VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	cmdBuffer.begin(&cmdBufInfo);

	// Image barrier for optimal image (target)
	// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
	vk::ImageSubresourceRange subresourceRange;
	subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = texture->mipLevels;
	subresourceRange.layerCount = 6;

	vkx::setImageLayout(
		cmdBuffer,
		texture->image,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal,
		subresourceRange);

	// Copy the cube map faces from the staging buffer to the optimal tiled image
	//vkCmdCopyBufferToImage(
	//	cmdBuffer,
	//	stagingBuffer,
	//	texture->image,
	//	vk::ImageLayout::eTransferDstOptimal,
	//	static_cast<uint32_t>(bufferCopyRegions.size()),
	//	bufferCopyRegions.data());

	cmdBuffer.copyBufferToImage(
		stagingBuffer,
		texture->image,
		vk::ImageLayout::eTransferDstOptimal,
		static_cast<uint32_t>(bufferCopyRegions.size()),
		bufferCopyRegions.data());

	// Change texture image layout to shader read after all faces have been copied
	texture->imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	vkx::setImageLayout(
		cmdBuffer,
		texture->image,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageLayout::eTransferDstOptimal,
		texture->imageLayout,
		subresourceRange);

	//VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	cmdBuffer.end();

	// Create a fence to make sure that the copies have finished before continuing
	vk::Fence copyFence;
	vk::FenceCreateInfo fenceCreateInfo;// = vkx::fenceCreateInfo(VK_FLAGS_NONE);
	//VK_CHECK_RESULT(vkCreateFence(vulkanDevice->logicalDevice, &fenceCreateInfo, nullptr, &copyFence));
	vk::Device(vulkanDevice->logicalDevice).createFence(&fenceCreateInfo, nullptr, &copyFence);

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, copyFence));
	queue.submit(1, &submitInfo, copyFence);

	//VK_CHECK_RESULT(vkWaitForFences(vulkanDevice->logicalDevice, 1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
	vk::Device(vulkanDevice->logicalDevice).waitForFences(1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

	//vkDestroyFence(vulkanDevice->logicalDevice, copyFence, nullptr);
	vk::Device(vulkanDevice->logicalDevice).destroyFence(copyFence, nullptr);

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
	sampler.maxLod = (float)texture->mipLevels;
	sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	//VK_CHECK_RESULT(vkCreateSampler(vulkanDevice->logicalDevice, &sampler, nullptr, &texture->sampler));
	vk::Device(vulkanDevice->logicalDevice).createSampler(&sampler, nullptr, &texture->sampler);

	// Create image view
	vk::ImageViewCreateInfo view;
	view.viewType = vk::ImageViewType::eCube;
	view.format = format;
	view.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
	view.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
	view.subresourceRange.layerCount = 6;
	view.subresourceRange.levelCount = texture->mipLevels;
	view.image = texture->image;
	//VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &view, nullptr, &texture->view));
	vk::Device(vulkanDevice->logicalDevice).createImageView(&view, nullptr, &texture->view);

	// Clean up staging resources
	vkFreeMemory(vulkanDevice->logicalDevice, stagingMemory, nullptr);
	vkDestroyBuffer(vulkanDevice->logicalDevice, stagingBuffer, nullptr);

	// Fill descriptor image info that can be used for setting up descriptor sets
	texture->descriptor.imageLayout = vk::ImageLayout::eGeneral;
	texture->descriptor.imageView = texture->view;
	texture->descriptor.sampler = texture->sampler;
}

/**
* Load a texture array including all mip levels from a single file
*
* @param filename File to load
* @param format Vulkan format of the image data stored in the file
* @param texture Pointer to the texture object to load the image into
*
* @note Only supports .ktx and .dds
*/

void vkTools::VulkanTextureLoader::loadTextureArray(std::string filename, vk::Format format, VulkanTexture * texture, vk::ImageUsageFlags imageUsageFlags)
{
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

	assert(!tex2DArray.empty());

	texture->width = static_cast<uint32_t>(tex2DArray.dimensions().x);
	texture->height = static_cast<uint32_t>(tex2DArray.dimensions().y);
	texture->layerCount = static_cast<uint32_t>(tex2DArray.layers());
	texture->mipLevels = static_cast<uint32_t>(tex2DArray.levels());

	vk::MemoryAllocateInfo memAllocInfo;
	vk::MemoryRequirements memReqs;

	// Create a host-visible staging buffer that contains the raw image data
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingMemory;

	vk::BufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.size = tex2DArray.size();
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	//VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));
	vk::Device(vulkanDevice->logicalDevice).createBuffer(&bufferCreateInfo, nullptr, &stagingBuffer);

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	//vkGetBufferMemoryRequirements(vulkanDevice->logicalDevice, stagingBuffer, &memReqs);
	vk::Device(vulkanDevice->logicalDevice).getBufferMemoryRequirements(stagingBuffer, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	// Get memory type index for a host visible buffer
	memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
	vk::Device(vulkanDevice->logicalDevice).allocateMemory(&memAllocInfo, nullptr, &stagingMemory);
	//VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice->logicalDevice, stagingBuffer, stagingMemory, 0));
	vk::Device(vulkanDevice->logicalDevice).bindBufferMemory(stagingBuffer, stagingMemory, 0);

	// Copy texture data into staging buffer
	uint8_t *data;
	//VK_CHECK_RESULT(vkMapMemory(vulkanDevice->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
	vk::Device(vulkanDevice->logicalDevice).mapMemory(stagingMemory, 0, memReqs.size, vk::MemoryMapFlags());//what?
	memcpy(data, tex2DArray.data(), static_cast<size_t>(tex2DArray.size()));
	//vkUnmapMemory(vulkanDevice->logicalDevice, stagingMemory);
	vk::Device(vulkanDevice->logicalDevice).unmapMemory(stagingMemory);

	// Setup buffer copy regions for each layer including all of it's miplevels
	std::vector<vk::BufferImageCopy> bufferCopyRegions;
	size_t offset = 0;

	for (uint32_t layer = 0; layer < texture->layerCount; layer++) {
		for (uint32_t level = 0; level < texture->mipLevels; level++) {
			vk::BufferImageCopy bufferCopyRegion;
			bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex2DArray[layer][level].dimensions().x);
			bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex2DArray[layer][level].dimensions().y);
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;

			bufferCopyRegions.push_back(bufferCopyRegion);

			// Increase offset into staging buffer for next level / face
			offset += tex2DArray[layer][level].size();
		}
	}

	// Create optimal tiled target image
	vk::ImageCreateInfo imageCreateInfo;
	imageCreateInfo.imageType = vk::ImageType::e2D;
	imageCreateInfo.format = format;
	imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
	imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
	imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageCreateInfo.extent = { texture->width, texture->height, 1 };
	imageCreateInfo.usage = imageUsageFlags;
	// Ensure that the TRANSFER_DST bit is set for staging
	if (!(imageCreateInfo.usage & vk::ImageUsageFlagBits::eTransferDst)) {
		imageCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
	}
	imageCreateInfo.arrayLayers = texture->layerCount;
	imageCreateInfo.mipLevels = texture->mipLevels;

	//VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &imageCreateInfo, nullptr, &texture->image));
	vk::Device(vulkanDevice->logicalDevice).createImage(&imageCreateInfo, nullptr, &texture->image);

	//vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, texture->image, &memReqs);
	vk::Device(vulkanDevice->logicalDevice).getImageMemoryRequirements(texture->image, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

	//VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAllocInfo, nullptr, &texture->deviceMemory));
	vk::Device(vulkanDevice->logicalDevice).allocateMemory(&memAllocInfo, nullptr, &texture->deviceMemory);
	//VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, texture->image, texture->deviceMemory, 0));
	vk::Device(vulkanDevice->logicalDevice).bindImageMemory(texture->image, texture->deviceMemory, 0);

	vk::CommandBufferBeginInfo cmdBufInfo;
	//VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	cmdBuffer.begin(&cmdBufInfo);

	// Image barrier for optimal image (target)
	// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
	vk::ImageSubresourceRange subresourceRange;
	subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = texture->mipLevels;
	subresourceRange.layerCount = texture->layerCount;

	vkx::setImageLayout(
		cmdBuffer,
		texture->image,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal,
		subresourceRange);

	// Copy the layers and mip levels from the staging buffer to the optimal tiled image
	//vkCmdCopyBufferToImage(
	//	cmdBuffer,
	//	stagingBuffer,
	//	texture->image,
	//	vk::ImageLayout::eTransferDstOptimal,
	//	static_cast<uint32_t>(bufferCopyRegions.size()),
	//	bufferCopyRegions.data());
	cmdBuffer.copyBufferToImage(
		stagingBuffer,
		texture->image,
		vk::ImageLayout::eTransferDstOptimal,
		static_cast<uint32_t>(bufferCopyRegions.size()),
		bufferCopyRegions.data());

	// Change texture image layout to shader read after all faces have been copied
	texture->imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	vkx::setImageLayout(
		cmdBuffer,
		texture->image,
		vk::ImageAspectFlagBits::eColor,
		vk::ImageLayout::eTransferDstOptimal,
		texture->imageLayout,
		subresourceRange);

	//VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	cmdBuffer.end();

	// Create a fence to make sure that the copies have finished before continuing
	vk::Fence copyFence;
	vk::FenceCreateInfo fenceCreateInfo;//= vkx::fenceCreateInfo(VK_FLAGS_NONE);
	//VK_CHECK_RESULT(vkCreateFence(vulkanDevice->logicalDevice, &fenceCreateInfo, nullptr, &copyFence));
	vulkanDevice->logicalDevice.createFence(&fenceCreateInfo, nullptr, &copyFence);

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, copyFence));
	queue.submit(1, &submitInfo, copyFence);

	//VK_CHECK_RESULT(vkWaitForFences(vulkanDevice->logicalDevice, 1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
	vulkanDevice->logicalDevice.waitForFences(1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

	//vkDestroyFence(vulkanDevice->logicalDevice, copyFence, nullptr);
	vulkanDevice->logicalDevice.destroyFence(copyFence, nullptr);

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
	sampler.maxLod = (float)texture->mipLevels;
	sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	//VK_CHECK_RESULT(vkCreateSampler(vulkanDevice->logicalDevice, &sampler, nullptr, &texture->sampler));
	vk::Device(vulkanDevice->logicalDevice).createSampler(&sampler, nullptr, &texture->sampler);

	// Create image view
	vk::ImageViewCreateInfo view;
	view.viewType = vk::ImageViewType::e2DArray;
	view.format = format;
	view.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
	view.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
	view.subresourceRange.layerCount = texture->layerCount;
	view.subresourceRange.levelCount = texture->mipLevels;
	view.image = texture->image;
	//VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &view, nullptr, &texture->view));
	vk::Device(vulkanDevice->logicalDevice).createImageView(&view, nullptr, &texture->view);

	// Clean up staging resources
	//vkFreeMemory(vulkanDevice->logicalDevice, stagingMemory, nullptr);
	vk::Device(vulkanDevice->logicalDevice).freeMemory(stagingMemory, nullptr);
	//vkDestroyBuffer(vulkanDevice->logicalDevice, stagingBuffer, nullptr);
	vk::Device(vulkanDevice->logicalDevice).destroyBuffer(stagingBuffer, nullptr);

	// Fill descriptor image info that can be used for setting up descriptor sets
	texture->descriptor.imageLayout = vk::ImageLayout::eGeneral;
	texture->descriptor.imageView = texture->view;
	texture->descriptor.sampler = texture->sampler;
}

/**
* Free all Vulkan resources used by a texture object
*
* @param texture Texture object whose resources are to be freed
*/

void vkTools::VulkanTextureLoader::destroyTexture(VulkanTexture texture)
{
	vkDestroyImageView(vulkanDevice->logicalDevice, texture.view, nullptr);
	vkDestroyImage(vulkanDevice->logicalDevice, texture.image, nullptr);
	vkDestroySampler(vulkanDevice->logicalDevice, texture.sampler, nullptr);
	vkFreeMemory(vulkanDevice->logicalDevice, texture.deviceMemory, nullptr);
}
