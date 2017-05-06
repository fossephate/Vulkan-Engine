

#include "vulkanModel.h"

namespace vkx {

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


	void Model::asyncCreateMeshes(const std::vector<VertexComponent> &layout, float scale, uint32_t binding) {
		
		// load according to layout and scale
		this->meshLoader->createMeshBuffers(layout, scale);

		this->meshBuffers = this->meshLoader->meshBuffers;

		//std::vector<std::shared_ptr<MeshBuffer>> &meshBuffers = this->meshLoader->meshBuffers;
		//for (int i = 0; i < meshBuffers.size(); ++i) {
		//	vkx::Mesh m(meshBuffers[i]);
		//	this->meshes.push_back(m);
		//}


		this->vertexBufferBinding = binding;

		this->buffersReady = true;
	}




	void Model::createMeshes(const std::vector<VertexComponent> &layout, float scale, uint32_t binding) {
		/* Run some task on new thread. The launch policy std::launch::async
		makes sure that the task is run asynchronously on a new thread. */
		//this->myFuture = std::async(std::launch::async, [] {
		//});
		bool useAsync = false;
		if (useAsync) {
			this->myFuture = std::async(std::launch::async, &Model::asyncCreateMeshes, this, layout, scale, binding);
		} else {
			asyncCreateMeshes(layout, scale, binding);
		}

	}






	//void Model::asyncLoadAndCreateMeshes(const std::string &filename, const std::vector<VertexLayout>& layout, float scale, uint32_t binding) {
	//	this->meshLoader->load(filename);

	//	this->meshLoader->createMeshBuffers(layout, scale);

	//	std::vector<MeshBuffer> meshBuffers = this->meshLoader->meshBuffers;

	//	this->meshes.resize(meshBuffers.size());

	//	for (int i = 0; i < meshBuffers.size(); ++i) {
	//		vkx::Mesh m(meshBuffers[i]);
	//		this->meshes[i] = m;
	//	}

	//	this->vertexBufferBinding = binding;// important

	//	this->buffersReady = true;
	//}

	//void Model::loadAndCreateMeshes(const std::string &filename, const std::vector<VertexLayout>& layout, float scale, uint32_t binding) {

	//	this->myFuture = std::async(std::launch::async, &Model::asyncLoadAndCreateMeshes, this, filename, layout, scale, binding);
	//	this->buffersReady = true;
	//}


	void Model::destroy() {
		for (auto &meshBuffer : this->meshBuffers) {
			meshBuffer->destroy();
		}
		// todo:
		// more to delete:
		this->meshLoader->destroy();// todo: implement
		delete this->meshLoader;
	}



}