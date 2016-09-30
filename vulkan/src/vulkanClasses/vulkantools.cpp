/*
* Assorted commonly used Vulkan helper functions
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkantools.h"

namespace vkTools
{


//	VkBool32 checkGlobalExtensionPresent(const char* extensionName)
//	{
//		uint32_t extensionCount = 0;
//		std::vector<VkExtensionProperties> extensions;
//		vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
//		extensions.resize(extensionCount);
//		vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions.data());
//		for (auto& ext : extensions)
//		{
//			if (!strcmp(extensionName, ext.extensionName))
//			{
//				return true;
//			}
//		}
//		return false;
//	}
//
//	VkBool32 checkDeviceExtensionPresent(VkPhysicalDevice physicalDevice, const char* extensionName)
//	{
//		uint32_t extensionCount = 0;
//		std::vector<VkExtensionProperties> extensions;
//		vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);
//		extensions.resize(extensionCount);
//		vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, extensions.data());
//		for (auto& ext : extensions)
//		{
//			if (!strcmp(extensionName, ext.extensionName))
//			{
//				return true;
//			}
//		}
//		return false;
//	}
//
//	std::string errorString(VkResult errorCode)
//	{
//		switch (errorCode)
//		{
//#define STR(r) case VK_ ##r: return #r
//			STR(NOT_READY);
//			STR(TIMEOUT);
//			STR(EVENT_SET);
//			STR(EVENT_RESET);
//			STR(INCOMPLETE);
//			STR(ERROR_OUT_OF_HOST_MEMORY);
//			STR(ERROR_OUT_OF_DEVICE_MEMORY);
//			STR(ERROR_INITIALIZATION_FAILED);
//			STR(ERROR_DEVICE_LOST);
//			STR(ERROR_MEMORY_MAP_FAILED);
//			STR(ERROR_LAYER_NOT_PRESENT);
//			STR(ERROR_EXTENSION_NOT_PRESENT);
//			STR(ERROR_FEATURE_NOT_PRESENT);
//			STR(ERROR_INCOMPATIBLE_DRIVER);
//			STR(ERROR_TOO_MANY_OBJECTS);
//			STR(ERROR_FORMAT_NOT_SUPPORTED);
//			STR(ERROR_SURFACE_LOST_KHR);
//			STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
//			STR(SUBOPTIMAL_KHR);
//			STR(ERROR_OUT_OF_DATE_KHR);
//			STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
//			STR(ERROR_VALIDATION_FAILED_EXT);
//			STR(ERROR_INVALID_SHADER_NV);
//#undef STR
//		default:
//			return "UNKNOWN_ERROR";
//		}
//	}
//
//	VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat)
//	{
//		// Since all depth formats may be optional, we need to find a suitable depth format to use
//		// Start with the highest precision packed format
//		std::vector<VkFormat> depthFormats = { 
//			VK_FORMAT_D32_SFLOAT_S8_UINT, 
//			VK_FORMAT_D32_SFLOAT,
//			VK_FORMAT_D24_UNORM_S8_UINT, 
//			VK_FORMAT_D16_UNORM_S8_UINT, 
//			VK_FORMAT_D16_UNORM 
//		};
//
//		for (auto& format : depthFormats)
//		{
//			VkFormatProperties formatProps;
//			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
//			// Format must support depth stencil attachment for optimal tiling
//			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
//			{
//				*depthFormat = format;
//				return true;
//			}
//		}
//
//		return false;
//	}
//
//	// Create an image memory barrier for changing the layout of
//	// an image and put it into an active command buffer
//	// See chapter 11.4 "Image Layout" for details
//
//	void setImageLayout(
//		VkCommandBuffer cmdbuffer, 
//		VkImage image, 
//		VkImageAspectFlags aspectMask, 
//		VkImageLayout oldImageLayout, 
//		VkImageLayout newImageLayout,
//		VkImageSubresourceRange subresourceRange)
//	{
//		// Create an image barrier object
//		VkImageMemoryBarrier imageMemoryBarrier = vkTools::initializers::imageMemoryBarrier();
//		imageMemoryBarrier.oldLayout = oldImageLayout;
//		imageMemoryBarrier.newLayout = newImageLayout;
//		imageMemoryBarrier.image = image;
//		imageMemoryBarrier.subresourceRange = subresourceRange;
//
//		// Source layouts (old)
//		// Source access mask controls actions that have to be finished on the old layout
//		// before it will be transitioned to the new layout
//		switch (oldImageLayout)
//		{
//		case VK_IMAGE_LAYOUT_UNDEFINED:
//				// Image layout is undefined (or does not matter)
//				// Only valid as initial layout
//				// No flags required, listed only for completeness
//				imageMemoryBarrier.srcAccessMask = 0;
//				break;
//
//		case VK_IMAGE_LAYOUT_PREINITIALIZED:
//				// Image is preinitialized
//				// Only valid as initial layout for linear images, preserves memory contents
//				// Make sure host writes have been finished
//				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
//				break;
//
//		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
//				// Image is a color attachment
//				// Make sure any writes to the color buffer have been finished
//				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//				break;
//
//		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
//				// Image is a depth/stencil attachment
//				// Make sure any writes to the depth/stencil buffer have been finished
//				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//				break;
//
//		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
//				// Image is a transfer source 
//				// Make sure any reads from the image have been finished
//				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//				break;
//
//		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
//				// Image is a transfer destination
//				// Make sure any writes to the image have been finished
//				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//				break;
//
//		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
//				// Image is read by a shader
//				// Make sure any shader reads from the image have been finished
//				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
//				break;
//		}
//
//		// Target layouts (new)
//		// Destination access mask controls the dependency for the new image layout
//		switch (newImageLayout)
//		{
//		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
//			// Image will be used as a transfer destination
//			// Make sure any writes to the image have been finished
//			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//			break;
//
//		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
//			// Image will be used as a transfer source
//			// Make sure any reads from and writes to the image have been finished
//			imageMemoryBarrier.srcAccessMask = imageMemoryBarrier.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
//			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//			break;
//
//		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
//			// Image will be used as a color attachment
//			// Make sure any writes to the color buffer have been finished
//			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//			break;
//
//		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
//			// Image layout will be used as a depth/stencil attachment
//			// Make sure any writes to depth/stencil buffer have been finished
//			imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//			break;
//
//		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
//			// Image will be read in a shader (sampler, input attachment)
//			// Make sure any writes to the image have been finished
//			if (imageMemoryBarrier.srcAccessMask == 0)
//			{
//				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
//			}
//			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//			break;
//		}
//
//		// Put barrier on top
//		VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//		VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//
//		// Put barrier inside setup command buffer
//		vkCmdPipelineBarrier(
//			cmdbuffer, 
//			srcStageFlags, 
//			destStageFlags, 
//			0, 
//			0, nullptr,
//			0, nullptr,
//			1, &imageMemoryBarrier);
//	}
//
//	// Fixed sub resource on first mip level and layer
//	void setImageLayout(
//		VkCommandBuffer cmdbuffer,
//		VkImage image,
//		VkImageAspectFlags aspectMask,
//		VkImageLayout oldImageLayout,
//		VkImageLayout newImageLayout)
//	{
//		VkImageSubresourceRange subresourceRange = {};
//		subresourceRange.aspectMask = aspectMask;
//		subresourceRange.baseMipLevel = 0;
//		subresourceRange.levelCount = 1;
//		subresourceRange.layerCount = 1;
//		setImageLayout(cmdbuffer, image, aspectMask, oldImageLayout, newImageLayout, subresourceRange);
//	}

	vk::Bool32 checkGlobalExtensionPresent2(const char * extensionName)
	{
		return vk::Bool32();
	}

	vk::Bool32 checkDeviceExtensionPresent2(vk::PhysicalDevice physicalDevice, const char * extensionName)
	{
		return vk::Bool32();
	}

	std::string errorString2(vk::Result errorCode)
	{
		return std::string();
	}

	vk::Bool32 getSupportedDepthFormat(vk::PhysicalDevice physicalDevice, vk::Format * depthFormat)
	{
		return vk::Bool32();
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

	void setImageLayout2(vk::CommandBuffer cmdbuffer, vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout, vk::ImageSubresourceRange subresourceRange)
	{

		// Create an image barrier object
		vk::ImageMemoryBarrier imageMemoryBarrier = vkTools::initializers::imageMemoryBarrier2();
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		imageMemoryBarrier.srcAccessMask = accessFlagsForLayout(oldImageLayout);
		imageMemoryBarrier.dstAccessMask = accessFlagsForLayout(newImageLayout);

		// Put barrier on top
		//vk::PipelineStageFlags srcStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		//vk::PipelineStageFlags destStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;


		// Put barrier on top
		// Put barrier inside setup command buffer
		cmdbuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,//srcStageFlags,
			vk::PipelineStageFlagBits::eTopOfPipe,//destStageFlags,
			vk::DependencyFlags(),
			nullptr, nullptr,
			imageMemoryBarrier);
	}

	void setImageLayout2(
		vk::CommandBuffer cmdbuffer,
		vk::Image image,
		vk::ImageAspectFlags aspectMask,
		vk::ImageLayout oldImageLayout,
		vk::ImageLayout newImageLayout)
	{
		vk::ImageSubresourceRange subresourceRange;
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;
		setImageLayout2(cmdbuffer, image, aspectMask, oldImageLayout, newImageLayout, subresourceRange);
	}

	void exitFatal(std::string message, std::string caption)
	{
		#ifdef _WIN32
			MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
		#else
			// TODO : Linux
		#endif

		std::cerr << message << "\n";
		exit(1);
	}



	std::string readTextFile(const char *fileName)
	{
		std::string fileContent;
		std::ifstream fileStream(fileName, std::ios::in);
		if (!fileStream.is_open()) {
			printf("File %s not found\n", fileName);
			return "";
		}
		std::string line = "";
		while (!fileStream.eof()) {
			getline(fileStream, line);
			fileContent.append(line + "\n");
		}
		fileStream.close();
		return fileContent;
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






#if defined(__ANDROID__)
	// Android shaders are stored as assets in the apk
	// So they need to be loaded via the asset manager
	VkShaderModule loadShader(AAssetManager* assetManager, const char *fileName, VkDevice device, VkShaderStageFlagBits stage)
	{
		// Load shader from compressed asset
		AAsset* asset = AAssetManager_open(assetManager, fileName, AASSET_MODE_STREAMING);
		assert(asset);
		size_t size = AAsset_getLength(asset);
		assert(size > 0);

		char *shaderCode = new char[size];
		AAsset_read(asset, shaderCode, size);
		AAsset_close(asset);

		VkShaderModule shaderModule;
		VkShaderModuleCreateInfo moduleCreateInfo;
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;
		moduleCreateInfo.flags = 0;

		VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

		delete[] shaderCode;

		return shaderModule;
	}

	vk::ShaderModule loadShader2(AAssetManager * assetManager, const char * fileName, vk::Device device, vk::ShaderStageFlagBits stage)
	{
		return vk::ShaderModule();
	}

#else
	//VkShaderModule loadShader(const char *fileName, VkDevice device, VkShaderStageFlagBits stage) 
	//{
	//	size_t size;

	//	FILE *fp = fopen(fileName, "rb");
	//	assert(fp);

	//	fseek(fp, 0L, SEEK_END);
	//	size = ftell(fp);

	//	fseek(fp, 0L, SEEK_SET);

	//	//shaderCode = malloc(size);
	//	char *shaderCode = new char[size];
	//	size_t retval = fread(shaderCode, size, 1, fp);
	//	assert(retval == 1);
	//	assert(size > 0);

	//	fclose(fp);

	//	VkShaderModule shaderModule;
	//	VkShaderModuleCreateInfo moduleCreateInfo;
	//	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	//	moduleCreateInfo.pNext = NULL;
	//	moduleCreateInfo.codeSize = size;
	//	moduleCreateInfo.pCode = (uint32_t*)shaderCode;
	//	moduleCreateInfo.flags = 0;

	//	VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

	//	delete[] shaderCode;

	//	return shaderModule;
	//}
	vk::ShaderModule loadShader2(const char * fileName, vk::Device device, vk::ShaderStageFlagBits stage)
	{
		size_t size;

		FILE *fp = fopen(fileName, "rb");
		assert(fp);

		fseek(fp, 0L, SEEK_END);
		size = ftell(fp);

		fseek(fp, 0L, SEEK_SET);

		//shaderCode = malloc(size);
		char *shaderCode = new char[size];
		size_t retval = fread(shaderCode, size, 1, fp);
		assert(retval == 1);
		assert(size > 0);

		fclose(fp);

		vk::ShaderModule shaderModule;
		vk::ShaderModuleCreateInfo moduleCreateInfo;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;
		//moduleCreateInfo.flags = 0;

		//device.createShaderModule(&moduleCreateInfo, NULL, &shaderModule);

		VK_CHECK_RESULT2(device.createShaderModule(&moduleCreateInfo, NULL, &shaderModule));

		delete[] shaderCode;

		return shaderModule;
	}
#endif

	//VkShaderModule loadShaderGLSL(const char *fileName, VkDevice device, VkShaderStageFlagBits stage)
	//{
	//	std::string shaderSrc = readTextFile(fileName);
	//	const char *shaderCode = shaderSrc.c_str();
	//	size_t size = strlen(shaderCode);
	//	assert(size > 0);

	//	VkShaderModule shaderModule;
	//	VkShaderModuleCreateInfo moduleCreateInfo;
	//	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	//	moduleCreateInfo.pNext = NULL;
	//	moduleCreateInfo.codeSize = 3 * sizeof(uint32_t) + size + 1;
	//	moduleCreateInfo.pCode = (uint32_t*)malloc(moduleCreateInfo.codeSize);
	//	moduleCreateInfo.flags = 0;

	//	// Magic SPV number
	//	((uint32_t *)moduleCreateInfo.pCode)[0] = 0x07230203; 
	//	((uint32_t *)moduleCreateInfo.pCode)[1] = 0;
	//	((uint32_t *)moduleCreateInfo.pCode)[2] = stage;
	//	memcpy(((uint32_t *)moduleCreateInfo.pCode + 3), shaderCode, size + 1);

	//	VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

	//	return shaderModule;
	//}

	vk::ShaderModule loadShaderGLSL2(const char * fileName, vk::Device device, vk::ShaderStageFlagBits stage)
	{
		/*std::string shaderSrc = readTextFile(fileName);
		const char *shaderCode = shaderSrc.c_str();
		size_t size = strlen(shaderCode);
		assert(size > 0);

		vk::ShaderModule shaderModule;
		vk::ShaderModuleCreateInfo moduleCreateInfo;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.codeSize = 3 * sizeof(uint32_t) + size + 1;
		moduleCreateInfo.pCode = (uint32_t*)malloc(moduleCreateInfo.codeSize);

		// Magic SPV number
		((uint32_t *)moduleCreateInfo.pCode)[0] = 0x07230203;
		((uint32_t *)moduleCreateInfo.pCode)[1] = 0;
		((uint32_t *)moduleCreateInfo.pCode)[2] = stage;
		memcpy(((uint32_t *)moduleCreateInfo.pCode + 3), shaderCode, size + 1);
		VK_CHECK_RESULT2(device.createShaderModule(&moduleCreateInfo, NULL, &shaderModule));
		return shaderModule;*/

		std::string shaderSrc = readTextFile(fileName);

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

	//VkImageMemoryBarrier prePresentBarrier(VkImage presentImage)
	//{
	//	VkImageMemoryBarrier imageMemoryBarrier = vkTools::initializers::imageMemoryBarrier();
	//	imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	//	imageMemoryBarrier.dstAccessMask = 0;
	//	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	//	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//	imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	//	imageMemoryBarrier.image = presentImage;
	//	return imageMemoryBarrier;
	//}

	vk::ImageMemoryBarrier prePresentBarrier2(vk::Image presentImage)
	{
		vk::ImageMemoryBarrier imageMemoryBarrier = vkTools::initializers::imageMemoryBarrier2();
		imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		//imageMemoryBarrier.dstAccessMask = 0;
		imageMemoryBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		imageMemoryBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		imageMemoryBarrier.image = presentImage;
		return imageMemoryBarrier;
	}

	//VkImageMemoryBarrier postPresentBarrier(VkImage presentImage)
	//{
	//	VkImageMemoryBarrier imageMemoryBarrier = vkTools::initializers::imageMemoryBarrier();
	//	imageMemoryBarrier.srcAccessMask = 0;
	//	imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	//	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	//	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//	imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	//	imageMemoryBarrier.image = presentImage;
	//	return imageMemoryBarrier;
	//}

	vk::ImageMemoryBarrier postPresentBarrier2(vk::Image presentImage)
	{
		vk::ImageMemoryBarrier imageMemoryBarrier = vkTools::initializers::imageMemoryBarrier2();

		//imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		imageMemoryBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
		imageMemoryBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
		imageMemoryBarrier.image = presentImage;
		return imageMemoryBarrier;
	}

	void destroyUniformData(VkDevice device, vkTools::UniformData *uniformData)
	{
		if (uniformData->mapped != nullptr)
		{
			vkUnmapMemory(device, uniformData->memory);
		}
		vkDestroyBuffer(device, uniformData->buffer, nullptr);
		vkFreeMemory(device, uniformData->memory, nullptr);
	}
	void destroyUniformData2(vk::Device device, vkTools::UniformData2 * uniformData)
	{
		if (uniformData->mapped != nullptr)
		{
			vkUnmapMemory(device, uniformData->memory);
		}
		vkDestroyBuffer(device, uniformData->buffer, nullptr);
		vkFreeMemory(device, uniformData->memory, nullptr);
	}
}










































VkMemoryAllocateInfo vkTools::initializers::memoryAllocateInfo()
{
	//
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = NULL;
	memAllocInfo.allocationSize = 0;
	memAllocInfo.memoryTypeIndex = 0;
	return memAllocInfo;
}

VkCommandBufferAllocateInfo vkTools::initializers::commandBufferAllocateInfo(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t bufferCount)
{
	//
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = level;
	commandBufferAllocateInfo.commandBufferCount = bufferCount;
	return commandBufferAllocateInfo;
}



VkCommandPoolCreateInfo vkTools::initializers::commandPoolCreateInfo()
{
	//
	VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	return cmdPoolCreateInfo;
}

VkCommandBufferBeginInfo vkTools::initializers::commandBufferBeginInfo()
{
	//
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.pNext = NULL;
	return cmdBufferBeginInfo;
}

VkCommandBufferInheritanceInfo vkTools::initializers::commandBufferInheritanceInfo()
{
	//
	VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo = {};
	cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	return cmdBufferInheritanceInfo;
}

VkRenderPassBeginInfo vkTools::initializers::renderPassBeginInfo()
{
	//
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = NULL;
	return renderPassBeginInfo;
}

VkRenderPassCreateInfo vkTools::initializers::renderPassCreateInfo()
{
	//
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = NULL;
	return renderPassCreateInfo;
}



VkImageMemoryBarrier vkTools::initializers::imageMemoryBarrier()
{
	//
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = NULL;
	// Some default values
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	return imageMemoryBarrier;
}

VkBufferMemoryBarrier vkTools::initializers::bufferMemoryBarrier()
{
	//
	VkBufferMemoryBarrier bufferMemoryBarrier = {};
	bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.pNext = NULL;
	return bufferMemoryBarrier;
}

VkMemoryBarrier vkTools::initializers::memoryBarrier()
{
	//
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = NULL;
	return memoryBarrier;
}

VkImageCreateInfo vkTools::initializers::imageCreateInfo()
{
	//
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	return imageCreateInfo;
}

VkSamplerCreateInfo vkTools::initializers::samplerCreateInfo()
{
	//
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = NULL;
	return samplerCreateInfo;
}

VkImageViewCreateInfo vkTools::initializers::imageViewCreateInfo()
{
	//
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = NULL;
	return imageViewCreateInfo;
}

VkFramebufferCreateInfo vkTools::initializers::framebufferCreateInfo()
{
	//
	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.pNext = NULL;
	return framebufferCreateInfo;
}

VkSemaphoreCreateInfo vkTools::initializers::semaphoreCreateInfo()
{
	//
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = NULL;
	semaphoreCreateInfo.flags = 0;
	return semaphoreCreateInfo;
}

VkFenceCreateInfo vkTools::initializers::fenceCreateInfo(VkFenceCreateFlags flags)
{
	//
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = flags;
	return fenceCreateInfo;
}

VkEventCreateInfo vkTools::initializers::eventCreateInfo()
{
	//
	VkEventCreateInfo eventCreateInfo = {};
	eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	return eventCreateInfo;
}

VkSubmitInfo vkTools::initializers::submitInfo()
{
	//
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	return submitInfo;
}

VkViewport vkTools::initializers::viewport(
	float width,
	float height,
	float minDepth,
	float maxDepth)
{
	//
	VkViewport viewport = {};
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	return viewport;
}

VkRect2D vkTools::initializers::rect2D(
	int32_t width,
	int32_t height,
	int32_t offsetX,
	int32_t offsetY)
{
	//
	VkRect2D rect2D = {};
	rect2D.extent.width = width;
	rect2D.extent.height = height;
	rect2D.offset.x = offsetX;
	rect2D.offset.y = offsetY;
	return rect2D;
}

VkBufferCreateInfo vkTools::initializers::bufferCreateInfo()
{
	//
	VkBufferCreateInfo bufCreateInfo = {};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	return bufCreateInfo;
}

VkBufferCreateInfo vkTools::initializers::bufferCreateInfo(
	VkBufferUsageFlags usage,
	VkDeviceSize size)
{
	//
	VkBufferCreateInfo bufCreateInfo = {};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufCreateInfo.pNext = NULL;
	bufCreateInfo.usage = usage;
	bufCreateInfo.size = size;
	bufCreateInfo.flags = 0;
	return bufCreateInfo;
}

VkDescriptorPoolCreateInfo vkTools::initializers::descriptorPoolCreateInfo(
	uint32_t poolSizeCount,
	VkDescriptorPoolSize* pPoolSizes,
	uint32_t maxSets)
{
	//
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = NULL;
	descriptorPoolInfo.poolSizeCount = poolSizeCount;
	descriptorPoolInfo.pPoolSizes = pPoolSizes;
	descriptorPoolInfo.maxSets = maxSets;
	return descriptorPoolInfo;
}

VkDescriptorPoolSize vkTools::initializers::descriptorPoolSize(
	VkDescriptorType type,
	uint32_t descriptorCount)
{
	//
	VkDescriptorPoolSize descriptorPoolSize = {};
	descriptorPoolSize.type = type;
	descriptorPoolSize.descriptorCount = descriptorCount;
	return descriptorPoolSize;
}

VkDescriptorSetLayoutBinding vkTools::initializers::descriptorSetLayoutBinding(
	VkDescriptorType type,
	VkShaderStageFlags stageFlags,
	uint32_t binding,
	uint32_t count)
{
	//
	VkDescriptorSetLayoutBinding setLayoutBinding = {};
	setLayoutBinding.descriptorType = type;
	setLayoutBinding.stageFlags = stageFlags;
	setLayoutBinding.binding = binding;
	setLayoutBinding.descriptorCount = count;
	return setLayoutBinding;
}

VkDescriptorSetLayoutCreateInfo vkTools::initializers::descriptorSetLayoutCreateInfo(
	const VkDescriptorSetLayoutBinding* pBindings,
	uint32_t bindingCount)
{
	//
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext = NULL;
	descriptorSetLayoutCreateInfo.pBindings = pBindings;
	descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
	return descriptorSetLayoutCreateInfo;
}

VkPipelineLayoutCreateInfo vkTools::initializers::pipelineLayoutCreateInfo(
	const VkDescriptorSetLayout* pSetLayouts,
	uint32_t setLayoutCount)
{
	//
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = NULL;
	pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
	pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
	return pipelineLayoutCreateInfo;
}

VkDescriptorSetAllocateInfo vkTools::initializers::descriptorSetAllocateInfo(
	VkDescriptorPool descriptorPool,
	const VkDescriptorSetLayout* pSetLayouts,
	uint32_t descriptorSetCount)
{
	//
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = NULL;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
	descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
	return descriptorSetAllocateInfo;
}

VkDescriptorImageInfo vkTools::initializers::descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
{
	//
	VkDescriptorImageInfo descriptorImageInfo = {};
	descriptorImageInfo.sampler = sampler;
	descriptorImageInfo.imageView = imageView;
	descriptorImageInfo.imageLayout = imageLayout;
	return descriptorImageInfo;
}

VkWriteDescriptorSet vkTools::initializers::writeDescriptorSet(
	VkDescriptorSet dstSet,
	VkDescriptorType type,
	uint32_t binding,
	VkDescriptorBufferInfo* bufferInfo)
{
	//
	VkWriteDescriptorSet writeDescriptorSet = {};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.pNext = NULL;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.pBufferInfo = bufferInfo;
	// Default value in all examples
	writeDescriptorSet.descriptorCount = 1;
	return writeDescriptorSet;
}

VkWriteDescriptorSet vkTools::initializers::writeDescriptorSet(
	VkDescriptorSet dstSet,
	VkDescriptorType type,
	uint32_t binding,
	VkDescriptorImageInfo * imageInfo)
{
	//
	VkWriteDescriptorSet writeDescriptorSet = {};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.pNext = NULL;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.pImageInfo = imageInfo;
	// Default value in all examples
	writeDescriptorSet.descriptorCount = 1;
	return writeDescriptorSet;
}

VkVertexInputBindingDescription vkTools::initializers::vertexInputBindingDescription(
	uint32_t binding,
	uint32_t stride,
	VkVertexInputRate inputRate)
{
	//
	VkVertexInputBindingDescription vInputBindDescription = {};
	vInputBindDescription.binding = binding;
	vInputBindDescription.stride = stride;
	vInputBindDescription.inputRate = inputRate;
	return vInputBindDescription;
}

VkVertexInputAttributeDescription vkTools::initializers::vertexInputAttributeDescription(
	uint32_t binding,
	uint32_t location,
	VkFormat format,
	uint32_t offset)
{
	//
	VkVertexInputAttributeDescription vInputAttribDescription = {};
	vInputAttribDescription.location = location;
	vInputAttribDescription.binding = binding;
	vInputAttribDescription.format = format;
	vInputAttribDescription.offset = offset;
	return vInputAttribDescription;
}

VkPipelineVertexInputStateCreateInfo vkTools::initializers::pipelineVertexInputStateCreateInfo()
{
	//
	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineVertexInputStateCreateInfo.pNext = NULL;
	return pipelineVertexInputStateCreateInfo;
}

VkPipelineInputAssemblyStateCreateInfo vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
	VkPrimitiveTopology topology,
	VkPipelineInputAssemblyStateCreateFlags flags,
	VkBool32 primitiveRestartEnable)
{
	//
	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
	pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineInputAssemblyStateCreateInfo.topology = topology;
	pipelineInputAssemblyStateCreateInfo.flags = flags;
	pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
	return pipelineInputAssemblyStateCreateInfo;
}

VkPipelineRasterizationStateCreateInfo vkTools::initializers::pipelineRasterizationStateCreateInfo(
	VkPolygonMode polygonMode,
	VkCullModeFlags cullMode,
	VkFrontFace frontFace,
	VkPipelineRasterizationStateCreateFlags flags)
{
	//
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
	pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
	pipelineRasterizationStateCreateInfo.cullMode = cullMode;
	pipelineRasterizationStateCreateInfo.frontFace = frontFace;
	pipelineRasterizationStateCreateInfo.flags = flags;
	pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
	return pipelineRasterizationStateCreateInfo;
}

VkPipelineColorBlendAttachmentState vkTools::initializers::pipelineColorBlendAttachmentState(
	VkColorComponentFlags colorWriteMask,
	VkBool32 blendEnable)
{
	//
	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
	pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
	pipelineColorBlendAttachmentState.blendEnable = blendEnable;
	return pipelineColorBlendAttachmentState;
}

VkPipelineColorBlendStateCreateInfo vkTools::initializers::pipelineColorBlendStateCreateInfo(
	uint32_t attachmentCount,
	const VkPipelineColorBlendAttachmentState * pAttachments)
{
	//
	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
	pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineColorBlendStateCreateInfo.pNext = NULL;
	pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
	pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
	return pipelineColorBlendStateCreateInfo;
}

VkPipelineDepthStencilStateCreateInfo vkTools::initializers::pipelineDepthStencilStateCreateInfo(
	VkBool32 depthTestEnable,
	VkBool32 depthWriteEnable,
	VkCompareOp depthCompareOp)
{
	//
	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
	pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
	pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
	pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
	pipelineDepthStencilStateCreateInfo.front = pipelineDepthStencilStateCreateInfo.back;
	pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	return pipelineDepthStencilStateCreateInfo;
}

VkPipelineViewportStateCreateInfo vkTools::initializers::pipelineViewportStateCreateInfo(
	uint32_t viewportCount,
	uint32_t scissorCount,
	VkPipelineViewportStateCreateFlags flags)
{
	//
	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
	pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.viewportCount = viewportCount;
	pipelineViewportStateCreateInfo.scissorCount = scissorCount;
	pipelineViewportStateCreateInfo.flags = flags;
	return pipelineViewportStateCreateInfo;
}

VkPipelineMultisampleStateCreateInfo vkTools::initializers::pipelineMultisampleStateCreateInfo(
	VkSampleCountFlagBits rasterizationSamples,
	VkPipelineMultisampleStateCreateFlags flags)
{
	//
	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
	pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
	return pipelineMultisampleStateCreateInfo;
}

VkPipelineDynamicStateCreateInfo vkTools::initializers::pipelineDynamicStateCreateInfo(
	const VkDynamicState * pDynamicStates,
	uint32_t dynamicStateCount,
	VkPipelineDynamicStateCreateFlags flags)
{
	//
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
	pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
	pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
	return pipelineDynamicStateCreateInfo;
}

VkPipelineTessellationStateCreateInfo vkTools::initializers::pipelineTessellationStateCreateInfo(uint32_t patchControlPoints)
{
	//
	VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo = {};
	pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
	return pipelineTessellationStateCreateInfo;
}

VkGraphicsPipelineCreateInfo vkTools::initializers::pipelineCreateInfo(
	VkPipelineLayout layout,
	VkRenderPass renderPass,
	VkPipelineCreateFlags flags)
{
	//
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pNext = NULL;
	pipelineCreateInfo.layout = layout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.flags = flags;
	return pipelineCreateInfo;
}

VkComputePipelineCreateInfo vkTools::initializers::computePipelineCreateInfo(VkPipelineLayout layout, VkPipelineCreateFlags flags)
{
	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = layout;
	computePipelineCreateInfo.flags = flags;
	return computePipelineCreateInfo;
}

VkPushConstantRange vkTools::initializers::pushConstantRange(
	VkShaderStageFlags stageFlags,
	uint32_t size,
	uint32_t offset)
{
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = stageFlags;
	pushConstantRange.offset = offset;
	pushConstantRange.size = size;
	return pushConstantRange;
}

























































































vk::MemoryAllocateInfo vkTools::initializers::memoryAllocateInfo2()
{
	vk::MemoryAllocateInfo memAllocInfo;
	memAllocInfo.pNext = NULL;
	memAllocInfo.allocationSize = 0;
	memAllocInfo.memoryTypeIndex = 0;
	return memAllocInfo;
}

vk::CommandBufferAllocateInfo vkTools::initializers::commandBufferAllocateInfo2(vk::CommandPool commandPool, vk::CommandBufferLevel level, uint32_t bufferCount)
{
	vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = level;
	commandBufferAllocateInfo.commandBufferCount = bufferCount;
	return commandBufferAllocateInfo;
}

// unnecessary
vk::CommandPoolCreateInfo vkTools::initializers::commandPoolCreateInfo2()
{
	vk::CommandPoolCreateInfo cmdPoolCreateInfo;
	return cmdPoolCreateInfo;
}

// unnecessary
vk::CommandBufferBeginInfo vkTools::initializers::commandBufferBeginInfo2()
{
	vk::CommandBufferBeginInfo cmdBufferBeginInfo;
	cmdBufferBeginInfo.pNext = NULL;
	return cmdBufferBeginInfo;
}

// unnecessary
vk::CommandBufferInheritanceInfo vkTools::initializers::commandBufferInheritanceInfo2()
{
	vk::CommandBufferInheritanceInfo cmdBufferInheritanceInfo;
	return cmdBufferInheritanceInfo;
}

// unnecessary
vk::RenderPassBeginInfo vkTools::initializers::renderPassBeginInfo2()
{
	vk::RenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.pNext = NULL;
	return renderPassBeginInfo;
}

// unnecessary
vk::RenderPassCreateInfo vkTools::initializers::renderPassCreateInfo2()
{
	vk::RenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.pNext = NULL;
	return renderPassCreateInfo;
}

// not sure if unnecessary
vk::ImageMemoryBarrier vkTools::initializers::imageMemoryBarrier2()
{
	vk::ImageMemoryBarrier imageMemoryBarrier;
	imageMemoryBarrier.pNext = NULL;
	// Some default values
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	return imageMemoryBarrier;
}

// unnecessary
vk::BufferMemoryBarrier vkTools::initializers::bufferMemoryBarrier2()
{
	vk::BufferMemoryBarrier bufferMemoryBarrier;
	bufferMemoryBarrier.pNext = NULL;
	return bufferMemoryBarrier;
}

// not sure if unnecessary
VkMemoryBarrier vkTools::initializers::memoryBarrier2()
{
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = NULL;
	return memoryBarrier;
}

// unnecessary
vk::ImageCreateInfo vkTools::initializers::imageCreateInfo2()
{
	vk::ImageCreateInfo imageCreateInfo;
	imageCreateInfo.pNext = NULL;
	return imageCreateInfo;
}

// unnecessary
vk::SamplerCreateInfo vkTools::initializers::samplerCreateInfo2()
{
	vk::SamplerCreateInfo samplerCreateInfo;
	samplerCreateInfo.pNext = NULL;
	return samplerCreateInfo;
}

// unnecessary
vk::ImageViewCreateInfo vkTools::initializers::imageViewCreateInfo2()
{
	vk::ImageViewCreateInfo imageViewCreateInfo;
	imageViewCreateInfo.pNext = NULL;
	return imageViewCreateInfo;
}

// unnecessary
vk::FramebufferCreateInfo vkTools::initializers::framebufferCreateInfo2()
{
	vk::FramebufferCreateInfo framebufferCreateInfo;
	framebufferCreateInfo.pNext = NULL;
	return framebufferCreateInfo;
}

// unnecessary
vk::SemaphoreCreateInfo vkTools::initializers::semaphoreCreateInfo2()
{
	vk::SemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.pNext = NULL;
	//semaphoreCreateInfo.flags = 0;
	return semaphoreCreateInfo;
}

// not sure if unnecessary
vk::FenceCreateInfo vkTools::initializers::fenceCreateInfo2(vk::FenceCreateFlags flags)
{
	vk::FenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.flags = flags;
	return fenceCreateInfo;
}

// unnecessary
vk::EventCreateInfo vkTools::initializers::eventCreateInfo2()
{
	vk::EventCreateInfo eventCreateInfo;
	return eventCreateInfo;
}







// unnecessary
vk::SubmitInfo vkTools::initializers::submitInfo2()
{
	vk::SubmitInfo submitInfo;
	submitInfo.pNext = NULL;
	return submitInfo;
}

vk::Viewport vkTools::initializers::viewport2(float width, float height, float minDepth, float maxDepth)
{
	vk::Viewport viewport;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	return viewport;
}

vk::Rect2D vkTools::initializers::rect2D2(int32_t width, int32_t height, int32_t offsetX, int32_t offsetY)
{
	vk::Rect2D rect2D;
	rect2D.extent.width = width;
	rect2D.extent.height = height;
	rect2D.offset.x = offsetX;
	rect2D.offset.y = offsetY;
	return rect2D;
}

// unnecessary
vk::BufferCreateInfo vkTools::initializers::bufferCreateInfo2()
{
	vk::BufferCreateInfo bufCreateInfo;
	return bufCreateInfo;
}

vk::BufferCreateInfo vkTools::initializers::bufferCreateInfo2(vk::BufferUsageFlags usage, vk::DeviceSize size)
{
	vk::BufferCreateInfo bufCreateInfo;
	bufCreateInfo.pNext = NULL;
	bufCreateInfo.usage = usage;
	bufCreateInfo.size = size;
	//bufCreateInfo.flags = 0;
	return bufCreateInfo;
}

vk::DescriptorPoolCreateInfo vkTools::initializers::descriptorPoolCreateInfo2(uint32_t poolSizeCount, vk::DescriptorPoolSize * pPoolSizes, uint32_t maxSets)
{
	vk::DescriptorPoolCreateInfo descriptorPoolInfo;
	//descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = NULL;
	descriptorPoolInfo.poolSizeCount = poolSizeCount;
	descriptorPoolInfo.pPoolSizes = pPoolSizes;
	descriptorPoolInfo.maxSets = maxSets;
	return descriptorPoolInfo;
}

vk::DescriptorPoolSize vkTools::initializers::descriptorPoolSize2(vk::DescriptorType type, uint32_t descriptorCount)
{
	vk::DescriptorPoolSize descriptorPoolSize;
	descriptorPoolSize.type = type;
	descriptorPoolSize.descriptorCount = descriptorCount;
	return descriptorPoolSize;
}

vk::DescriptorSetLayoutBinding vkTools::initializers::descriptorSetLayoutBinding2(vk::DescriptorType type, vk::ShaderStageFlags stageFlags, uint32_t binding, uint32_t count)
{
	vk::DescriptorSetLayoutBinding setLayoutBinding;
	setLayoutBinding.descriptorType = type;
	setLayoutBinding.stageFlags = stageFlags;
	setLayoutBinding.binding = binding;
	setLayoutBinding.descriptorCount = count;
	return setLayoutBinding;
}

vk::DescriptorSetLayoutCreateInfo vkTools::initializers::descriptorSetLayoutCreateInfo2(const vk::DescriptorSetLayoutBinding * pBindings, uint32_t bindingCount)
{
	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	//descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext = NULL;
	descriptorSetLayoutCreateInfo.pBindings = pBindings;
	descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
	return descriptorSetLayoutCreateInfo;
}

vk::PipelineLayoutCreateInfo vkTools::initializers::pipelineLayoutCreateInfo2(const vk::DescriptorSetLayout * pSetLayouts, uint32_t setLayoutCount)
{
	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	//pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = NULL;
	pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
	pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
	return pipelineLayoutCreateInfo;
}

vk::DescriptorSetAllocateInfo vkTools::initializers::descriptorSetAllocateInfo2(vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayout * pSetLayouts, uint32_t descriptorSetCount)
{
	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
	//descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = NULL;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
	descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
	return descriptorSetAllocateInfo;
}

vk::DescriptorImageInfo vkTools::initializers::descriptorImageInfo2(vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout imageLayout)
{
	vk::DescriptorImageInfo descriptorImageInfo;
	descriptorImageInfo.sampler = sampler;
	descriptorImageInfo.imageView = imageView;
	descriptorImageInfo.imageLayout = imageLayout;
	return descriptorImageInfo;
}

vk::WriteDescriptorSet vkTools::initializers::writeDescriptorSet2(vk::DescriptorSet dstSet, vk::DescriptorType type, uint32_t binding, vk::DescriptorBufferInfo * bufferInfo)
{
	vk::WriteDescriptorSet writeDescriptorSet;
	//writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.pNext = NULL;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.pBufferInfo = bufferInfo;
	// Default value in all examples
	writeDescriptorSet.descriptorCount = 1;
	return writeDescriptorSet;
}

vk::WriteDescriptorSet vkTools::initializers::writeDescriptorSet2(vk::DescriptorSet dstSet, vk::DescriptorType type, uint32_t binding, vk::DescriptorImageInfo * imageInfo)
{
	vk::WriteDescriptorSet writeDescriptorSet;
	//writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.pNext = NULL;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.pImageInfo = imageInfo;
	// Default value in all examples
	writeDescriptorSet.descriptorCount = 1;
	return writeDescriptorSet;
}

vk::VertexInputBindingDescription vkTools::initializers::vertexInputBindingDescription2(uint32_t binding, uint32_t stride, vk::VertexInputRate inputRate)
{
	vk::VertexInputBindingDescription vInputBindDescription;
	vInputBindDescription.binding = binding;
	vInputBindDescription.stride = stride;
	vInputBindDescription.inputRate = inputRate;
	return vInputBindDescription;
}

vk::VertexInputAttributeDescription vkTools::initializers::vertexInputAttributeDescription2(uint32_t binding, uint32_t location, vk::Format format, uint32_t offset)
{
	vk::VertexInputAttributeDescription vInputAttribDescription;
	vInputAttribDescription.location = location;
	vInputAttribDescription.binding = binding;
	vInputAttribDescription.format = format;
	vInputAttribDescription.offset = offset;
	return vInputAttribDescription;
}

vk::PipelineVertexInputStateCreateInfo vkTools::initializers::pipelineVertexInputStateCreateInfo2()
{
	vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
	//pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineVertexInputStateCreateInfo.pNext = NULL;
	return pipelineVertexInputStateCreateInfo;
}

vk::PipelineInputAssemblyStateCreateInfo vkTools::initializers::pipelineInputAssemblyStateCreateInfo2(vk::PrimitiveTopology topology, vk::PipelineInputAssemblyStateCreateFlags flags, vk::Bool32 primitiveRestartEnable)
{
	vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
	//pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineInputAssemblyStateCreateInfo.topology = topology;
	pipelineInputAssemblyStateCreateInfo.flags = flags;
	pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
	return pipelineInputAssemblyStateCreateInfo;
}

vk::PipelineRasterizationStateCreateInfo vkTools::initializers::pipelineRasterizationStateCreateInfo2(vk::PolygonMode polygonMode, vk::CullModeFlags cullMode, vk::FrontFace frontFace, vk::PipelineRasterizationStateCreateFlags flags)
{
	vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
	//pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
	pipelineRasterizationStateCreateInfo.cullMode = cullMode;
	pipelineRasterizationStateCreateInfo.frontFace = frontFace;
	pipelineRasterizationStateCreateInfo.flags = flags;
	pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
	return pipelineRasterizationStateCreateInfo;
}

vk::PipelineColorBlendAttachmentState vkTools::initializers::pipelineColorBlendAttachmentState2(vk::ColorComponentFlags colorWriteMask, vk::Bool32 blendEnable)
{
	vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
	pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
	pipelineColorBlendAttachmentState.blendEnable = blendEnable;
	return pipelineColorBlendAttachmentState;
}

vk::PipelineColorBlendStateCreateInfo vkTools::initializers::pipelineColorBlendStateCreateInfo2(uint32_t attachmentCount, const vk::PipelineColorBlendAttachmentState * pAttachments)
{
	vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
	//pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineColorBlendStateCreateInfo.pNext = NULL;
	pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
	pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
	return pipelineColorBlendStateCreateInfo;
}

vk::PipelineDepthStencilStateCreateInfo vkTools::initializers::pipelineDepthStencilStateCreateInfo2(vk::Bool32 depthTestEnable, vk::Bool32 depthWriteEnable, vk::CompareOp depthCompareOp)
{
	vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo;
	//pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
	pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
	pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
	pipelineDepthStencilStateCreateInfo.front = pipelineDepthStencilStateCreateInfo.back;
	pipelineDepthStencilStateCreateInfo.back.compareOp = vk::CompareOp::eAlways;
	return pipelineDepthStencilStateCreateInfo;
}

vk::PipelineViewportStateCreateInfo vkTools::initializers::pipelineViewportStateCreateInfo2(uint32_t viewportCount, uint32_t scissorCount, vk::PipelineViewportStateCreateFlags flags)
{
	vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
	//pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.viewportCount = viewportCount;
	pipelineViewportStateCreateInfo.scissorCount = scissorCount;
	pipelineViewportStateCreateInfo.flags = flags;
	return pipelineViewportStateCreateInfo;
}

vk::PipelineMultisampleStateCreateInfo vkTools::initializers::pipelineMultisampleStateCreateInfo2(vk::SampleCountFlagBits rasterizationSamples, vk::PipelineMultisampleStateCreateFlags flags)
{
	vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;
	//pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
	return pipelineMultisampleStateCreateInfo;
}

vk::PipelineDynamicStateCreateInfo vkTools::initializers::pipelineDynamicStateCreateInfo2(const vk::DynamicState * pDynamicStates, uint32_t dynamicStateCount, vk::PipelineDynamicStateCreateFlags flags)
{
	vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
	//pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
	pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
	return pipelineDynamicStateCreateInfo;
}

vk::PipelineTessellationStateCreateInfo vkTools::initializers::pipelineTessellationStateCreateInfo2(uint32_t patchControlPoints)
{
	vk::PipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo;
	//pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
	return pipelineTessellationStateCreateInfo;
}

vk::GraphicsPipelineCreateInfo vkTools::initializers::pipelineCreateInfo2(vk::PipelineLayout layout, vk::RenderPass renderPass, vk::PipelineCreateFlags flags)
{
	vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
	//pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pNext = NULL;
	pipelineCreateInfo.layout = layout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.flags = flags;
	return pipelineCreateInfo;
}

vk::ComputePipelineCreateInfo vkTools::initializers::computePipelineCreateInfo2(vk::PipelineLayout layout, vk::PipelineCreateFlags flags)
{
	vk::ComputePipelineCreateInfo computePipelineCreateInfo;
	//computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = layout;
	computePipelineCreateInfo.flags = flags;
	return computePipelineCreateInfo;
}

vk::PushConstantRange vkTools::initializers::pushConstantRange2(vk::ShaderStageFlags stageFlags, uint32_t size, uint32_t offset)
{
	vk::PushConstantRange pushConstantRange;
	pushConstantRange.stageFlags = stageFlags;
	pushConstantRange.offset = offset;
	pushConstantRange.size = size;
	return pushConstantRange;
}






