#pragma once


#include "vulkanContext.h"
#include "vulkanAssetManager.h"
#include "Object3D.h"


namespace vkx {

	//extern struct materialProperties;
	//extern struct Material;



	// stores mesh info
	class Mesh : public Object3D {
		public:


		//uint32_t matrixIndex;

		// Mesh buffer
		vkx::MeshBuffer meshBuffer;

		// pointer to the material used by this mesh
		//vkx::Material *material;

		//http://www.learncpp.com/cpp-tutorial/8-5a-constructor-member-initializer-lists/


		// todo: fix:
		uint32_t vertexBufferBinding = 0;// not that necessary for most uses

		//vk::Pipeline pipeline;
		//vk::PipelineLayout pipelineLayout;
		//vk::DescriptorSet descriptorSet;

		//vk::PipelineVertexInputStateCreateInfo vertexInputState;
		//vk::VertexInputBindingDescription bindingDescription;
		//std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;


		// no default constructor
		Mesh();

		Mesh(vkx::MeshBuffer meshBuffer);

		//Mesh(vkx::MeshBuffer meshBuffer, Material material);

		//void createMeshBuffer(const std::vector<VertexLayout> &layout, float scale, uint32_t binding);
		//void setupVertexInputState(const std::vector<VertexLayout> &layout);
		//void drawIndexed(const vk::CommandBuffer& cmdBuffer);


	};

}