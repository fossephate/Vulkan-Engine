
#include "vulkanMesh.h"



namespace vkx {

	//http://stackoverflow.com/questions/12927169/how-can-i-initialize-c-object-member-variables-in-the-constructor
	//http://stackoverflow.com/questions/14169584/passing-and-storing-a-const-reference-via-a-constructor

	Mesh::Mesh(vkx::MeshBuffer meshBuffer) {
		this->meshBuffer = meshBuffer;
	}

	void Mesh::destroy() {
		this->meshBuffer.destroy();
	}

}