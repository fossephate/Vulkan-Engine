

#include "vulkanModel.h"

namespace vkx {


	void asyncCreateMeshes2(Model model, const std::vector<VertexLayout> &layout, float scale, uint32_t binding) {
		model.meshLoader->createMeshBuffers(layout, scale);

		std::vector<MeshBuffer> meshBuffers = model.meshLoader->meshBuffers;

		for (int i = 0; i < meshBuffers.size(); ++i) {
			vkx::Mesh m(meshBuffers[i]);
			model.meshes.push_back(m);
		}

		// todo: destroy this->meshLoader->meshBuffers here:
		//for (int i = 0; i < this->meshLoader->meshBuffers.size(); ++i) {
		//	// destroy here
		//}

		model.vertexBufferBinding = binding;// important
	}

	//http://stackoverflow.com/questions/12927169/how-can-i-initialize-c-object-member-variables-in-the-constructor
	//http://stackoverflow.com/questions/14169584/passing-and-storing-a-const-reference-via-a-constructor


	Model::Model(vkx::Context *context, vkx::AssetManager *assetManager) {
		this->meshLoader = new vkx::MeshLoader(context, assetManager);
	}


	void Model::load(const std::string &filename) {
		this->meshLoader->load(filename);
	}

	void Model::load(const std::string &filename, int flags) {
		this->meshLoader->load(filename, flags);
	}


	void Model::asyncCreateMeshes(const std::vector<VertexLayout> &layout, float scale, uint32_t binding) {
		this->meshLoader->createMeshBuffers(layout, scale);

		std::vector<MeshBuffer> meshBuffers = this->meshLoader->meshBuffers;

		for (int i = 0; i < meshBuffers.size(); ++i) {
			vkx::Mesh m(meshBuffers[i]);
			this->meshes.push_back(m);
		}

		// todo: destroy this->meshLoader->meshBuffers here:
		//for (int i = 0; i < this->meshLoader->meshBuffers.size(); ++i) {
		//	// destroy here
		//}

		this->vertexBufferBinding = binding;// important
	}




	void Model::createMeshes(const std::vector<VertexLayout> &layout, float scale, uint32_t binding) {

		/* Run some task on new thread. The launch policy std::launch::async
		makes sure that the task is run asynchronously on a new thread. */
		//this->myFuture = std::async(std::launch::async, [] {
		//	this->meshLoader->createMeshBuffers(layout, scale);
		//	std::vector<MeshBuffer> meshBuffers = this->meshLoader->meshBuffers;
		//	for (int i = 0; i < meshBuffers.size(); ++i) {
		//		vkx::Mesh m(meshBuffers[i]);
		//		this->meshes.push_back(m);
		//	}
		//	// todo: destroy this->meshLoader->meshBuffers here:
		//	//for (int i = 0; i < this->meshLoader->meshBuffers.size(); ++i) {
		//	//	// destroy here
		//	//}
		//	//this->materials = this->meshLoader->materials;
		//	//this->attributeDescriptions = this->meshLoader->attributeDescriptions;
		//	this->vertexBufferBinding = binding;// important
		//});

		//this->myFuture = std::async(asyncCreateMeshes, *this, layout, scale, binding);
		//this->myFuture = std::async(&Model::asyncCreateMeshes, *this, layout, scale, binding);

		this->myFuture = std::async(asyncCreateMeshes2, this, layout, scale, binding);

		//this->meshLoader->createMeshBuffers(layout, scale);

		//std::vector<MeshBuffer> meshBuffers = this->meshLoader->meshBuffers;

		//for (int i = 0; i < meshBuffers.size(); ++i) {
		//	vkx::Mesh m(meshBuffers[i]);
		//	this->meshes.push_back(m);
		//}
		//
		//// todo: destroy this->meshLoader->meshBuffers here:
		////for (int i = 0; i < this->meshLoader->meshBuffers.size(); ++i) {
		////	// destroy here
		////}

		//this->vertexBufferBinding = binding;// important



	}



	void Model::checkIfReady() {

		// Use wait_for() with zero milliseconds to check thread status.
		auto status = this->myFuture.wait_for(std::chrono::milliseconds(0));

		// Print status.
		if (status == std::future_status::ready) {
			this->buffersReady = true;
			//std::cout << "Thread finished" << std::endl;
		} else {
			//std::cout << "Thread still running" << std::endl;
		}
	}

	void Model::destroy() {
		for (auto &mesh : this->meshes) {
			mesh.meshBuffer.destroy();
		}
		// todo:
		// more to delete:
		delete this->meshLoader;
	}



}