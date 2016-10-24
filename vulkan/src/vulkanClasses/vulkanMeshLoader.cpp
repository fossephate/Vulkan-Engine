#include "vulkanMeshLoader.h"

// initialization
vkx::Mesh::Mesh(vkx::MeshBuffer mBuffer, uint32_t binding, const std::vector<VertexLayout>& layout)
{
	this->buffers = mBuffer;
	this->vertexBufferBinding = binding;
	this->setupVertexInputState(layout);
}

void vkx::Mesh::setupVertexInputState(const std::vector<VertexLayout>& layout) {
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

void vkx::Mesh::drawIndexed(const vk::CommandBuffer & cmdBuffer) {
	if (pipeline) {
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	}
	if ((pipelineLayout) && (descriptorSet)) {
		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
	}
	cmdBuffer.bindVertexBuffers(vertexBufferBinding, buffers.vertices.buffer, vk::DeviceSize());
	cmdBuffer.bindIndexBuffer(buffers.indices.buffer, 0, vk::IndexType::eUint32);
	cmdBuffer.drawIndexed(buffers.indexCount, 1, 0, 0, 0);
}

vkx::MeshLoader::~MeshLoader() {
	m_Entries.clear();
}

// Loads the mesh with some default flags

bool vkx::MeshLoader::load(const std::string & filename) {
	int flags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

	return load(filename, flags);
}

// Load the mesh with custom flags

bool vkx::MeshLoader::load(const std::string & filename, int flags) {
	#if defined(__ANDROID__)
	// Meshes are stored inside the apk on Android (compressed)
	// So they need to be loaded via the asset manager

	AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
	assert(asset);
	size_t size = AAsset_getLength(asset);

	assert(size > 0);

	void *meshData = malloc(size);
	AAsset_read(asset, meshData, size);
	AAsset_close(asset);

	pScene = Importer.ReadFileFromMemory(meshData, size, flags);

	free(meshData);
	#else
	pScene = Importer.ReadFile(filename.c_str(), flags);
	#endif
	if (!pScene) {
		throw std::runtime_error("Unable to parse " + filename);
	}
	return parse(pScene, filename);
}

bool vkx::MeshLoader::parse(const aiScene * pScene, const std::string & Filename) {
	m_Entries.resize(pScene->mNumMeshes);

	// Counters
	for (unsigned int i = 0; i < m_Entries.size(); i++) {
		m_Entries[i].vertexBase = numVertices;
		numVertices += pScene->mMeshes[i]->mNumVertices;
	}

	// Initialize the meshes in the scene one by one
	for (unsigned int i = 0; i < m_Entries.size(); i++) {
		const aiMesh* paiMesh = pScene->mMeshes[i];
		InitMesh(i, paiMesh, pScene);
	}

	return true;
}

void vkx::MeshLoader::InitMesh(unsigned int index, const aiMesh * paiMesh, const aiScene * pScene) {
	m_Entries[index].MaterialIndex = paiMesh->mMaterialIndex;

	aiColor3D pColor(0.f, 0.f, 0.f);
	pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

	aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

	for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
		aiVector3D* pPos = &(paiMesh->mVertices[i]);
		aiVector3D* pNormal = &(paiMesh->mNormals[i]);
		aiVector3D *pTexCoord;
		if (paiMesh->HasTextureCoords(0)) {
			pTexCoord = &(paiMesh->mTextureCoords[0][i]);
		} else {
			pTexCoord = &Zero3D;
		}
		aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[i]) : &Zero3D;
		aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[i]) : &Zero3D;

		Vertex v(glm::vec3(pPos->x, -pPos->y, pPos->z),
			glm::vec2(pTexCoord->x, pTexCoord->y),
			glm::vec3(pNormal->x, pNormal->y, pNormal->z),
			glm::vec3(pTangent->x, pTangent->y, pTangent->z),
			glm::vec3(pBiTangent->x, pBiTangent->y, pBiTangent->z),
			glm::vec3(pColor.r, pColor.g, pColor.b)
		);

		dim.max.x = fmax(pPos->x, dim.max.x);
		dim.max.y = fmax(pPos->y, dim.max.y);
		dim.max.z = fmax(pPos->z, dim.max.z);

		dim.min.x = fmin(pPos->x, dim.min.x);
		dim.min.y = fmin(pPos->y, dim.min.y);
		dim.min.z = fmin(pPos->z, dim.min.z);

		m_Entries[index].Vertices.push_back(v);
	}

	dim.size = dim.max - dim.min;

	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
		const aiFace& Face = paiMesh->mFaces[i];
		if (Face.mNumIndices != 3)
			continue;
		m_Entries[index].Indices.push_back(Face.mIndices[0]);
		m_Entries[index].Indices.push_back(Face.mIndices[1]);
		m_Entries[index].Indices.push_back(Face.mIndices[2]);
	}
}

// Create vertex and index buffer with given layout
// Note : Only does staging if a valid command buffer and transfer queue are passed

vkx::MeshBuffer vkx::MeshLoader::createBuffers(const Context & context, const std::vector<VertexLayout>& layout, float scale) {

	std::vector<float> vertexBuffer;
	for (int m = 0; m < m_Entries.size(); m++) {
		for (int i = 0; i < m_Entries[m].Vertices.size(); i++) {
			// Push vertex data depending on layout
			for (auto& layoutDetail : layout) {
				// Position
				if (layoutDetail == VERTEX_LAYOUT_POSITION) {
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.x * scale);
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.y * scale);
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.z * scale);
				}
				// Normal
				if (layoutDetail == VERTEX_LAYOUT_NORMAL) {
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.x);
					vertexBuffer.push_back(-m_Entries[m].Vertices[i].m_normal.y);
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.z);
				}
				// Texture coordinates
				if (layoutDetail == VERTEX_LAYOUT_UV) {
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.s);
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.t);
				}
				// Color
				if (layoutDetail == VERTEX_LAYOUT_COLOR) {
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.r);
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.g);
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.b);
				}
				// Tangent
				if (layoutDetail == VERTEX_LAYOUT_TANGENT) {
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.x);
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.y);
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.z);
				}
				// Bitangent
				if (layoutDetail == VERTEX_LAYOUT_BITANGENT) {
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.x);
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.y);
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.z);
				}
				// Dummy layout components for padding
				if (layoutDetail == VERTEX_LAYOUT_DUMMY_FLOAT) {
					vertexBuffer.push_back(0.0f);
				}
				if (layoutDetail == VERTEX_LAYOUT_DUMMY_VEC4) {
					vertexBuffer.push_back(0.0f);
					vertexBuffer.push_back(0.0f);
					vertexBuffer.push_back(0.0f);
					vertexBuffer.push_back(0.0f);
				}
			}
		}
	}

	MeshBuffer meshBuffer;
	meshBuffer.vertices.size = vertexBuffer.size() * sizeof(float);

	dim.min *= scale;
	dim.max *= scale;
	dim.size *= scale;

	std::vector<uint32_t> indexBuffer;
	for (uint32_t m = 0; m < m_Entries.size(); m++) {
		uint32_t indexBase = (uint32_t)indexBuffer.size();
		for (uint32_t i = 0; i < m_Entries[m].Indices.size(); i++) {
			indexBuffer.push_back(m_Entries[m].Indices[i] + indexBase);
		}
	}

	meshBuffer.indexCount = (uint32_t)indexBuffer.size();
	// Use staging buffer to move vertex and index buffer to device local memory
	// Vertex buffer
	meshBuffer.vertices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
	// Index buffer
	meshBuffer.indices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
	meshBuffer.dim = dim.size;
	return meshBuffer;
}

vkx::Mesh vkx::MeshLoader::createMeshFromBuffers(const Context & context, const std::vector<VertexLayout>& layout, float scale, uint32_t binding)
{
	vkx::MeshBuffer mBuffer = createBuffers(context, layout, scale);
	vkx::Mesh mesh = vkx::Mesh(mBuffer, binding, layout);
	mesh.attributeDescriptions = this->attributeDescriptions;
	mesh.vertexBufferBinding = binding;
	//mesh.bindingDescription = this->bindingDescriptions[0];
	mesh.pipeline = this->pipeline;
	
	return mesh;
}


