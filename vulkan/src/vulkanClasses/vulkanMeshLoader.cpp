#include "vulkanMeshLoader.h"



//vkx::MeshLoader::MeshLoader() {
//	this->textureLoader = nullptr;
//}

vkx::MeshLoader::MeshLoader(const vkx::Context &context) {
	this->textureLoader = new vkx::TextureLoader(context);
}


// deconstructor
vkx::MeshLoader::~MeshLoader() {
	m_Entries.clear();
}



// Loads the mesh with some default flags
bool vkx::MeshLoader::load(const std::string &filename) {
	int flags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

	return load(filename, flags);
}

// Load the mesh with custom flags
bool vkx::MeshLoader::load(const std::string &filename, int flags) {
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

	//pScene->mRootNode->mTransformation = glm::rotate(glm::mat4(1.0f), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)) * pScene->mRootNode->mTransformation;
	//pScene->mRootNode->mTransformation
	//aiMatrix4x4 &r = pScene->mRootNode->mTransformation;
	//aiMatrix4x4 rot;
	//rot.FromEulerAnglesXYZ(aiVector3D(-90.0f, 0.0f, 0.0f));
	//pScene->mRootNode->mTransformation = rot * pScene->mRootNode->mTransformation;// change collada model back to z up// important
	////pScene->mRootNode->mTransformation = pScene->mRootNode->mTransformation * rot;
	//glm::mat4 r2 = glm::mat4(\
	//	r.a1, r.a2, r.a3, r.a4,
	//	r.b1, r.b2, r.b3, r.b4,
	//	r.c1, r.c2, r.c3, r.c4, 
	//	r.d1, r.d2, r.d3, r.d4);
	//r2 = glm::rotate(glm::mat4(1.0f), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)) * r2;
	//glm::rotate(glm::mat4(1.0f), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));

	//RootNodeMatrix = glm::rotate(glm::mat4(1.0f), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)) * RootNodeMatrix;

	if (!pScene) {
		throw std::runtime_error("Unable to parse " + filename);
	}
	return parse(pScene, filename);
}



void vkx::MeshLoader::loadMaterials(const aiScene *pScene) {

	// keep track of original size
	int numOfMaterials = globalMaterials.size();

	//materials.resize(pScene->mNumMaterials);

	for (size_t i = 0; i < pScene->mNumMaterials; i++) {
		materials[i] = {};

		aiString name;
		pScene->mMaterials[i]->Get(AI_MATKEY_NAME, name);

		// Properties
		aiColor4D color;
		pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, color);
		materials[i].properties.ambient = glm::make_vec4(&color.r) + glm::vec4(0.1f);
		pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		materials[i].properties.diffuse = glm::make_vec4(&color.r);
		pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, color);
		materials[i].properties.specular = glm::make_vec4(&color.r);
		pScene->mMaterials[i]->Get(AI_MATKEY_OPACITY, materials[i].properties.opacity);

		if ((materials[i].properties.opacity) > 0.0f) {
			materials[i].properties.specular = glm::vec4(0.0f);
		}

		materials[i].name = name.C_Str();
		std::cout << "Material \"" << materials[i].name << "\"" << std::endl;

		// Textures
		aiString texturefile;
		std::string assetPath = getAssetPath()+ "models/";

		// Diffuse
		pScene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texturefile);
		if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			std::cout << "  Diffuse: \"" << texturefile.C_Str() << "\"" << std::endl;
			std::string fileName = std::string(texturefile.C_Str());
			std::replace(fileName.begin(), fileName.end(), '\\', '/');
			materials[i].diffuse = textureLoader->loadTexture(assetPath + fileName, vk::Format::eBc3UnormBlock);
		} else {

			std::cout << "  Material has no diffuse, using dummy texture!" << std::endl;
			// todo : separate pipeline and layout
			materials[i].diffuse = textureLoader->loadTexture(assetPath + "dummy.ktx", vk::Format::eBc2UnormBlock);
		}

		// For scenes with multiple textures per material we would need to check for additional texture types, e.g.:
		// aiTextureType_HEIGHT, aiTextureType_OPACITY, aiTextureType_SPECULAR, etc.

		// Assign pipeline
		//materials[i].pipeline = (materials[i].properties.opacity == 0.0f) ? &pipelines.solid : &pipelines.blending;

		// add materials to global materials vector
		globalMaterials.push_back(materials[i]);
	}

	

	





	// Generate descriptor sets for the materials

	//// Descriptor pool
	//std::vector<vk::DescriptorPoolSize> poolSizes;
	//poolSizes.push_back(vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(materials.size())));
	//poolSizes.push_back(vkx::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(materials.size())));

	//vk::DescriptorPoolCreateInfo descriptorPoolInfo =
	//	vkx::descriptorPoolCreateInfo(
	//		static_cast<uint32_t>(poolSizes.size()),
	//		poolSizes.data(),
	//		static_cast<uint32_t>(materials.size()) + 1);

	//descriptorPool = device.createDescriptorPool(descriptorPoolInfo);

	//// Descriptor set and pipeline layouts
	//std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings;
	//vk::DescriptorSetLayoutCreateInfo descriptorLayout;

	//// Set 0: Scene matrices
	//setLayoutBindings.push_back(vkx::descriptorSetLayoutBinding(
	//	vk::DescriptorType::eUniformBuffer,
	//	vk::ShaderStageFlagBits::eVertex,
	//	0));
	//descriptorLayout = vkx::descriptorSetLayoutCreateInfo(
	//	setLayoutBindings.data(),
	//	static_cast<uint32_t>(setLayoutBindings.size()));
	//descriptorSetLayouts.scene = device.createDescriptorSetLayout(descriptorLayout);

	//// Set 1: Material data
	//setLayoutBindings.clear();
	//setLayoutBindings.push_back(vkx::descriptorSetLayoutBinding(
	//	vk::DescriptorType::eCombinedImageSampler,
	//	vk::ShaderStageFlagBits::eFragment,
	//	0));
	//descriptorSetLayouts.material = device.createDescriptorSetLayout(descriptorLayout);

	//// Setup pipeline layout
	//std::array<vk::DescriptorSetLayout, 2> setLayouts = { descriptorSetLayouts.scene, descriptorSetLayouts.material };
	//vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vkx::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));

	//// We will be using a push constant block to pass material properties to the fragment shaders
	//vk::PushConstantRange pushConstantRange = vkx::pushConstantRange(
	//	vk::ShaderStageFlagBits::eFragment,
	//	sizeof(SceneMaterialProperites),
	//	0);
	//pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	//pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	//pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

	//// Material descriptor sets
	//for (size_t i = 0; i < materials.size(); i++) {
	//	// Descriptor set
	//	vk::DescriptorSetAllocateInfo allocInfo =
	//		vkx::descriptorSetAllocateInfo(
	//			descriptorPool,
	//			&descriptorSetLayouts.material,
	//			1);

	//	materials[i].descriptorSet = device.allocateDescriptorSets(allocInfo)[0];

	//	vk::DescriptorImageInfo texDescriptor =
	//		vkx::descriptorImageInfo(
	//			materials[i].diffuse.sampler,
	//			materials[i].diffuse.view,
	//			vk::ImageLayout::eGeneral);

	//	std::vector<vk::WriteDescriptorSet> writeDescriptorSets;

	//	// todo : only use image sampler descriptor set and use one scene ubo for matrices

	//	// Binding 0: Diffuse texture
	//	writeDescriptorSets.push_back(vkx::writeDescriptorSet(
	//		materials[i].descriptorSet,
	//		vk::DescriptorType::eCombinedImageSampler,
	//		0,
	//		&texDescriptor));

	//	device.updateDescriptorSets(writeDescriptorSets, {});
	//}

	//// Scene descriptor set
	//vk::DescriptorSetAllocateInfo allocInfo =
	//	vkx::descriptorSetAllocateInfo(
	//		descriptorPool,
	//		&descriptorSetLayouts.scene,
	//		1);
	//descriptorSetScene = device.allocateDescriptorSets(allocInfo)[0];

	//std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
	//// Binding 0 : Vertex shader uniform buffer
	//writeDescriptorSets.push_back(vkx::writeDescriptorSet(
	//	descriptorSetScene,
	//	vk::DescriptorType::eUniformBuffer,
	//	0,
	//	&uniformBuffer.descriptor));

	//device.updateDescriptorSets(writeDescriptorSets, {});
}



bool vkx::MeshLoader::parse(const aiScene *pScene, const std::string &Filename) {
	m_Entries.resize(pScene->mNumMeshes);

	// Counters
	for (unsigned int i = 0; i < m_Entries.size(); i++) {
		m_Entries[i].vertexBase = numVertices;
		numVertices += pScene->mMeshes[i]->mNumVertices;// total for all vertices
	}


	loadMaterials(pScene);
	loadMeshes(pScene);

	// Initialize the meshes in the scene one by one
	//for (unsigned int i = 0; i < m_Entries.size(); i++) {
	//	const aiMesh* paiMesh = pScene->mMeshes[i];
	//	InitMesh(i, paiMesh, pScene);
	//}

	return true;
}

void vkx::MeshLoader::loadMeshes(const aiScene *pScene) {


	// init each entry with mesh data
	for (unsigned int index = 0; index < m_Entries.size(); ++index) {

		// pointer to mesh
		const aiMesh *pMesh = pScene->mMeshes[index];

		// reference to corresponding mesh entry
		MeshEntry &meshEntry = m_Entries[index];


		// set material index for this mesh
		m_Entries[index].MaterialIndex = pMesh->mMaterialIndex;


		// get the color of this mesh's material
		// is this just a hack? probably // todo: remove this
		aiColor3D pColor(0.f, 0.f, 0.f);
		pScene->mMaterials[pMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);




		aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

		// get vertices

		for (unsigned int i = 0; i < pMesh->mNumVertices; i++) {
			aiVector3D* pPos = &(pMesh->mVertices[i]);
			aiVector3D* pNormal = &(pMesh->mNormals[i]);
			//aiVector3D *pTexCoord;
			//if (paiMesh->HasTextureCoords(0)) {
			//	pTexCoord = &(paiMesh->mTextureCoords[0][i]);
			//} else {
			//	pTexCoord = &Zero3D;
			//}
			aiVector3D *pTexCoord = (pMesh->HasTextureCoords(0)) ? &(pMesh->mTextureCoords[0][i]) : &Zero3D;

			aiVector3D* pTangent = (pMesh->HasTangentsAndBitangents()) ? &(pMesh->mTangents[i]) : &Zero3D;
			aiVector3D* pBiTangent = (pMesh->HasTangentsAndBitangents()) ? &(pMesh->mBitangents[i]) : &Zero3D;

			Vertex v(
				glm::vec3(pPos->x, pPos->y, pPos->z),
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

		// get indices

		for (unsigned int i = 0; i < pMesh->mNumFaces; i++) {
			const aiFace &Face = pMesh->mFaces[i];
			if (Face.mNumIndices != 3) {
				continue;
			}
			m_Entries[index].Indices.push_back(Face.mIndices[0]);
			m_Entries[index].Indices.push_back(Face.mIndices[1]);
			m_Entries[index].Indices.push_back(Face.mIndices[2]);
		}


	}

}

//void vkx::MeshLoader::InitMesh(unsigned int index, const aiMesh *paiMesh, const aiScene *pScene) {
//	
//	m_Entries[index].MaterialIndex = paiMesh->mMaterialIndex;
//
//	aiColor3D pColor(0.f, 0.f, 0.f);
//	pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);
//
//
//
//
//
//
//
//	aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
//
//	for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
//		aiVector3D* pPos = &(paiMesh->mVertices[i]);
//		aiVector3D* pNormal = &(paiMesh->mNormals[i]);
//		//aiVector3D *pTexCoord;
//		//if (paiMesh->HasTextureCoords(0)) {
//		//	pTexCoord = &(paiMesh->mTextureCoords[0][i]);
//		//} else {
//		//	pTexCoord = &Zero3D;
//		//}
//		aiVector3D *pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;
//
//		aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[i]) : &Zero3D;
//		aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[i]) : &Zero3D;
//
//		Vertex v(
//			glm::vec3(pPos->x, pPos->y, pPos->z),
//			glm::vec2(pTexCoord->x, pTexCoord->y),
//			glm::vec3(pNormal->x, pNormal->y, pNormal->z),
//			glm::vec3(pTangent->x, pTangent->y, pTangent->z),
//			glm::vec3(pBiTangent->x, pBiTangent->y, pBiTangent->z),
//			glm::vec3(pColor.r, pColor.g, pColor.b)
//		);
//
//		dim.max.x = fmax(pPos->x, dim.max.x);
//		dim.max.y = fmax(pPos->y, dim.max.y);
//		dim.max.z = fmax(pPos->z, dim.max.z);
//
//		dim.min.x = fmin(pPos->x, dim.min.x);
//		dim.min.y = fmin(pPos->y, dim.min.y);
//		dim.min.z = fmin(pPos->z, dim.min.z);
//
//		m_Entries[index].Vertices.push_back(v);
//	}
//
//	dim.size = dim.max - dim.min;
//
//	// get indices
//
//	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
//		const aiFace &Face = paiMesh->mFaces[i];
//		if (Face.mNumIndices != 3) {
//			continue;
//		}
//		m_Entries[index].Indices.push_back(Face.mIndices[0]);
//		m_Entries[index].Indices.push_back(Face.mIndices[1]);
//		m_Entries[index].Indices.push_back(Face.mIndices[2]);
//	}
//
//
//
//}




// Create vertex and index buffer with given layout
// Note : Only does staging if a valid command buffer and transfer queue are passed

//vkx::MeshBuffer vkx::MeshLoader::createBuffers(const Context &context, const std::vector<VertexLayout> &layout, float scale) {
//
//	std::vector<float> vertexBuffer;
//	for (int m = 0; m < m_Entries.size(); m++) {
//		for (int i = 0; i < m_Entries[m].Vertices.size(); i++) {
//			// Push vertex data depending on layout
//			for (auto& layoutDetail : layout) {
//				// Position
//				if (layoutDetail == VERTEX_LAYOUT_POSITION) {
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.x * scale);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.y * scale);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.z * scale);
//				}
//				// Normal
//				if (layoutDetail == VERTEX_LAYOUT_NORMAL) {
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.x);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.y);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.z);
//				}
//				// Texture coordinates
//				if (layoutDetail == VERTEX_LAYOUT_UV) {
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.s);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.t);
//				}
//				// Color
//				if (layoutDetail == VERTEX_LAYOUT_COLOR) {
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.r);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.g);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.b);
//				}
//				// Tangent
//				if (layoutDetail == VERTEX_LAYOUT_TANGENT) {
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.x);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.y);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.z);
//				}
//				// Bitangent
//				if (layoutDetail == VERTEX_LAYOUT_BITANGENT) {
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.x);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.y);
//					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.z);
//				}
//				// Dummy layout components for padding
//				if (layoutDetail == VERTEX_LAYOUT_DUMMY_FLOAT) {
//					vertexBuffer.push_back(0.0f);
//				}
//				if (layoutDetail == VERTEX_LAYOUT_DUMMY_VEC4) {
//					vertexBuffer.push_back(0.0f);
//					vertexBuffer.push_back(0.0f);
//					vertexBuffer.push_back(0.0f);
//					vertexBuffer.push_back(0.0f);
//				}
//			}
//		}
//	}
//
//	MeshBuffer meshBuffer;
//	meshBuffer.vertices.size = vertexBuffer.size() * sizeof(float);
//
//	dim.min *= scale;
//	dim.max *= scale;
//	dim.size *= scale;
//
//	std::vector<uint32_t> indexBuffer;
//	for (uint32_t m = 0; m < m_Entries.size(); m++) {
//		uint32_t indexBase = (uint32_t)indexBuffer.size();
//		for (uint32_t i = 0; i < m_Entries[m].Indices.size(); i++) {
//			indexBuffer.push_back(m_Entries[m].Indices[i] + indexBase);
//		}
//	}
//
//	meshBuffer.indexCount = (uint32_t)indexBuffer.size();
//	// Use staging buffer to move vertex and index buffer to device local memory
//	// Vertex buffer
//	meshBuffer.vertices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
//	// Index buffer
//	meshBuffer.indices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
//	meshBuffer.dim = dim.size;
//
//	this->combinedBuffer = meshBuffer;
//
//	return meshBuffer;
//}




void vkx::MeshLoader::createMeshBuffer(const Context &context, const std::vector<VertexLayout> &layout, float scale) {
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
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.y);
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
	
	this->combinedBuffer = meshBuffer;
}



void vkx::MeshLoader::createMeshBuffers(const Context &context, const std::vector<VertexLayout> &layout, float scale) {

	for (int m = 0; m < m_Entries.size(); m++) {


		std::vector<float> vertexBuffer;

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
					vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.y);// y was negative// important
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

		MeshBuffer meshBuffer;
		meshBuffer.vertices.size = vertexBuffer.size() * sizeof(float);

		dim.min *= scale;
		dim.max *= scale;
		dim.size *= scale;

		std::vector<uint32_t> indexBuffer;
		uint32_t indexBase = (uint32_t)indexBuffer.size();
		for (uint32_t i = 0; i < m_Entries[m].Indices.size(); i++) {
			indexBuffer.push_back(m_Entries[m].Indices[i] + indexBase);
		}

		meshBuffer.indexCount = (uint32_t)indexBuffer.size();
		// Use staging buffer to move vertex and index buffer to device local memory
		// Vertex buffer
		meshBuffer.vertices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
		// Index buffer
		meshBuffer.indices = context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
		meshBuffer.dim = dim.size;

		meshBuffer.materialIndex = m_Entries[m].MaterialIndex;



		// set pointer to material used by this mesh
		// todo: create an asset manager
		//meshBuffer.material = &materials[meshBuffer.materialIndex];

		meshBuffers.push_back(meshBuffer);
	}


}




































namespace vkx {

	//http://stackoverflow.com/questions/12927169/how-can-i-initialize-c-object-member-variables-in-the-constructor
	//http://stackoverflow.com/questions/14169584/passing-and-storing-a-const-reference-via-a-constructor

	// don't ever use this constructor
	//Mesh::Mesh():
	//	context(vkx::Context())
	//{
	//	//this->context = nullptr;
	//	//this->meshLoader = new vkx::MeshLoader();
	//}

	Mesh::Mesh() {

	}

	Mesh::Mesh(vkx::MeshBuffer meshBuffer, Material material) {
		this->meshBuffer = meshBuffer;
		this->material = &material;
	}

	// pointer way:
	//Mesh::Mesh(const vkx::Context &context) {
	//	this->context = &context;
	//	this->meshLoader = new vkx::MeshLoader(context);
	//}

	// reference way:
	//Mesh::Mesh(const vkx::Context &context):
	//	context(context)
	//{
	//	this->meshLoader = new vkx::MeshLoader(context);
	//}

	//void Mesh::load(const std::string &filename) {
	//	this->meshLoader->load(filename);
	//}

	//void Mesh::load(const std::string &filename, int flags) {
	//	this->meshLoader->load(filename, flags);
	//}

	//void Mesh::createBuffers(const std::vector<VertexLayout> &layout, float scale, uint32_t binding) {
	//	this->meshBuffer = this->meshLoader->createBuffers(this->context, layout, scale);
	//	//this->materials = this->meshLoader->materials;

	//	//this->attributeDescriptions = this->meshLoader->attributeDescriptions;

	//	this->vertexBufferBinding = binding;// important
	//	this->setupVertexInputState(layout);// doesn't seem to be necessary/used

	//	//this->bindingDescription = this->meshLoader->bindingDescriptions[0];// ?
	//	//this->pipeline = this->meshLoader->pipeline;// not needed?
	//}

	//void Mesh::createPartBuffers(const std::vector<VertexLayout> &layout, float scale, uint32_t binding) {
	//	this->partBuffers = this->meshLoader->createPartBuffers(this->context, layout, scale);
	//	//this->materials = this->meshLoader->materials;

	//	this->attributeDescriptions = this->meshLoader->attributeDescriptions;

	//	this->vertexBufferBinding = binding;// important
	//	this->setupVertexInputState(layout);// doesn't seem to be necessary/used

	//	//this->bindingDescription = this->meshLoader->bindingDescriptions[0];// ?
	//	//this->pipeline = this->meshLoader->pipeline;// not needed?
	//}



	//void Mesh::setupVertexInputState(const std::vector<VertexLayout> &layout) {
	//	bindingDescription = vertexInputBindingDescription(
	//		vertexBufferBinding,
	//		vertexSize(layout),
	//		vk::VertexInputRate::eVertex);

	//	attributeDescriptions.clear();
	//	uint32_t offset = 0;
	//	uint32_t binding = 0;
	//	for (auto& layoutDetail : layout) {
	//		// vk::Format (layout)
	//		vk::Format format = (layoutDetail == VERTEX_LAYOUT_UV) ? vk::Format::eR32G32Sfloat : vk::Format::eR32G32B32Sfloat;

	//		attributeDescriptions.push_back(
	//			vertexInputAttributeDescription(
	//				vertexBufferBinding,
	//				binding,
	//				format,
	//				offset));

	//		// Offset
	//		offset += (layoutDetail == VERTEX_LAYOUT_UV) ? (2 * sizeof(float)) : (3 * sizeof(float));
	//		binding++;
	//	}

	//	vertexInputState = vk::PipelineVertexInputStateCreateInfo();
	//	vertexInputState.vertexBindingDescriptionCount = 1;
	//	vertexInputState.pVertexBindingDescriptions = &bindingDescription;
	//	vertexInputState.vertexAttributeDescriptionCount = attributeDescriptions.size();
	//	vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
	//}

	//void Mesh::drawIndexed(const vk::CommandBuffer & cmdBuffer) {

	//	// todo: add more
	//	if (pipeline) {
	//		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	//	}
	//	if ((pipelineLayout) && (descriptorSet)) {
	//		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
	//	}







	//	cmdBuffer.bindVertexBuffers(vertexBufferBinding, meshBuffer.vertices.buffer, vk::DeviceSize());
	//	cmdBuffer.bindIndexBuffer(meshBuffer.indices.buffer, 0, vk::IndexType::eUint32);
	//	cmdBuffer.drawIndexed(meshBuffer.indexCount, 1, 0, 0, 0);

	//}

}









namespace vkx {

	//http://stackoverflow.com/questions/12927169/how-can-i-initialize-c-object-member-variables-in-the-constructor
	//http://stackoverflow.com/questions/14169584/passing-and-storing-a-const-reference-via-a-constructor

	// constructors

	// don't ever use this constructor
	Model::Model() :
		context(vkx::Context())
	{
		//this->context = nullptr;
		//this->meshLoader = new vkx::MeshLoader();
	}

	// reference way:
	Model::Model(const vkx::Context &context):
		context(context)// init context with reference
	{
		this->meshLoader = new vkx::MeshLoader(context);
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
		this->materials = this->meshLoader->materials;

		for (int i = 0; i < meshBuffers.size(); ++i) {
			vkx::MeshBuffer &mBuffer = meshBuffers[i];
			vkx::Mesh m(mBuffer, this->materials[mBuffer.materialIndex]);
			
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


	void Model::drawIndexed(const vk::CommandBuffer & cmdBuffer) {

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