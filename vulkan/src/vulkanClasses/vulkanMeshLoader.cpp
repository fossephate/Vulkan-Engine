#include "vulkanMeshLoader.h"



//vkx::MeshLoader::MeshLoader() {
//	this->textureLoader = nullptr;
//}

//vkx::MeshLoader::MeshLoader(const vkx::Context &context) {
//	this->textureLoader = new vkx::TextureLoader(context);
//}


vkx::MeshLoader::MeshLoader(const vkx::Context & context, vkx::AssetManager & assetManager) :
	assetManager(assetManager)
{
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

	tempMaterials.resize(pScene->mNumMaterials);

	for (size_t i = 0; i < pScene->mNumMaterials; i++) {

		Material material;

		aiString name;
		pScene->mMaterials[i]->Get(AI_MATKEY_NAME, name);

		// Properties
		aiColor4D color;
		pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, color);
		material.properties.ambient = glm::make_vec4(&color.r) + glm::vec4(0.1f);

		pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		material.properties.diffuse = glm::make_vec4(&color.r);

		pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, color);
		material.properties.specular = glm::make_vec4(&color.r);

		pScene->mMaterials[i]->Get(AI_MATKEY_OPACITY, tempMaterials[i].properties.opacity);

		if ((material.properties.opacity) > 0.0f) {
			material.properties.specular = glm::vec4(0.0f);
		}

		material.name = name.C_Str();
		std::cout << "Material \"" << material.name << "\"" << std::endl;

		// Textures
		aiString texturefile;
		std::string assetPath = getAssetPath()+ "models/";

		// Diffuse
		pScene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texturefile);
		if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			std::cout << "  Diffuse: \"" << texturefile.C_Str() << "\"" << std::endl;
			std::string fileName = std::string(texturefile.C_Str());
			std::replace(fileName.begin(), fileName.end(), '\\', '/');
			material.diffuse = textureLoader->loadTexture(assetPath + fileName, vk::Format::eBc3UnormBlock);
		} else {

			std::cout << "  Material has no diffuse, using dummy texture!" << std::endl;
			// todo : separate pipeline and layout
			material.diffuse = textureLoader->loadTexture(assetPath + "dummy.ktx", vk::Format::eBc2UnormBlock);
		}

		// For scenes with multiple textures per material we would need to check for additional texture types, e.g.:
		// aiTextureType_HEIGHT, aiTextureType_OPACITY, aiTextureType_SPECULAR, etc.

		// Assign pipeline
		//materials[i].pipeline = (materials[i].properties.opacity == 0.0f) ? &pipelines.solid : &pipelines.blending;

		tempMaterials.push_back(material);

		// add materials to global materials vector
		//globalMaterials.push_back(material);
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
	//m_Entries.resize(pScene->mNumMeshes);

	// Counters
	for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
		MeshEntry mEntry;
		mEntry.vertexBase = numVertices;
		m_Entries.push_back(mEntry);

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

	for (int i = 0; i < tempMaterials.size(); ++i) {
		//this->assetManager.materials.push_back(materials[i]);
		Material m = tempMaterials[i];
		this->assetManager.loadedMaterials.push_back(m);
	}


	// init each entry with mesh data
	for (unsigned int index = 0; index < m_Entries.size(); ++index) {

		// pointer to mesh
		const aiMesh *pMesh = pScene->mMeshes[index];

		// reference to corresponding mesh entry
		MeshEntry &meshEntry = m_Entries[index];


		// set material index for this mesh

		int materialIndex = this->assetManager.loadedMaterials.size() - pScene->mNumMaterials + pMesh->mMaterialIndex;
		m_Entries[index].MaterialIndex = materialIndex;


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














































