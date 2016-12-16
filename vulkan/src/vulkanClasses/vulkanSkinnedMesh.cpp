#include "vulkanSkinnedMesh.h"



namespace vkx {


	// don't ever use this constructor
	SkinnedMesh::SkinnedMesh() {

	}

	SkinnedMesh::SkinnedMesh(vkx::MeshBuffer meshBuffer) {
		this->meshBuffer = meshBuffer;
	}


	void SkinnedMesh::load(const std::string &filename) {
		this->meshLoader->load(filename);
	}

	void SkinnedMesh::load(const std::string &filename, int flags) {
		this->meshLoader->load(filename, flags);
	}

	// rename to createMeshes?
	void SkinnedMesh::createMeshes(const std::vector<VertexLayout> &layout, float scale, uint32_t binding) {

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

}