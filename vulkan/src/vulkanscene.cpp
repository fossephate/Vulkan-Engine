﻿/*
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

std::vector<vkx::VertexLayout> meshVertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4
};


std::vector<vkx::VertexLayout> skinnedMeshVertexLayout =
{
	vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
	vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
	vkx::VertexLayout::VERTEX_LAYOUT_UV,
	vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4,
	vkx::VertexLayout::VERTEX_LAYOUT_DUMMY_VEC4
};

// Maximum number of bones per mesh
// Must not be higher than same const in skinning shader
#define MAX_BONES 64
// Maximum number of bones per vertex
#define MAX_BONES_PER_VERTEX 4




inline size_t alignedSize(size_t align, size_t sz) {
	return ((sz + align - 1) / align)*align;
}

























inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from)
{
	glm::mat4 to;


	to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
	to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
	to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
	to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;

	return to;
}
















// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void* alignedAlloc(size_t size, size_t alignment)
{
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

void alignedFree(void* data)
{
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else 
	free(data);
#endif
}





class VulkanExample : public vkx::vulkanApp {

public:


	std::vector<vkx::Mesh> meshes;
	std::vector<vkx::Model> models;
	std::vector<vkx::SkinnedMesh> skinnedMeshes;


	struct {
		vkx::CreateBufferResult sceneVS;// scene data
		vkx::CreateBufferResult matrixVS;// matrix data
		vkx::CreateBufferResult materialVS;// material data
		vkx::CreateBufferResult bonesVS;// bone data for all skinned meshes // max of 1000 skinned meshes w/64 bones/mesh
	} uniformData;


	// static scene uniform buffer
	struct {

		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 normal;// why is this here?
		glm::vec3 lightPos;
		glm::vec3 cameraPos;

		glm::mat4 bones[MAX_BONES];

	} uboScene;


	struct matrixNode {
		glm::mat4 model;
		//int boneIndex;
		//glm::mat4 bones[MAX_BONES];
		//glm::mat4 g2;
		//glm::mat4 g3;
	};

	std::vector<matrixNode> matrixNodes;

	//matrixNode *mNodes = nullptr;

	//glm::mat4 *modelMatrices = nullptr;
	matrixNode *modelMatrices = nullptr;

	// rename to materialPropertiesNode// or something
	// possibly move
	// or just don't use
	//struct materialNode {
	//	glm::vec4 ambient;
	//	glm::vec4 diffuse;
	//	glm::vec4 specular;
	//	float opacity;
	//};

	std::vector<vkx::materialProperties> materialNodes;


	// bone data uniform buffer
	struct {
		glm::mat4 bones[MAX_BONES*64];
	} uboBoneData;
	


	unsigned int alignedMatrixSize;
	unsigned int alignedMaterialSize;

	size_t dynamicAlignment;

	float globalP = 0.0f;

	glm::vec3 lightPos = glm::vec3(1.0f, -2.0f, 2.0f);

	std::string consoleLog;



	// todo: remove this:
	struct {
		vkx::Texture colorMap;
		vkx::Texture floor;
	} textures;

	struct {
		vk::Pipeline meshes;
		vk::Pipeline skinnedMeshes;
		vk::Pipeline blending;// todo: make seperate for models and meshes, just meshes for now
		vk::Pipeline wireframe;
	} pipelines;



	struct {
		vk::PipelineLayout basic;
		vk::PipelineLayout meshes;
		vk::PipelineLayout skinnedMeshes;
	} pipelineLayouts;



	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
	std::vector<vk::DescriptorSet> descriptorSets;
	std::vector<vk::DescriptorPool> descriptorPools;

	struct {
		vk::PipelineVertexInputStateCreateInfo inputState;
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	} vertices;






	VulkanExample() : vkx::vulkanApp(ENABLE_VALIDATION) {
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
		//dynamicAlignment = (sizeof(matrixNode) / uboAlignment) * uboAlignment + ((sizeof(matrixNode) % uboAlignment) > 0 ? uboAlignment : 0);


		// todo: fix
		//size_t bufferSize = 100 * dynamicAlignment;
		//matrixNode2.model = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);
		//modelMatrices = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);
		//modelMatrices = (matrixNode*)alignedAlloc(bufferSize, dynamicAlignment);


		alignedMatrixSize = (unsigned int)(alignedSize(alignment, sizeof(matrixNode)));
		alignedMaterialSize = (unsigned int)(alignedSize(alignment, sizeof(vkx::materialProperties)));

		//camera.matrixNodes.projection = glm::perspectiveRH(glm::radians(60.0f), (float)size.width / (float)size.height, 0.0001f, 256.0f);

		title = "Vulkan Demo Scene";
	}

	~VulkanExample() {
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class


		// todo: fix this all up

		// destroy pipelines
		device.destroyPipeline(pipelines.meshes);
		device.destroyPipeline(pipelines.skinnedMeshes);

		device.destroyPipelineLayout(pipelineLayouts.basic);
		//device.destroyDescriptorSetLayout(descriptorSetLayout);

		uniformData.sceneVS.destroy();

		for (auto &mesh : meshes) {
			device.destroyBuffer(mesh.meshBuffer.vertices.buffer);
			device.freeMemory(mesh.meshBuffer.vertices.memory);

			device.destroyBuffer(mesh.meshBuffer.indices.buffer);
			device.freeMemory(mesh.meshBuffer.indices.memory);
		}


		//textures.colorMap.destroy();

		//uniformData.vsScene.destroy();

		//// Destroy and free mesh resources 
		//skinnedMesh->meshBuffer.destroy();
		//delete(skinnedMesh->meshLoader);
		//delete(skinnedMesh);
		//textures.skybox.destroy();

	}


	// todo: remove this:
	void loadTextures() {
		//textures.colorMap = textureLoader->loadCubemap(getAssetPath() + "textures/cubemap_vulkan.ktx", vk::Format::eR8G8B8A8Unorm);
		//textures.skybox = textureLoader->loadCubemap(getAssetPath() + "textures/cubemap_vulkan.ktx", vk::Format::eR8G8B8A8Unorm);
	}

	void updateDrawCommandBuffer(const vk::CommandBuffer &cmdBuffer) {
		cmdBuffer.setViewport(0, vkx::viewport(size));
		cmdBuffer.setScissor(0, vkx::rect2D(size));


		//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
		//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[0], nullptr);

		//https://github.com/nvpro-samples/gl_vk_threaded_cadscene/blob/master/doc/vulkan_uniforms.md


		// left:
		//vk::Viewport viewport = vkx::viewport((float)size.width / 3, (float)size.height, 0.0f, 1.0f);
		//cmdBuffer.setViewport(0, viewport);
		// center
		//viewport.x += viewport.width;
		//cmdBuffer.setViewport(0, viewport);



		// todo: fix this// important
		for (int i = 0; i < models.size(); ++i) {
			models[i].matrixIndex = i;
		}
		
		// todo: fix this
		skinnedMeshes[0].matrixIndex = 15;

		for (int i = 0; i < skinnedMeshes.size(); ++i) {
			skinnedMeshes[i].boneIndex = i;
		}
		

		globalP += 0.005f;


		//models[0].setTranslation(glm::vec3(3 * cos(globalP), 1.0f, 3*sin(globalP)));

		if (models.size() > 3) {

			models[1].setTranslation(glm::vec3(3 * cos(globalP), 1.0f, 3 * sin(globalP)));
			models[2].setTranslation(glm::vec3(2 * cos(globalP) + 2.0f, sin(globalP)+2.0, 0.0f));
			models[3].setTranslation(glm::vec3(cos(globalP) - 2.0f, 2.0f, sin(globalP)+2.0f));
			//models[4].setTranslation(glm::vec3(cos(globalP), 3.0f, 0.0f));
			//models[5].setTranslation(glm::vec3(cos(globalP) - 2.0f, 4.0f, 0.0f));
		}


		//models[1].setTranslation(glm::vec3(0, 0, 2));
		//glm::quat q = glm::angleAxis(3.14159f/2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		//models[1].setRotation(q);




		skinnedMeshes[0].setTranslation(glm::vec3(4.0f, sin(globalP) + 2.0, 0.0f));


		uboScene.lightPos = glm::vec3(cos(globalP), 4.0f, cos(globalP));
		//uboScene.lightPos = glm::vec3(1.0f, -2.0f, 4.0/**sin(globalP*10.0f)*/+4.0);


		//matrixNodes[0].model = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
		//matrixNodes[1].model = glm::translate(glm::mat4(), glm::vec3(sin(globalP), 1.0f, 0.0f));





		// todo: fix
		for (auto &model : models) {
			
			//consoleLog = std::to_string(model.transform.translation.x);

			matrixNodes[model.matrixIndex].model = model.transfMatrix;

			//glm::mat4* modelMat = (glm::mat4*)(((uint64_t)modelMatrices + (model.matrixIndex * dynamicAlignment)));
			//*modelMat = model.transfMatrix;

		}



		for (auto &skinnedMesh : skinnedMeshes) {
			matrixNodes[skinnedMesh.matrixIndex].model = skinnedMesh.transfMatrix;

			//skinnedMesh.update(globalP*0.0000000f);// slow down animation
			//skinnedMesh.update(globalP*0.0f);// slow down animation
			if (keyStates.space) {
				//skinnedMesh.update(globalP*0.05f);
				//std::ofstream log("logfile.txt", std::ios_base::app | std::ios_base::out);
				//log << &skinnedMesh.boneTransforms[i].a1 << "\n";
			}
			for (uint32_t i = 0; i < skinnedMesh.boneTransforms.size(); ++i) {
				//matrixNodes[skinnedMesh.matrixIndex].bones[i] = aiMatrix4x4ToGlm(&skinnedMesh.boneTransforms[i]);
				//matrixNodes[skinnedMesh.matrixIndex].bones[i] = glm::transpose(glm::make_mat4(&skinnedMesh.boneTransforms[i].a1));
				//std::ofstream log("logfile.txt", std::ios_base::app | std::ios_base::out);
				//log << skinnedMesh.boneTransforms[i].a1 << "\n";
			}
		}


		// uboBoneData.bones is a large bone data buffer
		// use offset to store bone data for each skinnedMesh
		// basically a manual dynamic buffer
		for (auto &skinnedMesh : skinnedMeshes) {

			matrixNodes[skinnedMesh.matrixIndex].model = skinnedMesh.transfMatrix;


			uint32_t boneOffset = skinnedMesh.boneIndex*64;;

			//skinnedMesh.update(globalP*0.0f);// slow down animation
			//skinnedMesh.update(globalP*0.5f);// slow down animation
			if (keyStates.space) {
				//skinnedMesh.update(globalP*0.05f);
				//std::ofstream log("logfile.txt", std::ios_base::app | std::ios_base::out);
				//log << &skinnedMesh.boneTransforms[i].a1 << "\n";
			}

			for (uint32_t i = 0; i < skinnedMesh.boneTransforms.size(); ++i) {
				
				//matrixNodes[skinnedMesh.matrixIndex].bones[i] = aiMatrix4x4ToGlm(&skinnedMesh.boneTransforms[i]);
				//matrixNodes[skinnedMesh.matrixIndex].bones[i] = glm::transpose(glm::make_mat4(&skinnedMesh.boneTransforms[i].a1));
				//std::ofstream log("logfile.txt", std::ios_base::app | std::ios_base::out);
				//log << skinnedMesh.boneTransforms[i].a1 << "\n";

				//uboBoneData.bones[boneOffset + i] = glm::transpose(glm::make_mat4(&skinnedMesh.boneTransforms[i].a1));
				//uboScene.bones[i] = glm::transpose(glm::make_mat4(&skinnedMesh.boneTransforms[i].a1));
				//uboScene.bones[i] = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
			}
		}

		
		//for (uint32_t i = 0; i < skinnedMeshes[0].boneTransforms.size(); i++) {
		//	uboVS.bones[i] = glm::transpose(glm::make_mat4(&skinnedMeshes[0].boneTransforms[i].a1));
		//}


		// what?
		//glm::mat4 test = glm::translate(glm::mat4(), glm::vec3(0.01*sin(globalP) - 2.0f, 0.01*cos(globalP), 0.0f));
		//matrixNodes[0].model = test;

		//glm::mat4 test = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
		//matrixNodes[0].model = test;


		
		updateMatrixBuffer();
		updateMaterialBuffer();
		updateUniformBuffers();


		//uniformData.bonesVS.copy(uboBoneData);
		






		//for (auto &mesh : meshes) {

		//	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mesh.pipeline);
		//	cmdBuffer.bindVertexBuffers(mesh.vertexBufferBinding, mesh.meshBuffer.vertices.buffer, vk::DeviceSize());
		//	cmdBuffer.bindIndexBuffer(mesh.meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

		//	// move this outside loop
		//	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[0], nullptr);

		//	uint32_t offset = mesh.matrixIndex * alignedMatrixSize;
		//	//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
		//	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, 1, &descriptorSets[1], 1, &offset);

		//	//uint32_t offset2 = mesh.matrixIndex * alignedMaterialSize;
		//	//uint32_t offset2 = mesh.materialIndex * alignedMatrixSize;// change?
		//	//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, 1, &descriptorSets[2], 1, &offset2);

		//	cmdBuffer.drawIndexed(mesh.meshBuffer.indexCount, 1, 0, 0, 0);
		//}



		//https://github.com/SaschaWillems/Vulkan/tree/master/dynamicuniformbuffer

		uint32_t lastMaterialIndex = -1;


		// MODELS:

		// bind mesh pipeline
		// don't have to do this for every mesh
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.meshes);

		// for each model
		// model = group of meshes
		// todo: add skinned / animated model support
		for (auto &model : models) {
			// for each of the model's meshes
			for (auto &mesh : model.meshes) {


				// bind vertex & index buffers
				cmdBuffer.bindVertexBuffers(mesh.vertexBufferBinding, mesh.meshBuffer.vertices.buffer, vk::DeviceSize());
				cmdBuffer.bindIndexBuffer(mesh.meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);

				uint32_t setNum;

				// bind scene descriptor set
				setNum = 0;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, descriptorSets[setNum], nullptr);


				//uint32_t offset1 = mesh.matrixIndex * alignedMatrixSize;
				uint32_t offset1 = model.matrixIndex * static_cast<uint32_t>(alignedMatrixSize);
				//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
				// the third param is the set number!
				setNum = 1;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset1);


				// todo:
				// avoid re binding the same material twice in a row
				// ie: if(lastMaterialIndex != mesh.meshBuffer.materialIndex)

				// get offset of mesh's material using meshbuffer's material index
				// and aligned material size

				//if (lastMaterialIndex != mesh.meshBuffer.materialIndex) {
					lastMaterialIndex = mesh.meshBuffer.materialIndex;
					uint32_t offset2 = mesh.meshBuffer.materialIndex * static_cast<uint32_t>(alignedMaterialSize);
					// the third param is the set number!
					setNum = 2;
					cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset2);
				//}







				////if (lastMaterialIndex != mesh.meshBuffer.materialIndex) {
				//lastMaterialIndex = mesh.meshBuffer.materialIndex;
				////uint32_t offset2 = mesh.meshBuffer.materialIndex * alignedMaterialSize;
				//uint32_t offset2 = 0 * alignedMaterialSize;
				//// the third param is the set number!
				//setNum = 2;
				//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, setNum, 1, &descriptorSets[setNum], 1, &offset2);

				
				//uint32_t offset3 = 1000000 * static_cast<uint32_t>(alignedMatrixSize);

				// must make pipeline layout compatible
				setNum = 3;
				vkx::Material m = this->assetManager.loadedMaterials[mesh.meshBuffer.materialIndex];
				//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &m.descriptorSet, 1, &offset3);
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, m.descriptorSet, nullptr);



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
			cmdBuffer.bindVertexBuffers(skinnedMesh.vertexBufferBinding, skinnedMesh.meshBuffer.vertices.buffer, vk::DeviceSize());
			cmdBuffer.bindIndexBuffer(skinnedMesh.meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);


			uint32_t setNum;
			//uint32_t offset;

			// bind scene descriptor set
			setNum = 0;
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, descriptorSets[setNum], nullptr);


			uint32_t offset1 = skinnedMesh.matrixIndex * alignedMatrixSize;
			//https://www.khronos.org/registry/vulkan/specs/1.0/apispec.html#vkCmdBindDescriptorSets
			// the third param is the set number!
			setNum = 1;
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset1);


			// todo:
			// avoid re binding the same material twice in a row
			// ie: if(lastMaterialIndex != mesh.meshBuffer.materialIndex)

			// get offset of mesh's material using meshbuffer's material index
			// and aligned material size

			if (lastMaterialIndex != skinnedMesh.meshBuffer.materialIndex) {

				lastMaterialIndex = skinnedMesh.meshBuffer.materialIndex;
				uint32_t offset2 = skinnedMesh.meshBuffer.materialIndex * alignedMaterialSize;
				// the third param is the set number!
				setNum = 2;
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, 1, &descriptorSets[setNum], 1, &offset2);
			}





			// must make pipeline layout compatible
			setNum = 3;
			vkx::Material m = this->assetManager.loadedMaterials[skinnedMesh.meshBuffer.materialIndex];
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.basic, setNum, m.descriptorSet, nullptr);



			// draw:
			cmdBuffer.drawIndexed(skinnedMesh.meshBuffer.indexCount, 1, 0, 0, 0);
		}


	}







	void prepareVertices() {

		struct meshVertex {
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
			glm::vec3 color;
		};

		struct skinnedMeshVertex {
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
			glm::vec3 color;
			// Max. four bones per vertex
			float boneWeights[4];
			uint32_t boneIDs[4];
		};

		//// Binding description
		//bindingDescriptions.resize(1);
		//bindingDescriptions[0] =
		//	vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(meshVertex), vk::VertexInputRate::eVertex);

		//// Attribute descriptions
		//attributeDescriptions.resize(4);
		//// Location 0 : Position
		//attributeDescriptions[0] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);
		//// Location 1 : Normal
		//attributeDescriptions[1] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3);
		//// Location 2 : UV
		//attributeDescriptions[2] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32Sfloat, sizeof(float) * 6);
		//// Location 3 : Color
		//attributeDescriptions[3] =
		//	vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat, sizeof(float) * 8);


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
		// Location 1 : Normal
		vertices.attributeDescriptions[1] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3);
		// Location 2 : (UV) Texture coordinates
		vertices.attributeDescriptions[2] =
			vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32Sfloat, sizeof(float) * 6);
		// Location 3 : Color
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


		

	}

	void setupDescriptorSetLayout() {


		// descriptor set layout 0
		// scene data

		std::vector<vk::DescriptorSetLayoutBinding> setLayout0Bindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout0 =
			vkx::descriptorSetLayoutCreateInfo(setLayout0Bindings.data(), setLayout0Bindings.size());

		vk::DescriptorSetLayout setLayout0 = device.createDescriptorSetLayout(descriptorLayout0);

		descriptorSetLayouts.push_back(setLayout0);



		// descriptor set layout 1
		// matrix data

		std::vector<vk::DescriptorSetLayoutBinding> setLayout1Bindings =
		{
			// Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout1 =
			vkx::descriptorSetLayoutCreateInfo(setLayout1Bindings.data(), setLayout1Bindings.size());

		vk::DescriptorSetLayout setLayout1 = device.createDescriptorSetLayout(descriptorLayout1);

		descriptorSetLayouts.push_back(setLayout1);





		// descriptor set layout 2
		// material data

		std::vector<vk::DescriptorSetLayoutBinding> setLayout2Bindings =
		{
			// Binding 0 : Vertex shader dynamic uniform buffer
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBufferDynamic,
				vk::ShaderStageFlagBits::eVertex,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout2 =
			vkx::descriptorSetLayoutCreateInfo(setLayout2Bindings.data(), setLayout2Bindings.size());

		vk::DescriptorSetLayout setLayout2 = device.createDescriptorSetLayout(descriptorLayout2);

		descriptorSetLayouts.push_back(setLayout2);



		// descriptor set layout 3
		// combined image sampler

		std::vector<vk::DescriptorSetLayoutBinding> setLayout3Bindings =
		{
			// Binding 0 : Fragment shader color map image sampler
			vkx::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				0),// binding 0
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout3 =
			vkx::descriptorSetLayoutCreateInfo(setLayout3Bindings.data(), setLayout3Bindings.size());

		vk::DescriptorSetLayout setLayout3 = device.createDescriptorSetLayout(descriptorLayout3);

		descriptorSetLayouts.push_back(setLayout3);

		this->assetManager.materialDescriptorSetLayout = &descriptorSetLayouts[3];







		//// descriptor set layout 4
		//// bone data

		//std::vector<vk::DescriptorSetLayoutBinding> setLayout4Bindings =
		//{
		//	// Binding 0 : Vertex shader uniform buffer
		//	vkx::descriptorSetLayoutBinding(
		//		vk::DescriptorType::eUniformBuffer,
		//		vk::ShaderStageFlagBits::eVertex,
		//		0),// binding 0
		//};

		//vk::DescriptorSetLayoutCreateInfo descriptorLayout4 =
		//	vkx::descriptorSetLayoutCreateInfo(setLayout4Bindings.data(), setLayout4Bindings.size());

		//vk::DescriptorSetLayout setLayout4 = device.createDescriptorSetLayout(descriptorLayout4);
		//descriptorSetLayouts.push_back(setLayout4);




		// use all descriptor set layouts
		// to form pipeline layout

		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkx::pipelineLayoutCreateInfo(descriptorSetLayouts.data(), descriptorSetLayouts.size());

		pipelineLayouts.basic = device.createPipelineLayout(pPipelineLayoutCreateInfo);

	}

	void setupDescriptorSet() {

		/*for (int i = 0; i < descriptorPools.size(); ++i) {
			vk::DescriptorSetAllocateInfo allocInfo =
				vkx::descriptorSetAllocateInfo(descriptorPools[i], &descriptorSetLayouts[i], 1);

			vk::DescriptorSet descSet = device.allocateDescriptorSets(allocInfo)[0];
			descriptorSets.push_back(descSet);
		}*/

		//std::vector<vk::DescriptorSetAllocateInfo> descAllocInfos = {
		//	vkx::descriptorSetAllocateInfo(descriptorPools[0], &descriptorSetLayouts[0], 1),
		//	vkx::descriptorSetAllocateInfo(descriptorPools[1], &descriptorSetLayouts[1], 1),
		//};




		// descriptor set 0
		// scene data
		vk::DescriptorSetAllocateInfo descriptorSetInfo0 =
			vkx::descriptorSetAllocateInfo(descriptorPools[0], &descriptorSetLayouts[0], 1);

		std::vector<vk::DescriptorSet> descSets0 = device.allocateDescriptorSets(descriptorSetInfo0);
		descriptorSets.push_back(descSets0[0]);// descriptor set 0




		// descriptor set 1
		// matrix data
		vk::DescriptorSetAllocateInfo descriptorSetInfo1 =
			vkx::descriptorSetAllocateInfo(descriptorPools[1], &descriptorSetLayouts[1], 1);

		std::vector<vk::DescriptorSet> descSets1 = device.allocateDescriptorSets(descriptorSetInfo1);
		descriptorSets.push_back(descSets1[0]);// descriptor set 1



		// descriptor set 2
		// material data
		vk::DescriptorSetAllocateInfo descriptorSetInfo2 =
			vkx::descriptorSetAllocateInfo(descriptorPools[2], &descriptorSetLayouts[2], 1);

		std::vector<vk::DescriptorSet> descSets2 = device.allocateDescriptorSets(descriptorSetInfo2);
		descriptorSets.push_back(descSets2[0]);// descriptor set 2



		// descriptor set 3
		// image sampler
		//vk::DescriptorSetAllocateInfo descriptorSetInfo3 =
		//	vkx::descriptorSetAllocateInfo(descriptorPools[3], &descriptorSetLayouts[3], 1);

		//std::vector<vk::DescriptorSet> descSets3 = device.allocateDescriptorSets(descriptorSetInfo3);
		//descriptorSets.push_back(descSets3[0]);// descriptor set 3




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

		//updateDescriptorSets();
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
		shaderStages[0] = context.loadShader(getAssetPath() + "shaders/vulkanscene/model.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = context.loadShader(getAssetPath() + "shaders/vulkanscene/model.frag.spv", vk::ShaderStageFlagBits::eFragment);
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








		// Assign pipelines
		
		// todo:
		//for (auto &mesh : meshes) {
		//	mesh.pipeline = pipelines.meshes;
		//}
		
		// why do I do this?
		// just bind pipelines.x at render?
		// todo: don't do this// just remove this
		//for (auto &model : models) {
		//	model.pipeline = pipelines.meshes;
		//}

		//for (auto &skinnedMesh : skinnedMeshes) {
		//	skinnedMesh.pipeline = pipelines.skinnedMeshes;
		//}

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

		updateUniformBuffers();// update scene ubo
		updateMatrixBuffer();// update matrix ubo
		updateMaterialBuffer();// update material ubo
		// todo: update bonedata ubo
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
				vkx::materialProperties p;
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



	void updateUniformBuffers() {
		uboScene.projection = camera.matrices.projection;
		uboScene.view = camera.matrices.view;
		uboScene.cameraPos = camera.transform.translation;

		//uboVS.model = camera.matrixNodes.skyboxView;

		// ?
		//uboScene.normal = glm::inverseTranspose(uboScene.view * uboScene.model);// fix this// important
		//uboScene.normal = glm::inverseTranspose(uboScene.view);
		//uboScene.lightPos = lightPos;

		uniformData.sceneVS.copy(uboScene);
		//uniformData.matrixVS.copy(matrixNodes);

	}


	void start() {

		vkx::Model planeModel(&context, &assetManager);
		planeModel.load(getAssetPath() + "models/plane.fbx");
		planeModel.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);




		//vkx::Model otherModel1(context, assetManager);
		//otherModel1.load(getAssetPath() + "models/plane.fbx");
		//otherModel1.createMeshes(meshVertexLayout, 0.01f, VERTEX_BUFFER_BIND_ID);


		//vkx::Model otherModel1(context, assetManager);
		//otherModel1.load(getAssetPath() + "models/vulkanscenemodels.dae");
		//otherModel1.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//vkx::Model otherModel1(context, assetManager);
		//otherModel1.load(getAssetPath() + "models/sibenik/sibenik.dae");
		//otherModel1.createMeshes(meshVertexLayout, 0.5f, VERTEX_BUFFER_BIND_ID);

		vkx::Model otherModel2(&context, &assetManager);
		otherModel2.load(getAssetPath() + "models/monkey.fbx");
		otherModel2.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//vkx::Model otherModel3(context, assetManager);
		//otherModel3.load(getAssetPath() + "models/myCube.dae");
		//otherModel3.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//vkx::Model otherModel2(context, assetManager);
		//otherModel2.load(getAssetPath() + "models/vulkanscenemodels.dae");
		//otherModel2.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

		//vkx::Model otherModel5(context, assetManager);
		//otherModel5.load(getAssetPath() + "models/cube.obj");
		//otherModel5.createMeshes(meshVertexLayout, 0.02f, VERTEX_BUFFER_BIND_ID);



		//meshes.push_back(skyboxMesh);
		//meshes.push_back(planeMesh);
		//meshes.push_back(otherMesh1);

		models.push_back(planeModel);
		//models.push_back(otherModel1);
		//models.push_back(otherModel2);
		//models.push_back(otherModel3);
		//models.push_back(otherModel4);
		//models.push_back(otherModel5);

		for (int i = 0; i < 6; ++i) {

			vkx::Model testModel(&context, &assetManager);
			testModel.load(getAssetPath() + "models/monkey.fbx");
			testModel.createMeshes(meshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);

			models.push_back(testModel);
		}




		vkx::SkinnedMesh skinnedMesh1(&context, &assetManager);

		//vkx::SkinnedMesh *skinnedMesh1 = new vkx::SkinnedMesh(&context, &assetManager);
		//delete skinnedMesh1->meshLoader;
		//delete skinnedMesh1;

		int flags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
		//skinnedMesh1.load(getAssetPath() + "models/goblin.dae", flags);
		skinnedMesh1.load(getAssetPath() + "models/goblin.dae", 0);
		skinnedMesh1.setup(0.0005f);
		//skinnedMesh1.setup(1.0f);
		//skinnedMesh1.createMeshes(skinnedMeshVertexLayout, 1.0f, VERTEX_BUFFER_BIND_ID);
		//skinnedMesh1.update(0.0f);


		skinnedMeshes.push_back(skinnedMesh1);

		// after any model loading with materials has occurred, updateMaterialBuffer() must be called
		// *after any material loading
		updateMaterialBuffer();

	}



	void prepare() {
		vulkanApp::prepare();

		// todo: remove this:
		loadTextures();

		
		prepareVertices();

		

		prepareUniformBuffers();

		setupDescriptorSetLayout();
		setupDescriptorPool();
		setupDescriptorSet();

		preparePipelines();

		start();


		updateDrawCommandBuffers();

		

		prepared = true;
	}

	virtual void render() {
		if (!prepared) {
			return;
		}
		draw();
	}

	virtual void viewChanged() {
		updateTextOverlay(); //disable this
		updateUniformBuffers();
	}

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

		//textOverlay->addText("camera stats:", 5.0f, vOffset, vkx::TextOverlay::alignLeft);

		//textOverlay->addText("log: ", 5.0f, vOffset + 20.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText(consoleLog.c_str(), 5.0f, vOffset + 40.0f, vkx::TextOverlay::alignLeft);

		if (models.size() > 4) {
			textOverlay->addText(std::to_string(models[2].transform.translation.x), 5.0f, vOffset + 60.0f, vkx::TextOverlay::alignLeft);
			textOverlay->addText(std::to_string(models[3].transform.translation.x), 5.0f, vOffset + 80.0f, vkx::TextOverlay::alignLeft);
		}

		//textOverlay->addText("rotation(q) w: " + std::to_string(camera.rotation.w), 5.0f, vOffset + 20.0f, vkx::TextOverlay::alignLeft);
		//textOverlay->addText("rotation(q) x: " + std::to_string(camera.rotation.x), 5.0f, vOffset + 40.0f, vkx::TextOverlay::alignLeft);
		//textOverlay->addText("rotation(q) y: " + std::to_string(camera.rotation.y), 5.0f, vOffset + 60.0f, vkx::TextOverlay::alignLeft);
		//textOverlay->addText("rotation(q) z: " + std::to_string(camera.rotation.z), 5.0f, vOffset + 80.0f, vkx::TextOverlay::alignLeft);

		textOverlay->addText("pos x: " + std::to_string(camera.transform.translation.x), 5.0f, vOffset + 100.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("pos y: " + std::to_string(camera.transform.translation.y), 5.0f, vOffset + 120.0f, vkx::TextOverlay::alignLeft);
		textOverlay->addText("pos z: " + std::to_string(camera.transform.translation.z), 5.0f, vOffset + 140.0f, vkx::TextOverlay::alignLeft);
	}

};



VulkanExample *vulkanExample;

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	VulkanExample* example = new VulkanExample();
	example->run();
	delete(example);
	return 0;
}