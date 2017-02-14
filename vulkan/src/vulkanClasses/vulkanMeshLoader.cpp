#include "vulkanMeshLoader.h"

namespace vkx {

	vkx::MeshLoader::MeshLoader(vkx::Context *context, vkx::AssetManager *assetManager) {

		this->context = context;
		this->assetManager = assetManager;
		this->textureLoader = new vkx::TextureLoader(*context);
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



		for (size_t i = 0; i < pScene->mNumMaterials; i++) {

			Material material;

			aiString name;
			pScene->mMaterials[i]->Get(AI_MATKEY_NAME, name);

			material.name = name.C_Str();
			std::string ls = "Info: Material: \"" + material.name + "\"\n";
			printf(ls.c_str());

			// if a material with the same name has already been loaded, continue
			if (this->assetManager->materials.doExist(material.name)) {
				// skip this material
				continue;
			}

			// Properties
			aiColor4D color;
			pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, color);
			material.properties.ambient = glm::make_vec4(&color.r) + glm::vec4(0.1f);

			pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			material.properties.diffuse = glm::make_vec4(&color.r);

			pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, color);
			material.properties.specular = glm::make_vec4(&color.r);

			pScene->mMaterials[i]->Get(AI_MATKEY_OPACITY, material.properties.opacity);

			if ((material.properties.opacity) > 0.0f) {
				material.properties.specular = glm::vec4(0.0f);
			}



			// Textures
			aiString texturefile;
			std::string assetPath = getAssetPath() + "models/";

			// get diffuse texture
			pScene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texturefile);
			if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) > 0) {

				
				std::string fileName = std::string(texturefile.C_Str());
				std::replace(fileName.begin(), fileName.end(), '\\', '/');

				//std::cout << "  Diffuse: \"" << texturefile.C_Str() << "\"" << std::endl;
				std::string ls = "Info: Diffuse: \"" + std::string(texturefile.C_Str()) + "\"\n";
				printf(ls.c_str());

				// if the texture has already been loaded previously
				// use it instead of loading it again
				if (this->assetManager->textures.doExist(fileName)) {
					// get from memory
					material.diffuse = assetManager->textures.get(fileName);
				} else {
					// load from file
					vkx::Texture tex = textureLoader->loadTexture(assetPath + fileName, vk::Format::eBc3UnormBlock);// this is the texture format! :O
					this->assetManager->textures.add(fileName, tex);
					material.diffuse = tex;
				}
			} else {
				std::string fileName = std::string(texturefile.C_Str());
				printf("Error: Material has no diffuse, using dummy texture!\n");
				// todo : separate pipeline and layout
				material.diffuse = textureLoader->loadTexture(assetPath + "kamen.ktx", vk::Format::eBc2UnormBlock);
			}



			// get specular texture
			pScene->mMaterials[i]->GetTexture(aiTextureType_SPECULAR, 0, &texturefile);
			if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_SPECULAR) > 0) {
				
				material.hasSpecular = true;

				std::string fileName = std::string(texturefile.C_Str());
				std::replace(fileName.begin(), fileName.end(), '\\', '/');

				

				std::string ls = "Info: Specular: \"" + std::string(texturefile.C_Str()) + "\"\n";
				printf(ls.c_str());

				// if the texture has already been loaded previously
				// use it instead of loading it again
				if (this->assetManager->textures.doExist(fileName)) {
					// get from memory
					material.specular = assetManager->textures.get(fileName);
				} else {
					// load from file
					vkx::Texture tex = textureLoader->loadTexture(assetPath + fileName, vk::Format::eBc3UnormBlock);// this is the texture format! :O
					this->assetManager->textures.add(fileName, tex);
					material.specular = tex;
				}
			} else {
				std::string fileName = std::string(texturefile.C_Str());
				printf("Error: Material has no specular, using dummy texture!\n");
				// todo : separate pipeline and layout
				material.specular = textureLoader->loadTexture(assetPath + "dummy.ktx", vk::Format::eBc2UnormBlock);
			}



			// get bump map
			pScene->mMaterials[i]->GetTexture(aiTextureType_NORMALS, 0, &texturefile);
			if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_NORMALS) > 0) {

				material.hasBump = true;

				std::string fileName = std::string(texturefile.C_Str());
				std::replace(fileName.begin(), fileName.end(), '\\', '/');

				std::string ls = "Info: Bump: \"" + std::string(texturefile.C_Str()) + "\"\n";
				printf(ls.c_str());

				

				// if the texture has already been loaded previously
				// use it instead of loading it again
				if (this->assetManager->textures.doExist(fileName)) {
					// get from memory
					material.bump = assetManager->textures.get(fileName);
				} else {
					// load from file
					vkx::Texture tex = textureLoader->loadTexture(assetPath + fileName, vk::Format::eBc3UnormBlock);// this is the texture format! :O
					this->assetManager->textures.add(fileName, tex);
					material.bump = tex;
				}
			} else {
				std::string fileName = std::string(texturefile.C_Str());
				printf("Error: Material has no bump, using dummy texture!\n");
				// todo : separate pipeline and layout
				material.bump = textureLoader->loadTexture(assetPath + "dummy.ktx", vk::Format::eBc2UnormBlock);
			}

			// Mask
			if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_OPACITY) > 0) {
				printf("Info: Material has opacity, enabling alpha test.\n");
				material.hasAlpha = true;
			}


			if (this->assetManager->materialDescriptorPool == nullptr) {
				return;
			}

			if (this->assetManager->materialDescriptorSetLayout == nullptr) {
				return;
			}


			vk::DescriptorSetAllocateInfo allocInfo =
				vkx::descriptorSetAllocateInfo(
					*this->assetManager->materialDescriptorPool,
					this->assetManager->materialDescriptorSetLayout,
					1);

			material.descriptorSet = context->device.allocateDescriptorSets(allocInfo)[0];


			std::vector<vk::WriteDescriptorSet> writeDescriptorSets =
			{
				// image bindings
				// binding 0: diffuse
				vkx::writeDescriptorSet(
					material.descriptorSet,
					vk::DescriptorType::eCombinedImageSampler,
					0,
					&material.diffuse.descriptor),
				//// binding 1: specular
				//vkx::writeDescriptorSet(
				//	material.descriptorSet,
				//	vk::DescriptorType::eCombinedImageSampler,
				//	1,
				//	&material.specular.descriptor),
				//// binding 2: normal
				//vkx::writeDescriptorSet(
				//	material.descriptorSet,
				//	vk::DescriptorType::eCombinedImageSampler,
				//	2,
				//	&material.bump.descriptor)
			};
			// need to update shaders to reflect new bindings



			///*context.*/device.updateDescriptorSets(writeDescriptorSets, {});

			context->device.updateDescriptorSets(writeDescriptorSets, {});

			this->assetManager->materials.add(material.name, material);

			//this->assetManager->loadedMaterials.push_back(tempMaterials[i]);

		}

	}





	void vkx::MeshLoader::loadMeshes(const aiScene *pScene) {

		// init each entry with mesh data
		for (unsigned int index = 0; index < m_Entries.size(); ++index) {


			// reference to corresponding mesh entry
			MeshEntry &meshEntry = m_Entries[index];

			// pointer to corresponding mesh
			const aiMesh *pMesh = pScene->mMeshes[index];







			// set material name for this mesh
			//int materialIndex = this->assetManager->loadedMaterials.size() - pScene->mNumMaterials + pMesh->mMaterialIndex;
			//m_Entries[index].MaterialIndex = materialIndex;

			aiString name;
			pScene->mMaterials[pMesh->mMaterialIndex]->Get(AI_MATKEY_NAME, name);

			m_Entries[index].materialName = name.C_Str();


			// get the color of this mesh's material
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



	bool vkx::MeshLoader::parse(const aiScene *pScene, const std::string &Filename) {

		// Counters
		for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
			MeshEntry mEntry;
			mEntry.vertexBase = numVertices;
			m_Entries.push_back(mEntry);

			numVertices += pScene->mMeshes[i]->mNumVertices;// total for all vertices
		}



		loadMaterials(pScene);
		loadMeshes(pScene);

		return true;
	}


	void vkx::MeshLoader::createMeshBuffer(const std::vector<VertexLayout> &layout, float scale) {

		// combined mesh buffer

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
		meshBuffer.vertices = context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
		// Index buffer
		meshBuffer.indices = context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
		meshBuffer.dim = dim.size;

		this->combinedBuffer = meshBuffer;
	}



	void vkx::MeshLoader::createMeshBuffers(const std::vector<VertexLayout> &layout, float scale) {

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
			meshBuffer.vertices = this->context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
			// Index buffer
			meshBuffer.indices = this->context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
			meshBuffer.dim = dim.size;

			meshBuffer.materialIndex = m_Entries[m].materialIndex;

			meshBuffer.materialName = m_Entries[m].materialName;


			meshBuffers.push_back(meshBuffer);
		}


	}







































	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ SKINNED MESH  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */




	void vkx::MeshLoader::createSkinnedMeshBuffer(const std::vector<VertexLayout> &layout, float scale) {


		this->setAnimation(0);


		// Setup bones
		// One vertex bone info structure per vertex
		this->boneData.bones.resize(numVertices);

		// Store global inverse transform matrix of root node 
		this->boneData.globalInverseTransform = pScene->mRootNode->mTransformation;
		this->boneData.globalInverseTransform.Inverse();

		aiMatrix4x4 scaleMatrix;
		aiMatrix4x4::Scaling(aiVector3D(scale, scale, scale), scaleMatrix);

		this->boneData.globalInverseTransform = this->boneData.globalInverseTransform * scaleMatrix;


		// Load bones (weights and IDs)
		for (uint32_t m = 0; m < m_Entries.size(); m++) {
			aiMesh *paiMesh = pScene->mMeshes[m];
			if (paiMesh->mNumBones > 0) {
				this->loadBones(m, paiMesh, this->boneData.bones/*, scale*/);
			}
		}

		// Generate vertex buffer
		std::vector<skinnedMeshVertex> vertexBuffer;
		// Iterate through all meshes in the file
		// and extract the vertex information used in this demo
		for (uint32_t m = 0; m < m_Entries.size(); m++) {
			for (uint32_t i = 0; i < m_Entries[m].Vertices.size(); i++) {
				skinnedMeshVertex vertex;

				vertex.pos = m_Entries[m].Vertices[i].m_pos;//*scale
				vertex.normal = m_Entries[m].Vertices[i].m_normal;
				vertex.uv = m_Entries[m].Vertices[i].m_tex;
				vertex.color = m_Entries[m].Vertices[i].m_color;

				// Fetch bone weights and IDs
				for (uint32_t j = 0; j < MAX_BONES_PER_VERTEX; j++) {
					vertex.boneWeights[j] = this->boneData.bones[m_Entries[m].vertexBase + i].weights[j];
					vertex.boneIDs[j] = this->boneData.bones[m_Entries[m].vertexBase + i].IDs[j];
				}

				vertexBuffer.push_back(vertex);
			}
		}
		uint32_t vertexBufferSize = vertexBuffer.size() * vkx::vertexSize(layout);

		// Generate index buffer from loaded mesh file
		std::vector<uint32_t> indexBuffer;
		for (uint32_t m = 0; m < m_Entries.size(); m++) {
			uint32_t indexBase = indexBuffer.size();
			for (uint32_t i = 0; i < m_Entries[m].Indices.size(); i++) {
				indexBuffer.push_back(m_Entries[m].Indices[i] + indexBase);
			}
		}
		uint32_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
		this->combinedBuffer.indexCount = indexBuffer.size();
		this->combinedBuffer.vertices = context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
		this->combinedBuffer.indices = context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);

		this->combinedBuffer.materialIndex = m_Entries[0].materialIndex;
		this->combinedBuffer.materialName = m_Entries[0].materialName;
	}





















	// Set active animation by index
	void vkx::MeshLoader::setAnimation(uint32_t animationIndex) {
		assert(animationIndex < pScene->mNumAnimations);
		boneData.pAnimation = pScene->mAnimations[animationIndex];
	}

	// Load bone information from ASSIMP mesh
	void vkx::MeshLoader::loadBones(uint32_t meshIndex, const aiMesh *pMesh, std::vector<VertexBoneData> &Bones/*, float scale*/) {
		for (uint32_t i = 0; i < pMesh->mNumBones; i++) {
			uint32_t index = 0;

			assert(pMesh->mNumBones <= MAX_BONES);

			std::string name(pMesh->mBones[i]->mName.data);

			if (boneData.boneMapping.find(name) == boneData.boneMapping.end()) {
				// Bone not present, add new one
				index = boneData.numBones;
				boneData.numBones++;
				BoneInfo bone;
				boneData.boneInfo.push_back(bone);

				boneData.boneInfo[index].offset = pMesh->mBones[i]->mOffsetMatrix;

				boneData.boneMapping[name] = index;

			} else {
				index = boneData.boneMapping[name];
			}

			for (uint32_t j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {
				uint32_t vertexID = m_Entries[meshIndex].vertexBase + pMesh->mBones[i]->mWeights[j].mVertexId;
				Bones[vertexID].add(index, pMesh->mBones[i]->mWeights[j].mWeight);
			}
		}
		boneData.boneTransforms.resize(boneData.numBones);
	}

	// Recursive bone transformation for given animation time
	void vkx::MeshLoader::update(float time) {
		float TicksPerSecond = (float)(pScene->mAnimations[0]->mTicksPerSecond != 0 ? pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
		float TimeInTicks = time * TicksPerSecond;
		float AnimationTime = fmod(TimeInTicks, (float)pScene->mAnimations[0]->mDuration);

		aiMatrix4x4 identity = aiMatrix4x4();
		readNodeHierarchy(AnimationTime, pScene->mRootNode, identity);

		for (uint32_t i = 0; i < boneData.boneTransforms.size(); i++) {
			boneData.boneTransforms[i] = boneData.boneInfo[i].finalTransformation;
		}

	}



	// Find animation for a given node
	const aiNodeAnim* vkx::MeshLoader::findNodeAnim(const aiAnimation* animation, const std::string nodeName) {
		for (uint32_t i = 0; i < animation->mNumChannels; i++) {
			const aiNodeAnim* nodeAnim = animation->mChannels[i];
			if (std::string(nodeAnim->mNodeName.data) == nodeName) {
				return nodeAnim;
			}
		}
		return nullptr;
	}

	// Returns a 4x4 matrix with interpolated translation between current and next frame
	aiMatrix4x4 vkx::MeshLoader::interpolateTranslation(float time, const aiNodeAnim* pNodeAnim) {
		aiVector3D translation;

		if (pNodeAnim->mNumPositionKeys == 1) {
			translation = pNodeAnim->mPositionKeys[0].mValue;
		} else {
			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
				if (time < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
					frameIndex = i;
					break;
				}
			}

			aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
			aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

			const aiVector3D& start = currentFrame.mValue;
			const aiVector3D& end = nextFrame.mValue;

			translation = (start + delta * (end - start));
		}

		aiMatrix4x4 mat;
		aiMatrix4x4::Translation(translation, mat);
		return mat;
	}

	// Returns a 4x4 matrix with interpolated rotation between current and next frame
	aiMatrix4x4 vkx::MeshLoader::interpolateRotation(float time, const aiNodeAnim* pNodeAnim) {
		aiQuaternion rotation;

		if (pNodeAnim->mNumRotationKeys == 1) {
			rotation = pNodeAnim->mRotationKeys[0].mValue;
		} else {
			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
				if (time < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
					frameIndex = i;
					break;
				}
			}

			aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
			aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

			const aiQuaternion& start = currentFrame.mValue;
			const aiQuaternion& end = nextFrame.mValue;

			aiQuaternion::Interpolate(rotation, start, end, delta);
			rotation.Normalize();
		}

		aiMatrix4x4 mat(rotation.GetMatrix());
		return mat;
	}


	// Returns a 4x4 matrix with interpolated scaling between current and next frame
	aiMatrix4x4 vkx::MeshLoader::interpolateScale(float time, const aiNodeAnim* pNodeAnim) {
		aiVector3D scale;

		if (pNodeAnim->mNumScalingKeys == 1) {
			scale = pNodeAnim->mScalingKeys[0].mValue;
		} else {
			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
				if (time < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
					frameIndex = i;
					break;
				}
			}

			aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
			aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

			const aiVector3D& start = currentFrame.mValue;
			const aiVector3D& end = nextFrame.mValue;

			scale = (start + delta * (end - start));
		}

		aiMatrix4x4 mat;
		aiMatrix4x4::Scaling(scale, mat);
		return mat;
	}






	// Get node hierarchy for current animation time
	void vkx::MeshLoader::readNodeHierarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform) {
		std::string NodeName(pNode->mName.data);

		aiMatrix4x4 NodeTransformation(pNode->mTransformation);

		const aiNodeAnim* pNodeAnim = findNodeAnim(boneData.pAnimation, NodeName);

		if (pNodeAnim) {
			// Get interpolated matrices between current and next frame
			aiMatrix4x4 matScale = interpolateScale(AnimationTime, pNodeAnim);
			aiMatrix4x4 matRotation = interpolateRotation(AnimationTime, pNodeAnim);
			aiMatrix4x4 matTranslation = interpolateTranslation(AnimationTime, pNodeAnim);

			NodeTransformation = matTranslation * matRotation * matScale;
		}

		aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

		if (boneData.boneMapping.find(NodeName) != boneData.boneMapping.end()) {
			uint32_t BoneIndex = boneData.boneMapping[NodeName];
			boneData.boneInfo[BoneIndex].finalTransformation = boneData.globalInverseTransform * GlobalTransformation * boneData.boneInfo[BoneIndex].offset;
		}

		for (uint32_t i = 0; i < pNode->mNumChildren; i++) {
			readNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
		}
	}





















}