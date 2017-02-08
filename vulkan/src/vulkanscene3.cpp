/*
* Vulkan Demo Scene
*
* Don't take this a an example, it's more of a personal playground
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* Note : Different license than the other examples!
*
* This code is licensed under the Mozilla Public License Version 2.0 (http://opensource.org/licenses/MPL-2.0)
*/

#include "vulkanApp.h"
#include "vulkanOffscreenExampleBase.hpp"




// Maximum number of bones per mesh
// Must not be higher than same const in skinning shader
#define MAX_BONES 64
// Maximum number of bones per vertex
#define MAX_BONES_PER_VERTEX 4
// Maximum number of skinned meshes (by 65k uniform limit)
#define MAX_SKINNED_MESHES 10
// Texture properties
#define TEX_DIM 1024





std::vector<vkx::VertexLayout> meshVertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
};


std::vector<vkx::VertexLayout> skinnedMeshVertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4
};



inline size_t alignedSize(size_t align, size_t sz) {
	return ((sz + align - 1) / align)*align;
}



inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from) {
	glm::mat4 to;
	to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
	to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
	to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
	to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;
	return to;
}



// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void* alignedAlloc(size_t size, size_t alignment) {
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else 
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void alignedFree(void* data) {
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else 
	free(data);
#endif
}





class VulkanExample : public /*vkx::vulkanApp*/ vkx::OffscreenExampleBase {

public:


	std::vector<std::shared_ptr<vkx::Mesh>> meshes;
	std::vector<std::shared_ptr<vkx::Model>> models;
	std::vector<std::shared_ptr<vkx::SkinnedMesh>> skinnedMeshes;

	std::vector<std::shared_ptr<vkx::PhysicsObject>> physicsObjects;

	struct {
		vkx::MeshBuffer example;
		vkx::MeshBuffer quad;
	} meshBuffers;

	//std::vector<std::shared_ptr<vkx::SkinnedMesh>> skinnedMeshes;


	struct {
		vkx::CreateBufferResult sceneVS;// scene data
		vkx::CreateBufferResult matrixVS;// matrix data
		vkx::CreateBufferResult materialVS;// material data
		vkx::CreateBufferResult bonesVS;// bone data for all skinned meshes // max of 1000 skinned meshes w/64 bones/mesh
	} uniformData;


	// static scene uniform buffer
	struct {
		//glm::mat4 model;// todo: remove
		glm::mat4 view;
		glm::mat4 projection;

		glm::vec4 lightPos;
		glm::vec4 cameraPos;
		glm::mat4 bones[MAX_BONES*MAX_SKINNED_MESHES];
	} uboScene;

	// todo: fix this
	struct MatrixNode {
		glm::mat4 model;
		glm::mat4 boneIndex;
	};

	std::vector<MatrixNode> matrixNodes;

	// material properties not defined here
	std::vector<vkx::MaterialProperties> materialNodes;


	// bone data uniform buffer
	struct {
		glm::mat4 bones[1];
	} uboBoneData;



	unsigned int alignedMatrixSize;
	unsigned int alignedMaterialSize;

	size_t dynamicAlignment;

	float globalP = 0.0f;

	bool debugDisplay = false;

	//glm::vec3 lightPos = glm::vec3(1.0f, -2.0f, 2.0f);
	glm::vec4 lightPos = glm::vec4(1.0f, -2.0f, 2.0f, 1.0f);

	std::string consoleLog;



	// todo: remove this:
	struct {
		vkx::Texture colorMap;
		vkx::Texture floor;
	} textures;




	struct {
		vk::Pipeline meshes;
		vk::Pipeline skinnedMeshes;
		vk::Pipeline blending;
		vk::Pipeline wireframe;


		vk::Pipeline deferred;
		vk::Pipeline offscreen;
		vk::Pipeline debug;
	} pipelines;



	struct {
		vk::PipelineLayout basic;

		vk::PipelineLayout deferred;

		vk::PipelineLayout offscreen;
	} pipelineLayouts;



	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
	std::vector<vk::DescriptorSet> descriptorSets;
	std::vector<vk::DescriptorPool> descriptorPools;


	struct {
		vk::PipelineVertexInputStateCreateInfo inputState;
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vk::PipelineVertexInputStateCreateInfo inputState;
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	} verticesDeferred;




	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
	} uboVS, uboOffscreenVS;

	struct Light {
		glm::vec4 position;
		glm::vec4 color;
		float radius;
		float quadraticFalloff;
		float linearFalloff;
		float _pad;
	};

	struct {
		Light lights[5];
		glm::vec4 viewPos;
	} uboFSLights;


	vk::DescriptorPool descriptorPoolDeferred;
	vk::DescriptorSetLayout descriptorSetLayoutDeferred;

	struct {
		vk::DescriptorSet basic;

		vk::DescriptorSet offscreen;
	} descriptorSetsDeferred;



	struct {
		vkx::UniformData vsFullScreen;
		vkx::UniformData vsOffscreen;
		vkx::UniformData fsLights;
	} uniformDataDeferred;

	vk::CommandBuffer offscreenCmdBuffer;













	//struct Offscreen {
	//	const vkx::Context& context;
	//	bool active{ true };
	//	vk::RenderPass renderPass;
	//	vk::CommandBuffer cmdBuffer;
	//	vk::Semaphore renderComplete;

	//	glm::uvec2 size;
	//	std::vector<vk::Format> colorFormats{ { vk::Format::eB8G8R8A8Unorm } };
	//	// This value is chosen as an invalid default that signals that the code should pick a specific depth buffer
	//	// Alternative, you can set this to undefined to explicitly declare you want no depth buffer.
	//	vk::Format depthFormat = vk::Format::eR8Uscaled;
	//	std::vector<vkx::Framebuffer> framebuffers{ 1 };
	//	vk::ImageUsageFlags attachmentUsage{ vk::ImageUsageFlagBits::eSampled };
	//	vk::ImageUsageFlags depthAttachmentUsage;
	//	vk::ImageLayout colorFinalLayout{ vk::ImageLayout::eShaderReadOnlyOptimal };
	//	vk::ImageLayout depthFinalLayout{ vk::ImageLayout::eUndefined };

	//	Offscreen(const vkx::Context& context) : context(context) {}

	//	void prepare() {
	//		assert(!colorFormats.empty());
	//		assert(size != glm::uvec2());

	//		if (depthFormat == vk::Format::eR8Uscaled) {
	//			depthFormat = vkx::getSupportedDepthFormat(context.physicalDevice);
	//		}

	//		cmdBuffer = context.device.allocateCommandBuffers(vkx::commandBufferAllocateInfo(context.getCommandPool(), vk::CommandBufferLevel::ePrimary, 1))[0];
	//		renderComplete = context.device.createSemaphore(vk::SemaphoreCreateInfo());
	//		if (!renderPass) {
	//			prepareRenderPass();
	//		}

	//		for (auto& framebuffer : framebuffers) {
	//			framebuffer.create(context, size, colorFormats, depthFormat, renderPass, attachmentUsage, depthAttachmentUsage);
	//		}
	//		prepareSampler();
	//	}

	//	void destroy() {
	//		for (auto& framebuffer : framebuffers) {
	//			framebuffer.destroy();
	//		}
	//		framebuffers.clear();
	//		context.device.freeCommandBuffers(context.getCommandPool(), cmdBuffer);
	//		context.device.destroyRenderPass(renderPass);
	//		context.device.destroySemaphore(renderComplete);
	//	}

	//protected:
	//	void prepareSampler() {
	//		// Create sampler
	//		vk::SamplerCreateInfo sampler;
	//		sampler.magFilter = vk::Filter::eLinear;
	//		sampler.minFilter = vk::Filter::eLinear;
	//		sampler.mipmapMode = vk::SamplerMipmapMode::eLinear;
	//		sampler.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	//		sampler.addressModeV = sampler.addressModeU;
	//		sampler.addressModeW = sampler.addressModeU;
	//		sampler.mipLodBias = 0.0f;
	//		sampler.maxAnisotropy = 0;
	//		sampler.compareOp = vk::CompareOp::eNever;
	//		sampler.minLod = 0.0f;
	//		sampler.maxLod = 0.0f;
	//		sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	//		for (auto& framebuffer : framebuffers) {
	//			if (attachmentUsage | vk::ImageUsageFlagBits::eSampled) {
	//				for (auto& color : framebuffer.colors) {
	//					color.sampler = context.device.createSampler(sampler);
	//				}
	//			}
	//			if (depthAttachmentUsage | vk::ImageUsageFlagBits::eSampled) {
	//				framebuffer.depth.sampler = context.device.createSampler(sampler);
	//			}
	//		}
	//	}

	//	virtual void prepareRenderPass() {
	//		vk::SubpassDescription subpass;
	//		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

	//		std::vector<vk::AttachmentDescription> attachments;
	//		std::vector<vk::AttachmentReference> colorAttachmentReferences;
	//		attachments.resize(colorFormats.size());
	//		colorAttachmentReferences.resize(attachments.size());
	//		// Color attachment
	//		for (size_t i = 0; i < attachments.size(); ++i) {
	//			attachments[i].format = colorFormats[i];
	//			attachments[i].loadOp = vk::AttachmentLoadOp::eClear;
	//			attachments[i].storeOp = colorFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
	//			attachments[i].initialLayout = vk::ImageLayout::eUndefined;
	//			attachments[i].finalLayout = colorFinalLayout;

	//			vk::AttachmentReference& attachmentReference = colorAttachmentReferences[i];
	//			attachmentReference.attachment = i;
	//			attachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	//			subpass.colorAttachmentCount = colorAttachmentReferences.size();
	//			subpass.pColorAttachments = colorAttachmentReferences.data();
	//		}

	//		// Do we have a depth format?
	//		vk::AttachmentReference depthAttachmentReference;
	//		if (depthFormat != vk::Format::eUndefined) {
	//			vk::AttachmentDescription depthAttachment;
	//			depthAttachment.format = depthFormat;
	//			depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	//			// We might be using the depth attacment for something, so preserve it if it's final layout is not undefined
	//			depthAttachment.storeOp = depthFinalLayout == vk::ImageLayout::eUndefined ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
	//			depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
	//			depthAttachment.finalLayout = depthFinalLayout;
	//			attachments.push_back(depthAttachment);
	//			depthAttachmentReference.attachment = attachments.size() - 1;
	//			depthAttachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	//			subpass.pDepthStencilAttachment = &depthAttachmentReference;
	//		}

	//		std::vector<vk::SubpassDependency> subpassDependencies;
	//		{
	//			if ((colorFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (colorFinalLayout != vk::ImageLayout::eUndefined)) {
	//				// Implicit transition 
	//				vk::SubpassDependency dependency;
	//				dependency.srcSubpass = 0;
	//				dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	//				dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	//				dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	//				dependency.dstAccessMask = vkx::accessFlagsForLayout(colorFinalLayout);
	//				dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	//				subpassDependencies.push_back(dependency);
	//			}

	//			if ((depthFinalLayout != vk::ImageLayout::eColorAttachmentOptimal) && (depthFinalLayout != vk::ImageLayout::eUndefined)) {
	//				// Implicit transition 
	//				vk::SubpassDependency dependency;
	//				dependency.srcSubpass = 0;
	//				dependency.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	//				dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;

	//				dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	//				dependency.dstAccessMask = vkx::accessFlagsForLayout(depthFinalLayout);
	//				dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	//				subpassDependencies.push_back(dependency);
	//			}
	//		}

	//		if (renderPass) {
	//			context.device.destroyRenderPass(renderPass);
	//		}

	//		vk::RenderPassCreateInfo renderPassInfo;
	//		renderPassInfo.attachmentCount = attachments.size();
	//		renderPassInfo.pAttachments = attachments.data();
	//		renderPassInfo.subpassCount = 1;
	//		renderPassInfo.pSubpasses = &subpass;
	//		renderPassInfo.dependencyCount = subpassDependencies.size();
	//		renderPassInfo.pDependencies = subpassDependencies.data();
	//		renderPass = context.device.createRenderPass(renderPassInfo);
	//	}
	//} offscreen;





	VulkanExample() : /*vkx::vulkanApp*/ vkx::OffscreenExampleBase(ENABLE_VALIDATION)/*, offscreen(context)*/ {
		// todo: pick better numbers
		// or pick based on screen size
		size.width = 1280;
		size.height = 720;


		camera.setTranslation({ -0.0f, -16.0f, 3.0f });
		glm::quat initialOrientation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		camera.setRotation(initialOrientation);



		matrixNodes.resize(100);
		materialNodes.resize(100);


		// todo: move this somewhere else
		// it doesn't need to be here
		unsigned int alignment = (uint32_t)context.deviceProperties.limits.minUniformBufferOffsetAlignment;
		size_t uboAlignment = context.deviceProperties.limits.minUniformBufferOffsetAlignment;


		//dynamicAlignment = (sizeof(glm::mat4) / uboAlignment) * uboAlignment + ((sizeof(glm::mat4) % uboAlignment) > 0 ? uboAlignment : 0);
		//dynamicAlignment = (sizeof(MatrixNode) / uboAlignment) * uboAlignment + ((sizeof(MatrixNode) % uboAlignment) > 0 ? uboAlignment : 0);


		// todo: fix
		//size_t bufferSize = 100 * dynamicAlignment;
		//MatrixNode2.model = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);
		//modelMatrices = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);
		//modelMatrices = (MatrixNode*)alignedAlloc(bufferSize, dynamicAlignment);


		alignedMatrixSize = (unsigned int)(alignedSize(alignment, sizeof(MatrixNode)));
		alignedMaterialSize = (unsigned int)(alignedSize(alignment, sizeof(vkx::MaterialProperties)));

		//camera.matrixNodes.projection = glm::perspectiveRH(glm::radians(60.0f), (float)size.width / (float)size.height, 0.0001f, 256.0f);

		title = "Vulkan Demo Scene";
	}

	~VulkanExample() {
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class


		// todo: fix this all up


		//offscreen.destroy();

		// destroy pipelines
		device.destroyPipeline(pipelines.meshes);
		device.destroyPipeline(pipelines.skinnedMeshes);


		device.destroyPipeline(pipelines.deferred);
		device.destroyPipeline(pipelines.offscreen);
		device.destroyPipeline(pipelines.debug);



		// destroy pipeline layouts
		device.destroyPipelineLayout(pipelineLayouts.basic);

		device.destroyPipelineLayout(pipelineLayouts.deferred);
		device.destroyPipelineLayout(pipelineLayouts.offscreen);


		//device.destroyDescriptorSetLayout(descriptorSetLayout);

		// destroy uniform buffers
		uniformData.sceneVS.destroy();

		// destroy offscreen uniform buffers
		uniformDataDeferred.vsOffscreen.destroy();
		uniformDataDeferred.vsFullScreen.destroy();
		uniformDataDeferred.fsLights.destroy();

		// destroy offscreen command buffer
		device.freeCommandBuffers(cmdPool, offscreenCmdBuffer);



		for (auto &mesh : meshes) {
			mesh->destroy();
		}

		for (auto &model : models) {
			model->destroy();
		}


		for (auto &skinnedMesh : skinnedMeshes) {
			skinnedMesh->destroy();
		}

		for (auto &physicsObject : physicsObjects) {
			physicsObject->destroy();
		}

		//textures.colorMap.destroy();

		//uniformData.vsScene.destroy();

		//uniformDataDeferred.vsOffscreen.destroy();
		//uniformDataDeferred.vsFullScreen.destroy();
		//uniformDataDeferred.fsLights.destroy();

		//// Destroy and free mesh resources 
		//skinnedMesh->meshBuffer.destroy();
		//delete(skinnedMesh->meshLoader);
		//delete(skinnedMesh);
		//textures.skybox.destroy();

	}







	void prepareVertexDescriptions() {

		struct meshVertex {
			glm::vec3 pos;
			glm::vec2 uv;
			glm::vec3 color;
			glm::vec3 normal;
		};

		struct skinnedMeshVertex {
			glm::vec3 pos;
			glm::vec2 uv;
			glm::vec3 color;
			glm::vec3 normal;

			// Max. four bones per vertex
			float boneWeights[4];
			uint32_t boneIDs[4];
		};



		/* not deferred */

		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(skinnedMeshVertex), vk::VertexInputRate::eVertex);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(6);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);
		// Location 1 : (UV) Texture coordinates
		vertices.attributeDescriptions[1] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32Sfloat, sizeof(float) * 3);
		// Location 2 : Color
		vertices.attributeDescriptions[2] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32B32Sfloat, sizeof(float) * 5);
		// Location 3 : Normal
		vertices.attributeDescriptions[3] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat, sizeof(float) * 8);
		// Location 4 : Bone weights
		vertices.attributeDescriptions[4] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 11);
		// Location 5 : Bone IDs
		vertices.attributeDescriptions[5] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 5, vk::Format::eR32G32B32A32Sint, sizeof(float) * 15);



		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();

		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();







		///* deferred */

		//// Binding description
		//verticesDeferred.bindingDescriptions.resize(1);
		//verticesDeferred.bindingDescriptions[0] =
		//	vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, vkx::vertexSize(/*skinnedMeshVertexLayout*/meshVertexLayout), vk::VertexInputRate::eVertex);

		//// Attribute descriptions
		//// Describes memory layout and shader positions
		//verticesDeferred.attributeDescriptions.resize(6);
		//// Location 0 : Position
		//verticesDeferred.attributeDescriptions[0] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);
		//// Location 1 : (UV) Texture coordinates
		//verticesDeferred.attributeDescriptions[1] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32Sfloat, sizeof(float) * 3);
		//// Location 2 : Color
		//verticesDeferred.attributeDescriptions[2] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32B32Sfloat, sizeof(float) * 5);
		//// Location 3 : Normal
		//verticesDeferred.attributeDescriptions[3] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat, sizeof(float) * 8);
		//// Location 4 : Bone weights
		//verticesDeferred.attributeDescriptions[4] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 11);
		//// Location 5 : Bone IDs
		//verticesDeferred.attributeDescriptions[5] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 5, vk::Format::eR32G32B32A32Sint, sizeof(float) * 15);



		//verticesDeferred.inputState.vertexBindingDescriptionCount = verticesDeferred.bindingDescriptions.size();
		//verticesDeferred.inputState.pVertexBindingDescriptions = verticesDeferred.bindingDescriptions.data();

		//verticesDeferred.inputState.vertexAttributeDescriptionCount = verticesDeferred.attributeDescriptions.size();
		//verticesDeferred.inputState.pVertexAttributeDescriptions = verticesDeferred.attributeDescriptions.data();

	}


	void setupDescriptorPool() {

		// scene data
		std::vector<vk::DescriptorPoolSize> poolSizes0 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),// mostly static data
		};

		vk::DescriptorPoolCreateInfo descriptorPool0Info =
			vkx::descriptorPoolCreateInfo(poolSizes0.size(), poolSizes0.data(), 1);


		vk::DescriptorPool descPool0 = device.createDescriptorPool(descriptorPool0Info);
		descriptorPools.push_back(descPool0);




		// matrix data
		std::vector<vk::DescriptorPoolSize> poolSizes1 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),// non-static data
		};

		vk::DescriptorPoolCreateInfo descriptorPool1Info =
			vkx::descriptorPoolCreateInfo(poolSizes1.size(), poolSizes1.data(), 1);

		vk::DescriptorPool descPool1 = device.createDescriptorPool(descriptorPool1Info);
		descriptorPools.push_back(descPool1);



		// material data
		std::vector<vk::DescriptorPoolSize> poolSizes2 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1),
		};

		vk::DescriptorPoolCreateInfo descriptorPool2Info =
			vkx::descriptorPoolCreateInfo(poolSizes2.size(), poolSizes2.data(), 1);


		vk::DescriptorPool descPool2 = device.createDescriptorPool(descriptorPool2Info);
		descriptorPools.push_back(descPool2);



		// combined image sampler
		std::vector<vk::DescriptorPoolSize> poolSizes3 =
		{
			vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 10000),
		};

		vk::DescriptorPoolCreateInfo descriptorPool3Info =
			vkx::descriptorPoolCreateInfo(poolSizes3.size(), poolSizes3.data(), 10000);


		vk::DescriptorPool descPool3 = device.createDescriptorPool(descriptorPool3Info);
		descriptorPools.push_back(descPool3);

		this->assetManager.materialDescriptorPool = &descriptorPools[3];




		//// bone data
		//std::vector<vk::DescriptorPoolSize> poolSizes4 =
		//{
		//	vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),// static bone data
		//};

		//vk::DescriptorPoolCreateInfo descriptorPool4Info =
		//	vkx::descriptorPoolCreateInfo(poolSizes4.size(), poolSizes4.data(), 1);

		//vk::DescriptorPool descPool4 = device.createDescriptorPool(descriptorPool4Info);
		//descriptorPools.push_back(descPool4);


		//std::vector<vk::DescriptorPoolSize> poolSizesDeferred =
		//{
		//	vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 8),
		//	vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 8)
		//};

		//vk::DescriptorPoolCreateInfo descriptorPoolInfo =
		//	vkx::descriptorPoolCreateInfo(poolSizesDeferred.size(), poolSizesDeferred.data(), 2);

		//descriptorPoolDeferred = device.createDescriptorPool(descriptorPoolInfo);




	}

	void setupDescriptorSetLayout() {


		// descriptor set layout 0
		// scene data

		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings0 =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo0 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings0.data(), descriptorSetLayoutBindings0.size());

		vk::DescriptorSetLayout descriptorSetLayout0 = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo0);
		descriptorSetLayouts.push_back(descriptorSetLayout0);



		// descriptor set layout 1
		// matrix data

		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings1 =
		{
			// Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo1 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings1.data(), descriptorSetLayoutBindings1.size());

		vk::DescriptorSetLayout descriptorSetLayout1 = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo1);
		descriptorSetLayouts.push_back(descriptorSetLayout1);





		// descriptor set layout 2
		// material data

		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings2 =
		{
			// Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo2 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings2.data(), descriptorSetLayoutBindings2.size());

		vk::DescriptorSetLayout descriptorSetLayout2 = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo2);

		descriptorSetLayouts.push_back(descriptorSetLayout2);



		// descriptor set layout 3
		// combined image sampler

		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings3 =
		{
			// Binding 0 : Fragment shader color map image sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo3 =
			vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings3.data(), descriptorSetLayoutBindings3.size());

		vk::DescriptorSetLayout descriptorSetLayout3 = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo3);
		descriptorSetLayouts.push_back(descriptorSetLayout3);

		this->assetManager.materialDescriptorSetLayout = &descriptorSetLayouts[3];







		//// descriptor set layout 4
		//// bone data

		//std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings4 =
		//{
		//	// Binding 0 : Vertex shader uniform buffer
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBuffer,
		//		vk::ShaderStageFlagBits::eVertex,
		//		0),// binding 0
		//};

		//vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo4 =
		//	vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings4.data(), descriptorSetLayoutBindings4.size());

		//vk::DescriptorSetLayout descriptorSetLayout4 = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo4);
		//descriptorSetLayouts.push_back(descriptorSetLayout4);




		// use all descriptor set layouts
		// to form pipeline layout

		//vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
		//	vkx::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());

		//pipelineLayouts.basic = device.createPipelineLayout(pPipelineLayoutCreateInfo);








		//// Deferred shading layout
		//std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindingsDeferred =
		//{
		//	// Binding 0 : Vertex shader uniform buffer
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBuffer,
		//		vk::ShaderStageFlagBits::eVertex,
		//		0),
		//	// Binding 1 : Position texture target / Scene colormap
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eCombinedImageSampler,
		//		vk::ShaderStageFlagBits::eFragment,
		//		1),
		//	// Binding 2 : Normals texture target
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eCombinedImageSampler,
		//		vk::ShaderStageFlagBits::eFragment,
		//		2),
		//	// Binding 3 : Albedo texture target
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eCombinedImageSampler,
		//		vk::ShaderStageFlagBits::eFragment,
		//		3),
		//	// Binding 4 : Fragment shader uniform buffer
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBuffer,
		//		vk::ShaderStageFlagBits::eFragment,
		//		4),
		//};


		//vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfoDeferred =
		//	vkx::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindingsDeferred.data(), descriptorSetLayoutBindingsDeferred.size());

		//descriptorSetLayoutDeferred = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfoDeferred);

		//vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfoDeferred =
		//	vkx::pipelineLayoutCreateInfo(&descriptorSetLayoutDeferred, 1);

		//pipelineLayouts.deferred = device.createPipelineLayout(pPipelineLayoutCreateInfoDeferred);


		//// Offscreen (scene) rendering pipeline layout
		//pipelineLayouts.offscreen = device.createPipelineLayout(pPipelineLayoutCreateInfoDeferred);



	}

	void setupDescriptorSet() {

		// descriptor set 0
		// scene data
		vk::DescriptorSetAllocateInfo descriptorSetInfo0 =
			vkx::descriptorSetAllocateInfo(descriptorPools[0], &descriptorSetLayouts[0], 1);

		std::vector<vk::DescriptorSet> descriptorSets0 = device.allocateDescriptorSets(descriptorSetInfo0);
		descriptorSets.push_back(descriptorSets0[0]);// descriptor set 0




													 // descriptor set 1
													 // matrix data
		vk::DescriptorSetAllocateInfo descriptorSetInfo1 =
			vkx::descriptorSetAllocateInfo(descriptorPools[1], &descriptorSetLayouts[1], 1);

		std::vector<vk::DescriptorSet> descriptorSets1 = device.allocateDescriptorSets(descriptorSetInfo1);
		descriptorSets.push_back(descriptorSets1[0]);// descriptor set 1



													 // descriptor set 2
													 // material data
		vk::DescriptorSetAllocateInfo descriptorSetInfo2 =
			vkx::descriptorSetAllocateInfo(descriptorPools[2], &descriptorSetLayouts[2], 1);

		std::vector<vk::DescriptorSet> descriptorSets2 = device.allocateDescriptorSets(descriptorSetInfo2);
		descriptorSets.push_back(descriptorSets2[0]);// descriptor set 2



													 // descriptor set 3
													 // image sampler
													 //vk::DescriptorSetAllocateInfo descriptorSetInfo3 =
													 //	vkx::descriptorSetAllocateInfo(descriptorPools[3], &descriptorSetLayouts[3], 1);

													 //std::vector<vk::DescriptorSet> descriptorSets3 = device.allocateDescriptorSets(descriptorSetInfo3);
													 //descriptorSets.push_back(descriptorSets3[0]);// descriptor set 3




													 //// descriptor set 4
													 //// bone data
													 //vk::DescriptorSetAllocateInfo descriptorSetInfo4 =
													 //	vkx::descriptorSetAllocateInfo(descriptorPools[4], &descriptorSetLayouts[4], 1);

													 //std::vector<vk::DescriptorSet> descSets4 = device.allocateDescriptorSets(descriptorSetInfo4);
													 //descriptorSets.push_back(descSets4[0]);// descriptor set 4




		std::vector<vk::WriteDescriptorSet> writeDescriptorSets =
		{
			// set 0
			// Binding 0 : Vertex shader uniform buffer
			vkx::writeDescriptorSet(
				descriptorSets[0],// descriptor set 0
				vk::DescriptorType::eUniformBuffer,
				0,// binding 0
				&uniformData.sceneVS.descriptor),

			// set 1
			// vertex shader matrix dynamic buffer
			vkx::writeDescriptorSet(
				descriptorSets[1],// descriptor set 1
				vk::DescriptorType::eUniformBufferDynamic,
				0,// binding 0
				&uniformData.matrixVS.descriptor),


			// set 2
			// fragment shader material dynamic buffer
			vkx::writeDescriptorSet(
				descriptorSets[2],// descriptor set 2
				vk::DescriptorType::eUniformBufferDynamic,
				0,// binding 0
				&uniformData.materialVS.descriptor),


			// set 3 is set later (textures)


			//// set 4
			//// static bone data buffer
			//vkx::writeDescriptorSet(
			//	/*descriptorSets[2]*/descSets4[0],// descriptor set 4
			//	vk::DescriptorType::eUniformBuffer,// static
			//	0,// binding 0
			//	&uniformData.bonesVS.descriptor)

			//// set 4
			//// static bone data buffer
			//vkx::writeDescriptorSet(
			//	descriptorSets[3],// descriptor set 4
			//	vk::DescriptorType::eUniformBuffer,// static
			//	0,// binding 0
			//	&uniformData.bonesVS.descriptor)
		};


		device.updateDescriptorSets(writeDescriptorSets, nullptr);























		//// Textured quad descriptor set
		//vk::DescriptorSetAllocateInfo allocInfo =
		//	vkx::descriptorSetAllocateInfo(descriptorPoolDeferred, &descriptorSetLayoutDeferred, 1);


		//// allocates a descriptor set from descriptorPoolDeferred using layout descriptorSetLayoutDeferred
		//descriptorSetsDeferred.basic = device.allocateDescriptorSets(allocInfo)[0];

		//// Offscreen (scene) // allocates a descriptor set from descriptorPoolDeferred using layout descriptorSetLayoutDeferred
		//descriptorSetsDeferred.offscreen = device.allocateDescriptorSets(allocInfo)[0];


		//// vk::Image descriptor for the offscreen texture targets
		//vk::DescriptorImageInfo texDescriptorPosition =
		//	vkx::descriptorImageInfo(offscreen.framebuffers[0].colors[0].sampler, offscreen.framebuffers[0].colors[0].view, vk::ImageLayout::eShaderReadOnlyOptimal);

		//vk::DescriptorImageInfo texDescriptorNormal =
		//	vkx::descriptorImageInfo(offscreen.framebuffers[0].colors[1].sampler, offscreen.framebuffers[0].colors[1].view, vk::ImageLayout::eShaderReadOnlyOptimal);

		//vk::DescriptorImageInfo texDescriptorAlbedo =
		//	vkx::descriptorImageInfo(offscreen.framebuffers[0].colors[2].sampler, offscreen.framebuffers[0].colors[2].view, vk::ImageLayout::eShaderReadOnlyOptimal);

		//std::vector<vk::WriteDescriptorSet> writeDescriptorSets2 =
		//{
		//	// Binding 0 : Vertex shader uniform buffer
		//	vkx::writeDescriptorSet(
		//		descriptorSetsDeferred.basic,
		//		vk::DescriptorType::eUniformBuffer,
		//		0,
		//		&uniformDataDeferred.vsFullScreen.descriptor),
		//	// Binding 1 : Position texture target
		//	vkx::writeDescriptorSet(
		//		descriptorSetsDeferred.basic,
		//		vk::DescriptorType::eCombinedImageSampler,
		//		1,
		//		&texDescriptorPosition),
		//	// Binding 2 : Normals texture target
		//	vkx::writeDescriptorSet(
		//		descriptorSetsDeferred.basic,
		//		vk::DescriptorType::eCombinedImageSampler,
		//		2,
		//		&texDescriptorNormal),
		//	// Binding 3 : Albedo texture target
		//	vkx::writeDescriptorSet(
		//		descriptorSetsDeferred.basic,
		//		vk::DescriptorType::eCombinedImageSampler,
		//		3,
		//		&texDescriptorAlbedo),
		//	// Binding 4 : Fragment shader uniform buffer
		//	vkx::writeDescriptorSet(
		//		descriptorSetsDeferred.basic,
		//		vk::DescriptorType::eUniformBuffer,
		//		4,
		//		&uniformDataDeferred.fsLights.descriptor),
		//};

		//device.updateDescriptorSets(writeDescriptorSets2, nullptr);





		//vk::DescriptorImageInfo texDescriptorSceneColormap =
		//	vkx::descriptorImageInfo(textures.colorMap.sampler, textures.colorMap.view, vk::ImageLayout::eGeneral);

		//std::vector<vk::WriteDescriptorSet> offscreenWriteDescriptorSets =
		//{
		//	// Binding 0 : Vertex shader uniform buffer
		//	vkx::writeDescriptorSet(
		//		descriptorSetsDeferred.offscreen,
		//		vk::DescriptorType::eUniformBuffer,
		//		0,
		//		&uniformDataDeferred.vsOffscreen.descriptor),
		//	// Binding 1 : Scene color map
		//	vkx::writeDescriptorSet(
		//		descriptorSetsDeferred.offscreen,
		//		vk::DescriptorType::eCombinedImageSampler,
		//		1,
		//		&texDescriptorSceneColormap)
		//};
		//device.updateDescriptorSets(offscreenWriteDescriptorSets, nullptr);




	}


	void preparePipelines() {
		// todo:
		// understand this:

		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;

		vk::PipelineRasterizationStateCreateInfo rasterizationState =
			vkx::pipelineRasterizationStateCreateInfo(
				vk::PolygonMode::eFill,
				vk::CullModeFlagBits::eBack,
				vk::FrontFace::eClockwise);

		vk::PipelineColorBlendAttachmentState blendAttachmentState;
		blendAttachmentState.colorWriteMask = vkx::fullColorWriteMask();

		vk::PipelineColorBlendStateCreateInfo colorBlendState;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;

		vk::PipelineViewportStateCreateInfo viewportState;
		viewportState.scissorCount = 1;
		viewportState.viewportCount = 1;

		vk::PipelineMultisampleStateCreateInfo multisampleState = vkx::pipelineMultisampleStateCreateInfo(vk::SampleCountFlagBits::e1);// added 1/20/17

		std::vector<vk::DynamicState> dynamicStateEnables = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};
		vk::PipelineDynamicStateCreateInfo dynamicState =
			vkx::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());




		// vk::Pipeline for the meshes (armadillo, bunny, etc.)
		// Load shaders
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;


		vk::GraphicsPipelineCreateInfo pipelineCreateInfo =
			vkx::pipelineCreateInfo(pipelineLayouts.basic, renderPass);

		pipelineCreateInfo.pVertexInputState = &vertices.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();



		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/mesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/mesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		pipelines.meshes = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];






		// skinned meshes:
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/skinnedMesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/skinnedMesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
		pipelines.skinnedMeshes = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];





		// vk::Pipeline for the sky box
		//rasterizationState.cullMode = vk::CullModeFlagBits::eFront; // Inverted culling
		//depthStencilState.depthWriteEnable = VK_FALSE; // No depth writes
		//shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/skybox.vert.spv", vk::ShaderStageFlagBits::eVertex);
		//shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/skybox.frag.spv", vk::ShaderStageFlagBits::eFragment);
		//pipelines.skybox = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];


		//// Alpha blended pipeline
		//// transparency
		//rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
		//blendAttachmentState.blendEnable = VK_TRUE;
		//blendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
		//blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcColor;
		//blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcColor;
		//pipelines.blending = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo)[0];


		//// Wire frame rendering pipeline
		//rasterizationState.cullMode = vk::CullModeFlagBits::eBack;
		//blendAttachmentState.blendEnable = VK_FALSE;
		//rasterizationState.polygonMode = vk::PolygonMode::eLine;
		//rasterizationState.lineWidth = 1.0f;
		//pipelines.wireframe = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo)[0];

	}


	void prepareDeferredPipelines() {
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkx::pipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology::eTriangleList);

		vk::PipelineRasterizationStateCreateInfo rasterizationState =
			vkx::pipelineRasterizationStateCreateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise);

		vk::PipelineColorBlendAttachmentState blendAttachmentState =
			vkx::pipelineColorBlendAttachmentState();

		vk::PipelineColorBlendStateCreateInfo colorBlendState =
			vkx::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

		vk::PipelineDepthStencilStateCreateInfo depthStencilState =
			vkx::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, vk::CompareOp::eLessOrEqual);

		vk::PipelineViewportStateCreateInfo viewportState =
			vkx::pipelineViewportStateCreateInfo(1, 1);

		vk::PipelineMultisampleStateCreateInfo multisampleState;

		std::vector<vk::DynamicState> dynamicStateEnables = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};
		vk::PipelineDynamicStateCreateInfo dynamicState;
		dynamicState.dynamicStateCount = dynamicStateEnables.size();
		dynamicState.pDynamicStates = dynamicStateEnables.data();

		// Final fullscreen pass pipeline
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo = vkx::pipelineCreateInfo(pipelineLayouts.deferred, renderPass);
		pipelineCreateInfo.pVertexInputState = &vertices.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();


		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/deferred/deferred.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/deferred/deferred.frag.spv", vk::ShaderStageFlagBits::eFragment);

		pipelines.deferred = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];


		// Debug display pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/deferred/debug.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/deferred/debug.frag.spv", vk::ShaderStageFlagBits::eFragment);
		pipelines.debug = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];


		// Offscreen pipeline
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/deferred/mrt.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/deferred/mrt.frag.spv", vk::ShaderStageFlagBits::eFragment);

		// Separate render pass
		pipelineCreateInfo.renderPass = offscreen.renderPass;

		// Separate layout
		pipelineCreateInfo.layout = pipelineLayouts.offscreen;

		// Blend attachment states required for all color attachments
		// This is important, as color write mask will otherwise be 0x0 and you
		// won't see anything rendered to the attachment
		std::array<vk::PipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
			vkx::pipelineColorBlendAttachmentState(),
			vkx::pipelineColorBlendAttachmentState(),
			vkx::pipelineColorBlendAttachmentState()
		};

		colorBlendState.attachmentCount = blendAttachmentStates.size();
		colorBlendState.pAttachments = blendAttachmentStates.data();

		pipelines.offscreen = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];
	}





	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers() {
		// Vertex shader uniform buffer block
		uniformData.sceneVS = context.createUniformBuffer(uboScene);
		uniformData.matrixVS = context.createDynamicUniformBuffer(matrixNodes);
		uniformData.materialVS = context.createDynamicUniformBuffer(materialNodes);
		uniformData.bonesVS = context.createUniformBuffer(uboBoneData);

		//uniformData.matrixVS = context.createDynamicUniformBufferManual(modelMatrices, 100);




		//uniformData.matrixVS = context.createDynamicUniformBuffer(matrixNodes);

		updateSceneBuffer();// update scene ubo
		updateMatrixBuffer();// update matrix ubo
		updateMaterialBuffer();// update material ubo
							   // todo: update bonedata ubo

	}


	void updateSceneBuffer() {

		camera.updateViewMatrix();

		uboScene.view = camera.matrices.view;
		uboScene.projection = camera.matrices.projection;
		//uboScene.cameraPos = camera.transform.translation;
		uboScene.cameraPos = glm::vec4(camera.transform.translation, 0.0f);
		uniformData.sceneVS.copy(uboScene);
	}

	void updateMatrixBuffer() {
		// todo:
		uniformData.matrixVS.copy(matrixNodes);
		//uniformData.matrixVS.copy(modelMatrices);

		//memcpy(uniformData.matrixVS.mapped, modelMatrices, uniformData.matrixVS.size);

		// not needed bc host coherent flag set
		//// Flush to make changes visible to the host 
		//VkMappedMemoryRange memoryRange = vkTools::initializers::mappedMemoryRange();
		//memoryRange.memory = uniformBuffers.dynamic.memory;
		//memoryRange.size = sizeof(uboDataDynamic);
		//vkFlushMappedMemoryRanges(device, 1, &memoryRange);
	}

	void updateMaterialBuffer() {
		if (materialNodes.size() != this->assetManager.loadedMaterials.size()) {
			if (this->assetManager.loadedMaterials.size() == 0) {
				vkx::MaterialProperties p;
				p.ambient = glm::vec4();
				p.diffuse = glm::vec4();
				p.specular = glm::vec4();
				p.opacity = 1.0f;
				materialNodes[0] = p;
			} else {
				materialNodes.resize(this->assetManager.loadedMaterials.size());
			}
		}

		for (int i = 0; i < this->assetManager.loadedMaterials.size(); ++i) {
			materialNodes[i] = this->assetManager.loadedMaterials[i].properties;
		}

		// todo: don't update the whole buffer each time
		// use map memory range and flush
		// uniform data must not set local host coherent bit?
		// makes changes visible to host
		uniformData.materialVS.copy(materialNodes);
	}



	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffersDeferred() {
		// Fullscreen vertex shader
		uniformDataDeferred.vsFullScreen = context.createUniformBuffer(uboVS);
		// Deferred vertex shader
		uniformDataDeferred.vsOffscreen = context.createUniformBuffer(uboOffscreenVS);
		// Deferred fragment shader
		uniformDataDeferred.fsLights = context.createUniformBuffer(uboFSLights);

		// Update
		updateUniformBuffersScreen();
		updateUniformBufferDeferredMatrices();
		updateUniformBufferDeferredLights();
	}

	void updateUniformBuffersScreen() {
		if (debugDisplay) {
			uboVS.projection = glm::ortho(0.0f, 2.0f, 0.0f, 2.0f, -1.0f, 1.0f);
		} else {
			uboVS.projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
		}
		uboVS.model = glm::mat4();
		//uboVS.model = camera.matrices.view;
		uniformDataDeferred.vsFullScreen.copy(uboVS);
	}

	void updateUniformBufferDeferredMatrices() {
		camera.updateViewMatrix();
		uboOffscreenVS.projection = camera.matrices.projection;
		uboOffscreenVS.view = camera.matrices.view;
		uniformDataDeferred.vsOffscreen.copy(uboOffscreenVS);
	}

	// Update fragment shader light position uniform block
	void updateUniformBufferDeferredLights() {
		// White light from above
		uboFSLights.lights[0].position = glm::vec4(0.0f, 4.0f*sin(globalP), 3.0f, 0.0f);
		uboFSLights.lights[0].color = glm::vec4(1.5f);
		uboFSLights.lights[0].radius = 15.0f;
		uboFSLights.lights[0].linearFalloff = 0.3f;
		uboFSLights.lights[0].quadraticFalloff = 0.4f;
		// Red light
		uboFSLights.lights[1].position = glm::vec4(2.0f*cos(globalP) - 2.0f, 0.0f, 0.0f, 0.0f);
		uboFSLights.lights[1].color = glm::vec4(1.5f, 0.0f, 0.0f, 0.0f);
		uboFSLights.lights[1].radius = 15.0f;
		uboFSLights.lights[1].linearFalloff = 0.4f;
		uboFSLights.lights[1].quadraticFalloff = 0.3f;
		// Blue light
		uboFSLights.lights[2].position = glm::vec4(2.0f, 1.0f, 0.0f, 0.0f);
		uboFSLights.lights[2].color = glm::vec4(0.0f, 0.0f, 2.5f, 0.0f);
		uboFSLights.lights[2].radius = 10.0f;
		uboFSLights.lights[2].linearFalloff = 0.45f;
		uboFSLights.lights[2].quadraticFalloff = 0.35f;
		// Belt glow
		uboFSLights.lights[3].position = glm::vec4(0.0f, 0.7f, 0.5f, 0.0f);
		uboFSLights.lights[3].color = glm::vec4(2.5f, 2.5f, 0.0f, 0.0f);
		uboFSLights.lights[3].radius = 5.0f;
		uboFSLights.lights[3].linearFalloff = 8.0f;
		uboFSLights.lights[3].quadraticFalloff = 6.0f;
		// Green light
		uboFSLights.lights[4].position = glm::vec4(3.0f, 2.0f, 1.0f, 0.0f);
		uboFSLights.lights[4].color = glm::vec4(0.0f, 1.5f, 0.0f, 0.0f);
		uboFSLights.lights[4].radius = 10.0f;
		uboFSLights.lights[4].linearFalloff = 0.8f;
		uboFSLights.lights[4].quadraticFalloff = 0.6f;

		// Current view position
		//uboFragmentLights.viewPos = glm::vec4(0.0f, 0.0f, -camera.transform.translation.z, 0.0f);
		//uboFragmentLights.viewPos = glm::vec4(-camera.transform.translation.x, -camera.transform.translation.y, -camera.transform.translation.z, 0.0f);
		uboFSLights.viewPos = glm::vec4(camera.transform.translation, 0.0f) * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);

		uniformDataDeferred.fsLights.copy(uboFSLights);
	}


	void toggleDebugDisplay() {
		debugDisplay = !debugDisplay;
		updateDrawCommandBuffers();
		buildOffscreenCommandBuffer();
		updateUniformBuffersScreen();
	}

	void start() {


		auto planeModel = std::make_shared<vkx::Model>(&context, &assetManager);
		planeModel->load(getAssetPath() + "models/plane.fbx");
		planeModel->createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//auto otherModel2 = std::make_shared<vkx::Model>(&context, &assetManager);
		//otherModel2->load(getAssetPath() + "models/monkey.fbx");
		//otherModel2->createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);



		//meshes.push_back(skyboxMesh);
		//meshes.push_back(planeMesh);
		//meshes.push_back(otherMesh1);

		models.push_back(planeModel);
		//models.push_back(otherModel1);

		for (int i = 0; i < 4; ++i) {

			auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
			testModel->load(getAssetPath() + "models/monkey.fbx");
			testModel->createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

			models.push_back(testModel);
		}



		// important!
		//http://stackoverflow.com/questions/6624819/c-vector-of-objects-vs-vector-of-pointers-to-objects

		auto skinnedMesh1 = std::make_shared<vkx::SkinnedMesh>(&context, &assetManager);
		skinnedMesh1->load(getAssetPath() + "models/goblin.dae");
		skinnedMesh1->setup(0.0005f);

		for (int i = 0; i < 1; ++i) {

			auto testSkinnedMesh = std::make_shared<vkx::SkinnedMesh>(&context, &assetManager);
			testSkinnedMesh->load(getAssetPath() + "models/goblin.dae");
			testSkinnedMesh->setup(0.0005f);

			//skinnedMeshes.push_back(testSkinnedMesh);
		}

		auto physicsPlane = std::make_shared<vkx::PhysicsObject>(&physicsManager, models[0]);




		auto physicsBall = std::make_shared<vkx::PhysicsObject>(&physicsManager, models[1]);

		//the ground is a cube of side 100 at position y = -56.
		//the sphere will hit it at y = -6, with center at -5
		{
			btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(20.), btScalar(20.), btScalar(0.1)));

			this->physicsManager.collisionShapes.push_back(groundShape);

			btTransform groundTransform;
			groundTransform.setIdentity();
			groundTransform.setOrigin(btVector3(0, 0, 0));

			btScalar mass(0.);

			//rigidbody is dynamic if and only if mass is non zero, otherwise static
			bool isDynamic = (mass != 0.f);

			btVector3 localInertia(0, 0, 0);
			if (isDynamic) {
				groundShape->calculateLocalInertia(mass, localInertia);
			}

			//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
			btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
			btRigidBody* body = new btRigidBody(rbInfo);

			physicsPlane->rigidBody = body;

			//add the body to the dynamics world
			this->physicsManager.dynamicsWorld->addRigidBody(body);
		}

		{
			//create a dynamic rigidbody

			//btCollisionShape* colShape = new btBoxShape(btVector3(1,1,1));
			btCollisionShape* colShape = new btSphereShape(btScalar(1.));
			this->physicsManager.collisionShapes.push_back(colShape);

			/// Create Dynamic Objects
			btTransform startTransform;
			startTransform.setIdentity();

			btScalar	mass(1.f);

			//rigidbody is dynamic if and only if mass is non zero, otherwise static
			bool isDynamic = (mass != 0.f);

			btVector3 localInertia(0, 0, 0);
			if (isDynamic) {
				colShape->calculateLocalInertia(mass, localInertia);

				startTransform.setOrigin(btVector3(0, 0, 10));

				//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
				btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
				btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
				btRigidBody* body = new btRigidBody(rbInfo);

				physicsBall->rigidBody = body;

				this->physicsManager.dynamicsWorld->addRigidBody(body);

			}
		}

		physicsObjects.push_back(physicsPlane);
		physicsObjects.push_back(physicsBall);




		// after any model loading with materials has occurred, updateMaterialBuffer() must be called
		// *after any material loading
		updateMaterialBuffer();

	}









	void updateWorld() {


		// z-up translations
		if (!keyStates.shift) {
			if (keyStates.w) {
				camera.translateLocal(glm::vec3(0.0f, camera.movementSpeed, 0.0f));
			}
			if (keyStates.s) {
				camera.translateLocal(glm::vec3(0.0f, -camera.movementSpeed, 0.0f));
			}
			if (keyStates.a) {
				camera.translateLocal(glm::vec3(-camera.movementSpeed, 0.0f, 0.0f));
			}
			if (keyStates.d) {
				camera.translateLocal(glm::vec3(camera.movementSpeed, 0.0f, 0.0f));
			}
			if (keyStates.q) {
				camera.translateLocal(glm::vec3(0.0f, 0.0f, -camera.movementSpeed));
			}
			if (keyStates.e) {
				camera.translateLocal(glm::vec3(0.0f, 0.0f, camera.movementSpeed));
			}
		} else {
			if (keyStates.w) {
				camera.translateWorld(glm::vec3(0.0f, camera.movementSpeed, 0.0f));
			}
			if (keyStates.s) {
				camera.translateWorld(glm::vec3(0.0f, -camera.movementSpeed, 0.0f));
			}
			if (keyStates.a) {
				camera.translateWorld(glm::vec3(-camera.movementSpeed, 0.0f, 0.0f));
			}
			if (keyStates.d) {
				camera.translateWorld(glm::vec3(camera.movementSpeed, 0.0f, 0.0f));
			}
			if (keyStates.q) {
				camera.translateWorld(glm::vec3(0.0f, 0.0f, -camera.movementSpeed));
			}
			if (keyStates.e) {
				camera.translateWorld(glm::vec3(0.0f, 0.0f, camera.movementSpeed));
			}
		}



		// z-up rotations
		camera.rotationSpeed = -0.005f;

		if (mouse.leftMouseButton.state) {
			camera.rotateWorldZ(mouse.delta.x*camera.rotationSpeed);
			camera.rotateLocalX(mouse.delta.y*camera.rotationSpeed);

			if (!camera.isFirstPerson) {
				camera.sphericalCoords.theta += mouse.delta.x*camera.rotationSpeed;
				camera.sphericalCoords.phi -= mouse.delta.y*camera.rotationSpeed;
			}


			SDL_SetRelativeMouseMode((SDL_bool)1);
		} else {
			bool isCursorLocked = (bool)SDL_GetRelativeMouseMode();
			if (isCursorLocked) {
				SDL_SetRelativeMouseMode((SDL_bool)0);
				SDL_WarpMouseInWindow(this->SDLWindow, mouse.leftMouseButton.pressedCoords.x, mouse.leftMouseButton.pressedCoords.y);
			}
		}


		camera.rotationSpeed = -0.02f;


		if (keyStates.up_arrow) {
			camera.rotateLocalX(camera.rotationSpeed);
		}
		if (keyStates.down_arrow) {
			camera.rotateLocalX(-camera.rotationSpeed);
		}

		if (!keyStates.shift) {
			if (keyStates.left_arrow) {
				camera.rotateWorldZ(-camera.rotationSpeed);
			}
			if (keyStates.right_arrow) {
				camera.rotateWorldZ(camera.rotationSpeed);
			}
		} else {
			//if (keyStates.left_arrow) {
			//	camera.rotateWorldY(-camera.rotationSpeed);
			//}
			//if (keyStates.right_arrow) {
			//	camera.rotateWorldY(camera.rotationSpeed);
			//}
		}

		if (keyStates.t) {
			camera.isFirstPerson = !camera.isFirstPerson;
		}

		if (keyStates.r) {
			toggleDebugDisplay();
		}

		//camera.updateViewMatrix();
		//// todo: definitely remove this from here
		//updateUniformBuffersScreen();
		//updateUniformBufferDeferredMatrices();
		//updateUniformBufferDeferredLights();

		if (!camera.isFirstPerson) {
			camera.followOpts.point = models[1]->transform.translation;
		}


		if (keyStates.i) {
			physicsObjects[1]->rigidBody->activate();
			physicsObjects[1]->rigidBody->applyCentralForce(btVector3(0.0f, 0.1f, 0.0f));
		}
		if (keyStates.k) {
			physicsObjects[1]->rigidBody->activate();
			physicsObjects[1]->rigidBody->applyCentralForce(btVector3(0.0f, -0.1f, 0.0f));
		}
		if (keyStates.j) {
			physicsObjects[1]->rigidBody->activate();
			physicsObjects[1]->rigidBody->applyCentralForce(btVector3(-0.1f, 0.0f, 0.0f));
		}
		if (keyStates.l) {
			physicsObjects[1]->rigidBody->activate();
			physicsObjects[1]->rigidBody->applyCentralForce(btVector3(0.1f, 0.0f, 0.0f));
		}

	}






	void updatePhysics() {
		//this->physicsManager.dynamicsWorld->stepSimulation(1.f / 60.f, 10);
		this->physicsManager.dynamicsWorld->stepSimulation(1.f / (frameTimer*1000.0f), 10);

		for (int i = 0; i < this->physicsObjects.size(); ++i) {
			this->physicsObjects[i]->sync();
		}
	}














	void updateDrawCommandBuffer(const vk::CommandBuffer &cmdBuffer) {
		cmdBuffer.setViewport(0, vkx::viewport(size));
		cmdBuffer.setScissor(0, vkx::rect2D(size));


		//https://github.com/nvpro-samples/gl_vk_threaded_cadscene/blob/master/doc/vulkan_uniforms.md


		// left:
		//vk::Viewport testViewport = vkx::viewport((float)size.width / 3, (float)size.height, 0.0f, 1.0f);
		//cmdBuffer.setViewport(0, testViewport);
		// center
		//viewport.x += viewport.width;
		//cmdBuffer.setViewport(0, viewport);



		// todo: fix this// important
		// stop doing this every frame
		// only when necessary
		// actually not that bad, since it makes pushing to vector easy
		for (int i = 0; i < models.size(); ++i) {
			models[i]->matrixIndex = i;
		}


		// uses matrix indices directly after meshes' indices
		for (int i = 0; i < skinnedMeshes.size(); ++i) {
			skinnedMeshes[i]->matrixIndex = models.size() + i;
		}

		for (int i = 0; i < skinnedMeshes.size(); ++i) {
			skinnedMeshes[i]->boneIndex = i;
			// todo: set 4x4 matrix bone index
		}

		//for (int i = 0; i < matrixNodes.size(); ++i) {
		//	matrixNodes[i].boneIndex[0][0] = i;
		//}


		globalP += 0.005f;



		if (models.size() > 3) {

			//models[1]->setTranslation(glm::vec3(3 * cos(globalP), 1.0f, 3 * sin(globalP)));
			models[2]->setTranslation(glm::vec3(2 * cos(globalP) + 2.0f, sin(globalP) + 2.0, 0.0f));
			models[3]->setTranslation(glm::vec3(cos(globalP) - 2.0f, 2.0f, sin(globalP) + 2.0f));
			//models[4].setTranslation(glm::vec3(cos(globalP), 3.0f, 0.0f));
			//models[5].setTranslation(glm::vec3(cos(globalP) - 2.0f, 4.0f, 0.0f));
		}

		//if (keyStates.space) {
		//	auto testModel = std::make_shared<vkx::Model>(&context, &assetManager);
		//	testModel->load(getAssetPath() + "models/monkey.fbx");
		//	testModel->createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
		//	testModel->matrixIndex = 0;

		//	models.push_back(testModel);
		//}

		if (models.size() > 6) {
			models[4]->setTranslation(glm::vec3(cos(globalP), 3.0f, 0.0f));
		}




		if (skinnedMeshes.size() > 1) {
			skinnedMeshes[0]->setTranslation(glm::vec3(2.0f, sin(globalP), 1.0f));

			glm::vec3 point = skinnedMeshes[1]->transform.translation;
			skinnedMeshes[1]->setTranslation(glm::vec3(point.x, point.y, 1.0f));
			skinnedMeshes[1]->translateLocal(glm::vec3(0.0f, 0.05f, 0.0f));
			skinnedMeshes[1]->rotateLocalZ(0.014f);
		}



		if (skinnedMeshes.size() > 1) {
			skinnedMeshes[0]->animationSpeed = 1.0f;
			skinnedMeshes[1]->animationSpeed = 3.5f;
		}


		//uboScene.lightPos = glm::vec3(cos(globalP), 4.0f, cos(globalP));
		uboScene.lightPos = glm::vec4(cos(globalP), 4.0f, cos(globalP), 1.0f);
		//uboScene.lightPos = glm::vec3(1.0f, -2.0f, 4.0/**sin(globalP*10.0f)*/+4.0);


		//matrixNodes[0].model = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
		//matrixNodes[1].model = glm::translate(glm::mat4(), glm::vec3(sin(globalP), 1.0f, 0.0f));


		if (keyStates.space) {
			physicsObjects[1]->rigidBody->activate();
			//physicsObjects[1]->rigidBody->setLinearVelocity(btVector3(0.0f, 0.0f, 12.0f));
			physicsObjects[1]->rigidBody->applyCentralForce(btVector3(0.0f, sin(globalP)*0.1f, 0.05f));
		}



		// todo: fix
		for (auto &model : models) {

			//consoleLog = std::to_string(model.transform.translation.x);
			matrixNodes[model->matrixIndex].model = model->transfMatrix;
			//glm::mat4* modelMat = (glm::mat4*)(((uint64_t)modelMatrices + (model.matrixIndex * dynamicAlignment)));
			//*modelMat = model.transfMatrix;

		}




		// uboBoneData.bones is a large bone data buffer
		// use offset to store bone data for each skinnedMesh
		// basically a manual dynamic buffer
		for (auto &skinnedMesh : skinnedMeshes) {

			matrixNodes[skinnedMesh->matrixIndex].model = skinnedMesh->transfMatrix;
			matrixNodes[skinnedMesh->matrixIndex].boneIndex[0][0] = skinnedMesh->boneIndex;

			// performance optimization needed here:
			skinnedMesh->update(globalP*skinnedMesh->animationSpeed);// update animation / interpolated


			uint32_t boneOffset = skinnedMesh->boneIndex*MAX_BONES;

			for (uint32_t i = 0; i < skinnedMesh->boneTransforms.size(); ++i) {

				//matrixNodes[skinnedMesh.matrixIndex].bones[i] = aiMatrix4x4ToGlm(&skinnedMesh.boneTransforms[i]);
				//matrixNodes[skinnedMesh.matrixIndex].bones[i] = glm::transpose(glm::make_mat4(&skinnedMesh.boneTransforms[i].a1));
				//std::ofstream log("logfile.txt", std::ios_base::app | std::ios_base::out);
				//log << skinnedMesh.boneTransforms[i].a1 << "\n";

				//uboBoneData.bones[boneOffset + i] = glm::transpose(glm::make_mat4(&skinnedMesh->boneTransforms[i].a1));
				//uboScene.bones[i] = glm::transpose(glm::make_mat4(&skinnedMesh->boneTransforms[i].a1));

				uboScene.bones[boneOffset + i] = glm::transpose(glm::make_mat4(&skinnedMesh->boneTransforms[i].a1));

			}
		}


		updateSceneBuffer();
		updateMatrixBuffer();
		updateMaterialBuffer();

		//updateTextOverlay();

		//uniformData.bonesVS.copy(uboBoneData);



		uint32_t lastMaterialIndex = -1;



		//// for each of the model's meshes
		//for (auto &mesh : meshes) {

		//	// bind vertex & index buffers
		//	cmdBuffer.bindVertexBuffers(mesh->vertexBufferBinding, mesh->meshBuffer.vertices.buffer, vk::DeviceSize());
		//	cmdBuffer.bindIndexBuffer(mesh->meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

		//	uint32_t setNum;

		//	// bind scene descriptor set
		//	setNum = 0;
		//	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, descriptorSets[setNum], nullptr);


		//	//uint32_t offset1 = mesh.matrixIndex * alignedMatrixSize;
		//	uint32_t offset1 = mesh->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
		//	//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
		//	// the third param is the set number!
		//	setNum = 1;
		//	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset1);


		//	if (lastMaterialIndex != mesh->meshBuffer.materialIndex) {
		//		lastMaterialIndex = mesh->meshBuffer.materialIndex;
		//		uint32_t offset2 = mesh->meshBuffer.materialIndex * static_cast<uint32_t>(alignedMaterialSize);
		//		// the third param is the set number!
		//		setNum = 2;
		//		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset2);

		//		// must make pipeline layout compatible
		//		setNum = 3;
		//		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, m.descriptorSet, nullptr);
		//	}

		//	// draw:
		//	cmdBuffer.drawIndexed(mesh->meshBuffer.indexCount, 1, 0, 0, 0);
		//}



		//https://github.com/SaschaWillems/Vulkan/tree/master/dynamicuniformbuffer




		// MODELS:

		// bind mesh pipeline
		// don't have to do this for every mesh
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.meshes);

		// for each model
		// model = group of meshes
		// todo: add skinned / animated model support
		for (auto &model : models) {
			// for each of the model's meshes
			for (auto &mesh : model->meshes) {


				// bind vertex & index buffers
				cmdBuffer.bindVertexBuffers(mesh.vertexBufferBinding, mesh.meshBuffer.vertices.buffer, vk::DeviceSize());
				cmdBuffer.bindIndexBuffer(mesh.meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

				// descriptor set #
				uint32_t setNum;

				// bind scene descriptor set
				setNum = 0;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, descriptorSets[setNum], nullptr);


				//uint32_t offset1 = mesh.matrixIndex * alignedMatrixSize;
				uint32_t offset1 = model->matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
				//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
				// the third param is the set number!
				setNum = 1;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset1);


				if (lastMaterialIndex != mesh.meshBuffer.materialIndex) {
					lastMaterialIndex = mesh.meshBuffer.materialIndex;
					uint32_t offset2 = mesh.meshBuffer.materialIndex * static_cast<uint32_t>(alignedMaterialSize);
					// the third param is the set number!
					setNum = 2;
					cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset2);
					//}

					lastMaterialIndex = mesh.meshBuffer.materialIndex;

					// must make pipeline layout compatible
					vkx::Material m = this->assetManager.loadedMaterials[mesh.meshBuffer.materialIndex];
					setNum = 3;
					cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, m.descriptorSet, nullptr);
				}


				// draw:
				cmdBuffer.drawIndexed(mesh.meshBuffer.indexCount, 1, 0, 0, 0);
			}

		}














		// SKINNED MESHES:

		// bind skinned mesh pipeline
		// don't have to do this for every skinned mesh// bind once
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skinnedMeshes);

		for (auto &skinnedMesh : skinnedMeshes) {


			// bind vertex & index buffers
			cmdBuffer.bindVertexBuffers(skinnedMesh->vertexBufferBinding, skinnedMesh->meshBuffer.vertices.buffer, vk::DeviceSize());
			cmdBuffer.bindIndexBuffer(skinnedMesh->meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);


			uint32_t setNum;
			//uint32_t offset;

			// bind scene descriptor set
			setNum = 0;
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, descriptorSets[setNum], nullptr);


			uint32_t offset1 = skinnedMesh->matrixIndex * alignedMatrixSize;
			//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
			// the third param is the set number!
			setNum = 1;
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset1);


			if (lastMaterialIndex != skinnedMesh->meshBuffer.materialIndex) {

				lastMaterialIndex = skinnedMesh->meshBuffer.materialIndex;
				uint32_t offset2 = skinnedMesh->meshBuffer.materialIndex * alignedMaterialSize;
				// the third param is the set number!
				setNum = 2;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset2);
				//}




				//if (lastMaterialIndex != skinnedMesh->meshBuffer.materialIndex) {
				// must make pipeline layout compatible
				vkx::Material m = this->assetManager.loadedMaterials[skinnedMesh->meshBuffer.materialIndex];
				setNum = 3;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, m.descriptorSet, nullptr);
			}


			// draw:
			cmdBuffer.drawIndexed(skinnedMesh->meshBuffer.indexCount, 1, 0, 0, 0);
		}








		//updateUniformBufferDeferredLights();




		//vk::Viewport viewport = vkx::viewport(size);


		//cmdBuffer.setViewport(0, viewport);
		//cmdBuffer.setScissor(0, vkx::rect2D(size));


		//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.deferred, 0, descriptorSetsDeferred.basic, nullptr);
		//if (debugDisplay) {
		//	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.debug);
		//	cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshBuffers.quad.vertices.buffer, { 0 });
		//	cmdBuffer.bindIndexBuffer(meshBuffers.quad.indices.buffer, 0, vk::IndexType::eUint32);
		//	cmdBuffer.drawIndexed(meshBuffers.quad.indexCount, 1, 0, 0, 1);
		//	// Move viewport to display final composition in lower right corner
		//	viewport.x = viewport.width * 0.5f;
		//	viewport.y = viewport.height * 0.5f;
		//}

		//viewport.x = viewport.width * 0.5f;
		//viewport.y = viewport.height * 0.5f;

		//cmdBuffer.setViewport(0, viewport);
		//// Final composition as full screen quad
		//cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.deferred);
		//cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshBuffers.quad.vertices.buffer, { 0 });
		//cmdBuffer.bindIndexBuffer(meshBuffers.quad.indices.buffer, 0, vk::IndexType::eUint32);
		//cmdBuffer.drawIndexed(6, 1, 0, 0, 1);


	}




	// Build command buffer for rendering the scene to the offscreen frame buffer 
	// and blitting it to the different texture targets
	void buildOffscreenCommandBuffer() override {
		//// Create separate command buffer for offscreen 
		//// rendering
		//if (!offscreenCmdBuffer) {
		//	vk::CommandBufferAllocateInfo cmd = vkx::commandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1);
		//	offscreenCmdBuffer = device.allocateCommandBuffers(cmd)[0];
		//}

		//vk::CommandBufferBeginInfo cmdBufInfo;
		//cmdBufInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

		//// Clear values for all attachments written in the fragment sahder
		//std::array<vk::ClearValue, 4> clearValues;
		//clearValues[0].color = vkx::clearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
		//clearValues[1].color = vkx::clearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
		//clearValues[2].color = vkx::clearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
		//clearValues[3].depthStencil = { 1.0f, 0 };

		//vk::RenderPassBeginInfo renderPassBeginInfo;
		//renderPassBeginInfo.renderPass = offscreen.renderPass;
		//renderPassBeginInfo.framebuffer = offscreen.framebuffers[0].framebuffer;
		//renderPassBeginInfo.renderArea.extent.width = offscreen.size.x;
		//renderPassBeginInfo.renderArea.extent.height = offscreen.size.y;
		//renderPassBeginInfo.clearValueCount = clearValues.size();
		//renderPassBeginInfo.pClearValues = clearValues.data();

		//offscreenCmdBuffer.begin(cmdBufInfo);
		//offscreenCmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

		//vk::Viewport viewport = vkx::viewport(offscreen.size);
		//offscreenCmdBuffer.setViewport(0, viewport);

		//vk::Rect2D scissor = vkx::rect2D(offscreen.size);
		//offscreenCmdBuffer.setScissor(0, scissor);

		//offscreenCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.offscreen, 0, descriptorSetsDeferred.offscreen, nullptr);
		//offscreenCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.offscreen);

		//vk::DeviceSize offsets = { 0 };
		//offscreenCmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshBuffers.example.vertices.buffer, { 0 });
		//offscreenCmdBuffer.bindIndexBuffer(meshBuffers.example.indices.buffer, 0, vk::IndexType::eUint32);
		//offscreenCmdBuffer.drawIndexed(meshBuffers.example.indexCount, 1, 0, 0, 0);
		//offscreenCmdBuffer.endRenderPass();
		//offscreenCmdBuffer.end();
	}














	void viewChanged() override {
		//updateUniformBufferDeferredMatrices();
	}






	void loadTextures() {
		textures.colorMap = textureLoader->loadTexture(
			getAssetPath() + "models/armor/colormap.ktx",
			vk::Format::eBc3UnormBlock);
	}

	void loadMeshes() {
		//meshes.example = loadMesh(getAssetPath() + "models/armor/armor.dae", vertexLayout, 1.0f);

		vkx::MeshLoader loader(&this->context, &this->assetManager);
		loader.load(getAssetPath() + "models/armor/armor.dae");
		loader.createMeshBuffer(meshVertexLayout, 1.0f);
		meshBuffers.example = loader.combinedBuffer;
	}

	void generateQuads() {
		// Setup vertices for multiple screen aligned quads
		// Used for displaying final result and debug 
		struct Vertex {
			float pos[3];
			float uv[2];
			float col[3];
			float normal[3];
		};

		std::vector<Vertex> vertexBuffer;

		float x = 0.0f;
		float y = 0.0f;
		for (uint32_t i = 0; i < 3; i++) {
			// Last component of normal is used for debug display sampler index
			vertexBuffer.push_back({ { x + 1.0f, y + 1.0f, 0.0f },{ 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, (float)i } });
			vertexBuffer.push_back({ { x,      y + 1.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, (float)i } });
			vertexBuffer.push_back({ { x,      y,      0.0f },{ 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, (float)i } });
			vertexBuffer.push_back({ { x + 1.0f, y,      0.0f },{ 1.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, (float)i } });
			x += 1.0f;
			if (x > 1.0f) {
				x = 0.0f;
				y += 1.0f;
			}
		}
		meshBuffers.quad.vertices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);

		// Setup indices
		std::vector<uint32_t> indexBuffer = { 0,1,2, 2,3,0 };
		for (uint32_t i = 0; i < 3; ++i) {
			uint32_t indices[6] = { 0,1,2, 2,3,0 };
			for (auto index : indices) {
				indexBuffer.push_back(i * 4 + index);
			}
		}
		meshBuffers.quad.indexCount = indexBuffer.size();
		meshBuffers.quad.indices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
	}












	void prepare() override {


		offscreen.size = glm::uvec2(TEX_DIM);
		offscreen.colorFormats = std::vector<vk::Format>{ {
				vk::Format::eR16G16B16A16Sfloat,
				vk::Format::eR16G16B16A16Sfloat,
				vk::Format::eR8G8B8A8Unorm
			} };

		//vulkanApp::prepare();
		//offscreen.prepare();
		OffscreenExampleBase::prepare();



		loadTextures();
		generateQuads();
		loadMeshes();


		prepareVertexDescriptions();



		prepareUniformBuffers();
		//prepareUniformBuffersDeferred();

		setupDescriptorSetLayout();
		setupDescriptorPool();
		setupDescriptorSet();

		preparePipelines();
		//prepareDeferredPipelines();

		start();


		updateDrawCommandBuffers();
		//buildOffscreenCommandBuffer();



		prepared = true;
	}

	void draw() override {
		prepareFrame();
		{
			vk::SubmitInfo submitInfo;
			submitInfo.pWaitDstStageMask = this->submitInfo.pWaitDstStageMask;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &offscreenCmdBuffer;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &semaphores.acquireComplete;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &offscreen.renderComplete;
			queue.submit(submitInfo, VK_NULL_HANDLE);
		}
		drawCurrentCommandBuffer(offscreen.renderComplete);
		submitFrame();
	}

	virtual void render() {
		if (!prepared) {
			return;
		}
		draw();
	}

	//virtual void viewChanged() {
	//	updateTextOverlay(); // todo: remove this
	//	updateUniformBuffers();
	//}

	virtual void getOverlayText(vkx::TextOverlay *textOverlay) {


		textOverlay->addText(title, 5.0f, 5.0f, vkx::TextOverlay::alignLeft);

		std::stringstream ss;
		//ss << std::fixed << std::setprecision(2) << (frameTimer * 1000.0f) << "ms (" << lastFPS << " fps)";
		ss << lastFPS << " FPS";
		textOverlay->addText(ss.str(), 5.0f, 25.0f, vkx::TextOverlay::alignLeft);

		ss.str("");
		ss.clear();

		ss << std::fixed << std::setprecision(2) << (frameTimer * 1000.0f) << "ms";
		textOverlay->addText(ss.str(), 5.0f, 45.0f, vkx::TextOverlay::alignLeft);


		//ss.str("");
		//ss.clear();

		//ss << "Frame #: " << frameCounter;
		//textOverlay->addText(ss.str(), 5.0f, 65.0f, vkx::TextOverlay::alignLeft);

		ss.str("");
		ss.clear();

		ss << "GPU: ";
		ss << deviceProperties.deviceName;
		textOverlay->addText(ss.str(), 5.0f, 65.0f, vkx::TextOverlay::alignLeft);


		// vertical offset
		float vOffset = 120.0f;
		float spacing = 20.0f;

		textOverlay->addText("pos x: " + std::to_string(camera.transform.translation.x), 5.0f, vOffset + 100.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("pos y: " + std::to_string(camera.transform.translation.y), 5.0f, vOffset + 120.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("pos z: " + std::to_string(camera.transform.translation.z), 5.0f, vOffset + 140.0f, vkx::TextOverlay::alignLeft);
	}

};




int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {

	VulkanExample* example = new VulkanExample();
	example->run();
	delete(example);
	return 0;
}