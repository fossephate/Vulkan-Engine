#include "vulkanSkinnedMesh.h"



namespace vkx {


	SkinnedMesh::SkinnedMesh() {
		//this->context = nullptr;
		//this->assetManager = nullptr;
	}

	SkinnedMesh::SkinnedMesh(vkx::Context *context, vkx::AssetManager *assetManager) {

		this->context = context;
		this->meshLoader = new vkx::MeshLoader(context, assetManager);
	}


	void SkinnedMesh::load(const std::string &filename) {
		// default flags are different for skinned meshes
		int flags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
		this->meshLoader->load(filename, flags);
	}

	void SkinnedMesh::load(const std::string &filename, int flags) {
		this->meshLoader->load(filename, flags);
	}

	void SkinnedMesh::createSkinnedMeshBuffer(const std::vector<VertexLayout> &layout, float scale) {
		this->meshLoader->createSkinnedMeshBuffer(layout, scale);
		this->meshBuffer = this->meshLoader->combinedBuffer;
	}


	// Set active animation by index
	void SkinnedMesh::setAnimation(uint32_t animationIndex) {
		this->meshLoader->setAnimation(animationIndex);
	}

	// Recursive bone transformation for given animation time
	void SkinnedMesh::update(float time) {
		this->meshLoader->update(time);
	}

	void SkinnedMesh::destroy() {
		this->meshBuffer.destroy();
		delete this->meshLoader;
	}












}