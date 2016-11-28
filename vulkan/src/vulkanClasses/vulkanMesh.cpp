



#include "vulkanMesh.h"



namespace vkx {



	Mesh::Mesh() {
		this->meshLoader = new vkx::MeshLoader();
	}

	Mesh::Mesh(const std::string & filename) {
		this->load(filename);
	}

	void Mesh::load(const std::string & filename) {
		this->meshLoader->load(filename);
	}

	void Mesh::load(const std::string & filename, int flags) {
		this->meshLoader->load(filename, flags);
		
	}

	void Mesh::createBuffers(const Context & context, const std::vector<VertexLayout>& layout, float scale, uint32_t binding) {
		this->meshBuffer = this->meshLoader->createBuffers(context, layout, scale);

		this->attributeDescriptions = this->meshLoader->attributeDescriptions;

		this->vertexBufferBinding = binding;// important
		this->setupVertexInputState(layout);// doesn't seem to be necessary/used

		//this->bindingDescription = this->meshLoader->bindingDescriptions[0];// ?
		//this->pipeline = this->meshLoader->pipeline;// not needed?
	}



	void Mesh::setupVertexInputState(const std::vector<VertexLayout>& layout) {
		bindingDescription = vertexInputBindingDescription(
			vertexBufferBinding,
			vertexSize(layout),
			vk::VertexInputRate::eVertex);

		attributeDescriptions.clear();
		uint32_t offset = 0;
		uint32_t binding = 0;
		for (auto& layoutDetail : layout) {
			// vk::Format (layout)
			vk::Format format = (layoutDetail == VERTEX_LAYOUT_UV) ? vk::Format::eR32G32Sfloat : vk::Format::eR32G32B32Sfloat;

			attributeDescriptions.push_back(
				vertexInputAttributeDescription(
					vertexBufferBinding,
					binding,
					format,
					offset));

			// Offset
			offset += (layoutDetail == VERTEX_LAYOUT_UV) ? (2 * sizeof(float)) : (3 * sizeof(float));
			binding++;
		}

		vertexInputState = vk::PipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = 1;
		vertexInputState.pVertexBindingDescriptions = &bindingDescription;
		vertexInputState.vertexAttributeDescriptionCount = attributeDescriptions.size();
		vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
	}

	void Mesh::drawIndexed(const vk::CommandBuffer & cmdBuffer) {

		// todo: add more
		if (pipeline) {
			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		}
		if ((pipelineLayout) && (descriptorSet)) {
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
		}

		cmdBuffer.bindVertexBuffers(vertexBufferBinding, meshBuffer.vertices.buffer, vk::DeviceSize());
		cmdBuffer.bindIndexBuffer(meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);
		cmdBuffer.drawIndexed(meshBuffer.indexCount, 1, 0, 0, 0);
	}

}