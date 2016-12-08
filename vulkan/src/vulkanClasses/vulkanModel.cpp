

#include "vulkanModel.h"

namespace vkx {

	//http://stackoverflow.com/questions/12927169/how-can-i-initialize-c-object-member-variables-in-the-constructor
	//http://stackoverflow.com/questions/14169584/passing-and-storing-a-const-reference-via-a-constructor

	// constructors

	// don't ever use this constructor
	Model::Model() :
		context(vkx::Context()), assetManager(assetManager)
	{
		//this->context = nullptr;
		//this->meshLoader = new vkx::MeshLoader();
	}

	//// reference way:
	//Model::Model(const vkx::Context &context):
	//	context(context)// init context with reference
	//{
	//	this->meshLoader = new vkx::MeshLoader(context);
	//}



	// reference way:
	Model::Model(const vkx::Context &context, vkx::AssetManager &assetManager) :
		context(context), assetManager(assetManager)// init context with reference
	{
		this->meshLoader = new vkx::MeshLoader(context, assetManager);
	}


	//~Model::Model(){}



	void Model::load(const std::string &filename) {
		this->meshLoader->load(filename);
	}

	void Model::load(const std::string &filename, int flags) {
		this->meshLoader->load(filename, flags);
	}

	// rename to createMeshes?
	void Model::createMeshes(const std::vector<VertexLayout> &layout, float scale, uint32_t binding) {

		this->meshLoader->createMeshBuffers(this->context, layout, scale);



		std::vector<MeshBuffer> meshBuffers = this->meshLoader->meshBuffers;

		// copy vector of materials this class
		//this->materials = this->meshLoader->materials;

		for (int i = 0; i < meshBuffers.size(); ++i) {
			//vkx::MeshBuffer &mBuffer = meshBuffers[i];
			vkx::Mesh m(meshBuffers[i]);

			//this->meshes.push_back(vkx::Mesh(mBuffer, this->materials[mBuffer.materialIndex]));
			this->meshes.push_back(m);
		}

		//this->materials = this->meshLoader->materials;

		//this->attributeDescriptions = this->meshLoader->attributeDescriptions;

		this->vertexBufferBinding = binding;// important
		//this->setupVertexInputState(layout);// doesn't seem to be necessary/used

		//this->bindingDescription = this->meshLoader->bindingDescriptions[0];// ?
		//this->pipeline = this->meshLoader->pipeline;// not needed?
	}


	void Model::drawIndexed(const vk::CommandBuffer &cmdBuffer) {

		// todo: add more
		//if (pipeline) {
		//	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		//}


		//if ((pipelineLayout) && (descriptorSet)) {
		//	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
		//}







		//cmdBuffer.bindVertexBuffers(vertexBufferBinding, meshBuffer.vertices.buffer, vk::DeviceSize());
		//cmdBuffer.bindIndexBuffer(meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);
		//cmdBuffer.drawIndexed(meshBuffer.indexCount, 1, 0, 0, 0);

	}



}