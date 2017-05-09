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

		this->filename = filename;



		// use asset manager
		if (this->assetManager->scenes.present(filename)) {
			pScene = this->assetManager->scenes.get(filename);
		} else {
			//pScene = Importer.ReadFile(filename.c_str(), flags);
			Importer.ReadFile(filename.c_str(), flags);
			pScene = Importer.GetOrphanedScene();
			this->assetManager->scenes.add(filename, pScene);
		}





		#endif

		if (!pScene) {
			throw std::runtime_error("Unable to parse " + filename);
		}
		return parse(pScene, filename);
	}



	void vkx::MeshLoader::loadMaterials(const aiScene *pScene) {

		// todo:
		// Add dummy textures for objects without texture
		//resources.textures->addTexture2D("dummy.diffuse", assetPath + "sponza/dummy.dds", VK_FORMAT_BC2_UNORM_BLOCK);
		//resources.textures->addTexture2D("dummy.specular", assetPath + "sponza/dummy_specular.dds", VK_FORMAT_BC2_UNORM_BLOCK);
		//resources.textures->addTexture2D("dummy.bump", assetPath + "sponza/dummy_ddn.dds", VK_FORMAT_BC2_UNORM_BLOCK);



		for (size_t i = 0; i < pScene->mNumMaterials; i++) {

			Material material;

			// get name
			aiString name;
			pScene->mMaterials[i]->Get(AI_MATKEY_NAME, name);
			// set name
			material.name = name.C_Str();

			// if a material with the same name has already been loaded, continue
			if (this->assetManager->materials.present(material.name)) {
				// skip this material
				continue;
			}

			material.name = name.C_Str();
			std::string ls = "Info: Material: \"" + material.name + "\"\n";
			printf(ls.c_str());


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

				material.hasDiffuse = true;
				
				std::string fileName = std::string(texturefile.C_Str());
				std::replace(fileName.begin(), fileName.end(), '\\', '/');

				//std::cout << "  Diffuse: \"" << texturefile.C_Str() << "\"" << std::endl;
				std::string ls = "Info: Diffuse: \"" + std::string(texturefile.C_Str()) + "\"\n";
				printf(ls.c_str());

				// if the texture hasn't been loaded already, load it
				if (!this->assetManager->textures.present(fileName)) {
					// load from file
					vkx::Texture tex = textureLoader->loadTexture(assetPath + fileName, vk::Format::eBc2UnormBlock);
					this->assetManager->textures.add(fileName, tex);
				}
				material.diffuse = assetManager->textures.getSharedPtr(fileName);
			} else {
				std::string fileName = std::string(texturefile.C_Str());
				printf("Error: Material has no diffuse, using dummy texture!\n");
				
				//material.diffuse = textureLoader->loadTexture(assetPath + "dummy/dummy.dds", vk::Format::eBc2UnormBlock);
				material.diffuse = assetManager->textures.getOrLoad(assetPath + "dummy/dummy.dds", textureLoader);
			}



			// get specular texture
			pScene->mMaterials[i]->GetTexture(aiTextureType_SPECULAR, 0, &texturefile);
			if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_SPECULAR) > 0) {
				
				material.hasSpecular = true;

				std::string fileName = std::string(texturefile.C_Str());
				std::replace(fileName.begin(), fileName.end(), '\\', '/');

				std::string ls = "Info: Specular: \"" + std::string(texturefile.C_Str()) + "\"\n";
				printf(ls.c_str());

				// if the texture hasn't been loaded already, load it
				if (!this->assetManager->textures.present(fileName)) {
					// load from file
					vkx::Texture tex = textureLoader->loadTexture(assetPath + fileName, vk::Format::eBc2UnormBlock);
					this->assetManager->textures.add(fileName, tex);
				}
				material.specular = assetManager->textures.getSharedPtr(fileName);
			} else {
				std::string fileName = std::string(texturefile.C_Str());
				printf("Error: Material has no specular, using dummy texture!\n");

				//material.specular = textureLoader->loadTexture(assetPath + "dummy/dummy_specular.dds", vk::Format::eBc2UnormBlock);
				material.specular = assetManager->textures.getOrLoad(assetPath + "dummy/dummy_specular.dds", textureLoader);
			}



			// get bump map
			pScene->mMaterials[i]->GetTexture(aiTextureType_NORMALS, 0, &texturefile);
			if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_NORMALS) > 0) {

				material.hasBump = true;

				std::string fileName = std::string(texturefile.C_Str());
				std::replace(fileName.begin(), fileName.end(), '\\', '/');

				std::string ls = "Info: Bump: \"" + std::string(texturefile.C_Str()) + "\"\n";
				printf(ls.c_str());

				// if the texture hasn't been loaded already, load it
				if (!this->assetManager->textures.present(fileName)) {
					// load from file
					vkx::Texture tex = textureLoader->loadTexture(assetPath + fileName, vk::Format::eBc2UnormBlock);
					this->assetManager->textures.add(fileName, tex);
				}
				material.bump = assetManager->textures.getSharedPtr(fileName);
			} else {
				std::string fileName = std::string(texturefile.C_Str());
				printf("Error: Material has no bump, using dummy texture!\n");

				//material.bump = textureLoader->loadTexture(assetPath + "dummy/dummy_ddn.dds", vk::Format::eBc2UnormBlock);
				material.bump = assetManager->textures.getOrLoad(assetPath + "dummy/dummy_ddn.dds", textureLoader);
			}

			// Mask
			if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_OPACITY) > 0) {
				printf("Info: Material has opacity, enabling alpha.\n");
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
					&material.diffuse->descriptor),
				// binding 1: specular
				vkx::writeDescriptorSet(
					material.descriptorSet,
					vk::DescriptorType::eCombinedImageSampler,
					1,
					&material.specular->descriptor),
				// binding 2: normal
				vkx::writeDescriptorSet(
					material.descriptorSet,
					vk::DescriptorType::eCombinedImageSampler,
					2,
					&material.bump->descriptor)
			};

			context->device.updateDescriptorSets(writeDescriptorSets, {});

			this->assetManager->materials.add(material.name, material);

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



	bool vkx::MeshLoader::parse(const aiScene *pScene, const std::string &filename) {

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


	void vkx::MeshLoader::createMeshBuffer(const std::vector<VertexComponent> &layout, float scale) {

		// combined mesh buffer

		std::vector<float> vertexBuffer;
		for (int m = 0; m < m_Entries.size(); m++) {
			for (int i = 0; i < m_Entries[m].Vertices.size(); i++) {
				// Push vertex data depending on layout
				for (auto& layoutDetail : layout) {
					// Position
					if (layoutDetail == VERTEX_COMPONENT_POSITION) {
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.x * scale);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.y * scale);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.z * scale);
					}
					// Normal
					if (layoutDetail == VERTEX_COMPONENT_NORMAL) {
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.x);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.y);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.z);
					}
					// Texture coordinates
					if (layoutDetail == VERTEX_COMPONENT_UV) {
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.s);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.t);
					}
					// Color
					if (layoutDetail == VERTEX_COMPONENT_COLOR) {
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.r);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.g);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.b);
					}
					// Tangent
					if (layoutDetail == VERTEX_COMPONENT_TANGENT) {
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.x);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.y);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.z);
					}
					// Bitangent
					if (layoutDetail == VERTEX_COMPONENT_BITANGENT) {
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.x);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.y);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.z);
					}
					// Dummy layout components for padding
					if (layoutDetail == VERTEX_COMPONENT_DUMMY_FLOAT) {
						vertexBuffer.push_back(0.0f);
					}
					if (layoutDetail == VERTEX_COMPONENT_DUMMY_VEC4) {
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
					}
				}
			}
		}

		//MeshBuffer meshBuffer;
		std::shared_ptr<MeshBuffer> meshBuffer;
		meshBuffer->vertices.size = vertexBuffer.size() * sizeof(float);

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

		meshBuffer->indexCount = (uint32_t)indexBuffer.size();
		// Use staging buffer to move vertex and index buffer to device local memory
		// Vertex buffer
		meshBuffer->vertices = context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
		// Index buffer
		meshBuffer->indices = context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
		meshBuffer->dim = dim.size;

		this->combinedBuffer = meshBuffer;
	}



	struct ComponentInfo {
		int offset;
		int length;
		glm::vec2 Vertex::* vec2p;
		glm::vec3 Vertex::* vec3p;
		glm::vec4 Vertex::* vec4p;
		VertexComponent vertexComponent;
	};

	std::vector<vkx::VertexComponent> defaultLayout =
	{
		vkx::VertexComponent::VERTEX_COMPONENT_POSITION,
		vkx::VertexComponent::VERTEX_COMPONENT_UV,
		vkx::VertexComponent::VERTEX_COMPONENT_COLOR,
		vkx::VertexComponent::VERTEX_COMPONENT_NORMAL,
		vkx::VertexComponent::VERTEX_COMPONENT_TANGENT,
		vkx::VertexComponent::VERTEX_COMPONENT_DUMMY_VEC4,
		vkx::VertexComponent::VERTEX_COMPONENT_DUMMY_VEC4
	};

	// this needs to be fixed:
	struct defaultVert {
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 color;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec4 dummy1;
		glm::vec4 dummy2;
	};


	//void vkx::MeshLoader::createMeshBuffers(const std::vector<VertexComponent> &layout, float scale) {

	//	auto tStart = std::chrono::high_resolution_clock::now();


	//	// todo: fix:
	//	//if (this->assetManager->meshBuffers.present(filename)) {
	//	//	// assumes correct vertex layout
	//	//	this->meshBuffers = this->assetManager->meshBuffers.get(filename);
	//	//	return;
	//	//}

	//	// for each mesh
	//	for (int m = 0; m < m_Entries.size(); m++) {




	//		std::vector<float> vertexBuffer;
	//		int numOfFloats = m_Entries[m].Vertices.size() * (vkx::vertexSize(layout) / sizeof(float));
	//		vertexBuffer.reserve(numOfFloats);


	//		if (layout == defaultLayout) {

	//			for (int i = 0; i < m_Entries[m].Vertices.size(); i++) {
	//				// pos
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.x * scale);
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.y * scale);
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.z * scale);
	//				// uv
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.x);
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.y);
	//				// color
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.r);
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.g);
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.b);
	//				// normal
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.x);
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.y);
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.z);
	//				// tangent
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.x);
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.y);
	//				vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.z);

	//				// dummy vec4
	//				vertexBuffer.push_back(0.0f);
	//				vertexBuffer.push_back(0.0f);
	//				vertexBuffer.push_back(0.0f);
	//				vertexBuffer.push_back(0.0f);

	//				// dummy vec4
	//				vertexBuffer.push_back(0.0f);
	//				vertexBuffer.push_back(0.0f);
	//				vertexBuffer.push_back(0.0f);
	//				vertexBuffer.push_back(0.0f);

	//			}

	//		} else {










	//			//std::vector<int Vertex::*> componentMap;
	//			//std::vector<int> componentMap;
	//			std::vector<ComponentInfo> componentMap;


	//			//auto a = offsetof(Vertex, m_pos);
	//			//int Foo::* pm = &Foo::b;

	//			for (auto &layoutDetail : layout) {
	//				ComponentInfo c;
	//				switch (layoutDetail) {
	//					// Position
	//					case VERTEX_COMPONENT_POSITION:
	//						//int Vertex::* pm = &Vertex::m_pos;
	//						//componentMap.push_back(offsetof(Vertex, m_pos));
	//						c.offset = offsetof(Vertex, m_pos);
	//						c.length = 3;
	//						c.vec3p = &Vertex::m_pos;
	//						componentMap.push_back(c);
	//						break;
	//						// Normal
	//					case VERTEX_COMPONENT_NORMAL:
	//						//componentMap.push_back(offsetof(Vertex, m_normal));
	//						c.offset = offsetof(Vertex, m_normal);
	//						c.length = 3;
	//						c.vec3p = &Vertex::m_normal;
	//						componentMap.push_back(c);
	//						break;
	//						// Texture Coordinates
	//					case VERTEX_COMPONENT_UV:
	//						//componentMap.push_back(offsetof(Vertex, m_tex));
	//						c.offset = offsetof(Vertex, m_tex);
	//						c.length = 2;
	//						c.vec2p = &Vertex::m_tex;
	//						componentMap.push_back(c);
	//						break;
	//						// Color
	//					case VERTEX_COMPONENT_COLOR:
	//						//componentMap.push_back(offsetof(Vertex, m_color));
	//						c.offset = offsetof(Vertex, m_color);
	//						c.length = 3;
	//						c.vec3p = &Vertex::m_color;
	//						componentMap.push_back(c);
	//						break;
	//						// Tangent
	//					case VERTEX_COMPONENT_TANGENT:
	//						//componentMap.push_back(offsetof(Vertex, m_tangent));
	//						c.offset = offsetof(Vertex, m_tangent);
	//						c.length = 3;
	//						c.vec3p = &Vertex::m_tangent;
	//						componentMap.push_back(c);
	//						break;
	//						// Bitangent
	//					case VERTEX_COMPONENT_BITANGENT:
	//						//componentMap.push_back(offsetof(Vertex, m_binormal));
	//						c.offset = offsetof(Vertex, m_binormal);
	//						c.length = 3;
	//						c.vec3p = &Vertex::m_binormal;
	//						componentMap.push_back(c);
	//						break;
	//						// Dummy layout components for padding
	//					case VERTEX_COMPONENT_DUMMY_FLOAT:
	//						//componentMap.push_back(-1);
	//						c.offset = -1;
	//						c.length = 1;
	//						c.vertexComponent = VERTEX_COMPONENT_DUMMY_FLOAT;
	//						componentMap.push_back(c);
	//						break;
	//					case VERTEX_COMPONENT_DUMMY_VEC4:
	//						//componentMap.push_back(-4);
	//						c.offset = -4;
	//						c.length = 4;
	//						c.vertexComponent = VERTEX_COMPONENT_DUMMY_VEC4;
	//						componentMap.push_back(c);
	//						break;
	//				}
	//			}

	//			// for each vertex
	//			for (int i = 0; i < m_Entries[m].Vertices.size(); i++) {
	//				// Push vertex data depending on layout



	//				for (auto &info : componentMap) {

	//					if (info.vertexComponent == VERTEX_COMPONENT_DUMMY_VEC4) {
	//						vertexBuffer.push_back(0.0f);
	//						vertexBuffer.push_back(0.0f);
	//						vertexBuffer.push_back(0.0f);
	//						vertexBuffer.push_back(0.0f);
	//					} else if (info.vertexComponent == VERTEX_COMPONENT_DUMMY_FLOAT) {
	//						vertexBuffer.push_back(0.0f);
	//					} else {

	//						Vertex *vert = &m_Entries[m].Vertices[i];

	//						if (info.length == 2) {
	//							glm::vec2 comp = *reinterpret_cast<glm::vec2 *>(reinterpret_cast<char *>(vert) + info.offset);
	//							vertexBuffer.push_back(comp.x);
	//							vertexBuffer.push_back(comp.y);
	//						} else if (info.length == 3) {
	//							glm::vec3 comp = *reinterpret_cast<glm::vec3 *>(reinterpret_cast<char *>(vert) + info.offset);
	//							vertexBuffer.push_back(comp.x);
	//							vertexBuffer.push_back(comp.y);
	//							vertexBuffer.push_back(comp.z);
	//						} else if (info.length == 4) {
	//							glm::vec4 comp = *reinterpret_cast<glm::vec4 *>(reinterpret_cast<char *>(vert) + info.offset);
	//							vertexBuffer.push_back(comp.x);
	//							vertexBuffer.push_back(comp.y);
	//							vertexBuffer.push_back(comp.z);
	//							vertexBuffer.push_back(comp.w);
	//						}


	//						//int value = *reinterpret_cast<int *>(reinterpret_cast<char *>(&o1) +offsetof(TestMem, a));
	//						//int value = *reinterpret_cast<int *>(reinterpret_cast<char *>(vert) + offset);

	//					}

	//				}


	//				//for (auto &layoutDetail : layout) {
	//				//	switch (layoutDetail) {
	//				//		// Position
	//				//		case VERTEX_COMPONENT_POSITION:
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.x * scale);
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.y * scale);
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.z * scale);
	//				//			break;
	//				//		// Normal
	//				//		case VERTEX_COMPONENT_NORMAL:
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.x);
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.y);// y was negative// important
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.z);
	//				//			break;
	//				//		// Texture Coordinates
	//				//		case VERTEX_COMPONENT_UV:
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.s);
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.t);
	//				//			break;
	//				//		// Color
	//				//		case VERTEX_COMPONENT_COLOR:
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.r);
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.g);
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.b);
	//				//			break;
	//				//		// Tangent
	//				//		case VERTEX_COMPONENT_TANGENT:
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.x);
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.y);
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.z);
	//				//			break;
	//				//		// Bitangent
	//				//		case VERTEX_COMPONENT_BITANGENT:
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.x);
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.y);
	//				//			vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.z);
	//				//			break;
	//				//		// Dummy layout components for padding
	//				//		case VERTEX_COMPONENT_DUMMY_FLOAT:
	//				//			vertexBuffer.push_back(0.0f);
	//				//			break;
	//				//		case VERTEX_COMPONENT_DUMMY_VEC4:
	//				//			vertexBuffer.push_back(0.0f);
	//				//			vertexBuffer.push_back(0.0f);
	//				//			vertexBuffer.push_back(0.0f);
	//				//			vertexBuffer.push_back(0.0f);
	//				//			break;
	//				//	}
	//				//}
	//			}

	//		}




	//		//std::shared_ptr<MeshBuffer> meshesDeferred;
	//		auto meshBuffer = std::make_shared<MeshBuffer>();

	//		

	//		//MeshBuffer meshBuffer;
	//		meshBuffer->vertexLayout = layout;

	//		meshBuffer->vertices.size = vertexBuffer.size() * sizeof(float);

	//		dim.min *= scale;
	//		dim.max *= scale;
	//		dim.size *= scale;

	//		std::vector<uint32_t> indexBuffer;
	//		uint32_t indexBase = (uint32_t)indexBuffer.size();
	//		for (uint32_t i = 0; i < m_Entries[m].Indices.size(); i++) {
	//			indexBuffer.push_back(m_Entries[m].Indices[i] + indexBase);
	//		}

	//		meshBuffer->indexCount = (uint32_t)indexBuffer.size();
	//		// Use staging buffer to move vertex and index buffer to device local memory
	//		// Vertex buffer
	//		meshBuffer->vertices = this->context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
	//		// Index buffer
	//		meshBuffer->indices = this->context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
	//		meshBuffer->dim = dim.size;

	//		meshBuffer->materialIndex = m_Entries[m].materialIndex;
	//		meshBuffer->materialName = m_Entries[m].materialName;


	//		meshBuffers.push_back(meshBuffer);
	//	}



	//	//this->meshes.resize(this->meshBuffers.size());

	//	//for (int i = 0; i < meshBuffers.size(); ++i) {
	//	//	vkx::Mesh m(meshBuffers[i])
	//	//}

	//	// if these mesh buffers haven't been stored, store them
	//	if (!this->assetManager->meshBuffers.present(this->filename)) {
	//		this->assetManager->meshBuffers.add(filename, meshBuffers);
	//	}




	//	auto tEnd = std::chrono::high_resolution_clock::now();

	//	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();
	//	printf("Load Time: %d\n", duration);

	//}


















	void vkx::MeshLoader::createMeshBuffers(const std::vector<VertexComponent> &layout, float scale) {

		auto tStart = std::chrono::high_resolution_clock::now();


		// todo: fix:
		if (this->assetManager->meshBuffers.present(filename)) {
			// assumes correct vertex layout
			this->meshBuffers = this->assetManager->meshBuffers.get(filename);
			return;
		}

		// for each mesh
		for (int m = 0; m < m_Entries.size(); m++) {



			std::vector<defaultVert> verticesTest;
			verticesTest.resize(m_Entries[m].Vertices.size());
			std::vector<float> vertexBuffer;
			int numOfFloats = m_Entries[m].Vertices.size() * (vkx::vertexSize(layout) / sizeof(float));
			vertexBuffer.reserve(numOfFloats);


			if (layout == defaultLayout) {

				for (int i = 0; i < m_Entries[m].Vertices.size(); i++) {
					// pos
					verticesTest[i].pos = glm::make_vec3(&m_Entries[m].Vertices[i].m_pos.x) * scale;
					// uv
					verticesTest[i].uv = glm::make_vec2(&m_Entries[m].Vertices[i].m_tex.x);
					// color
					verticesTest[i].color = glm::make_vec3(&m_Entries[m].Vertices[i].m_color.x);
					// normal
					verticesTest[i].normal = glm::make_vec3(&m_Entries[m].Vertices[i].m_normal.x);
					// tangent
					verticesTest[i].tangent = glm::make_vec3(&m_Entries[m].Vertices[i].m_tangent.x);

				}

			} else {
				//std::vector<int Vertex::*> componentMap;
				//std::vector<int> componentMap;
				std::vector<ComponentInfo> componentMap;


				//auto a = offsetof(Vertex, m_pos);
				//int Foo::* pm = &Foo::b;

				for (auto &layoutDetail : layout) {
					ComponentInfo c;
					switch (layoutDetail) {
						// Position
						case VERTEX_COMPONENT_POSITION:
							//int Vertex::* pm = &Vertex::m_pos;
							//componentMap.push_back(offsetof(Vertex, m_pos));
							c.offset = offsetof(Vertex, m_pos);
							c.length = 3;
							c.vec3p = &Vertex::m_pos;
							componentMap.push_back(c);
							break;
							// Normal
						case VERTEX_COMPONENT_NORMAL:
							//componentMap.push_back(offsetof(Vertex, m_normal));
							c.offset = offsetof(Vertex, m_normal);
							c.length = 3;
							c.vec3p = &Vertex::m_normal;
							componentMap.push_back(c);
							break;
							// Texture Coordinates
						case VERTEX_COMPONENT_UV:
							//componentMap.push_back(offsetof(Vertex, m_tex));
							c.offset = offsetof(Vertex, m_tex);
							c.length = 2;
							c.vec2p = &Vertex::m_tex;
							componentMap.push_back(c);
							break;
							// Color
						case VERTEX_COMPONENT_COLOR:
							//componentMap.push_back(offsetof(Vertex, m_color));
							c.offset = offsetof(Vertex, m_color);
							c.length = 3;
							c.vec3p = &Vertex::m_color;
							componentMap.push_back(c);
							break;
							// Tangent
						case VERTEX_COMPONENT_TANGENT:
							//componentMap.push_back(offsetof(Vertex, m_tangent));
							c.offset = offsetof(Vertex, m_tangent);
							c.length = 3;
							c.vec3p = &Vertex::m_tangent;
							componentMap.push_back(c);
							break;
							// Bitangent
						case VERTEX_COMPONENT_BITANGENT:
							//componentMap.push_back(offsetof(Vertex, m_binormal));
							c.offset = offsetof(Vertex, m_binormal);
							c.length = 3;
							c.vec3p = &Vertex::m_binormal;
							componentMap.push_back(c);
							break;
							// Dummy layout components for padding
						case VERTEX_COMPONENT_DUMMY_FLOAT:
							//componentMap.push_back(-1);
							c.offset = -1;
							c.length = 1;
							c.vertexComponent = VERTEX_COMPONENT_DUMMY_FLOAT;
							componentMap.push_back(c);
							break;
						case VERTEX_COMPONENT_DUMMY_VEC4:
							//componentMap.push_back(-4);
							c.offset = -4;
							c.length = 4;
							c.vertexComponent = VERTEX_COMPONENT_DUMMY_VEC4;
							componentMap.push_back(c);
							break;
					}
				}

				// for each vertex
				for (int i = 0; i < m_Entries[m].Vertices.size(); i++) {
					// Push vertex data depending on layout



					for (auto &info : componentMap) {

						if (info.vertexComponent == VERTEX_COMPONENT_DUMMY_VEC4) {
							vertexBuffer.push_back(0.0f);
							vertexBuffer.push_back(0.0f);
							vertexBuffer.push_back(0.0f);
							vertexBuffer.push_back(0.0f);
						} else if (info.vertexComponent == VERTEX_COMPONENT_DUMMY_FLOAT) {
							vertexBuffer.push_back(0.0f);
						} else {

							Vertex *vert = &m_Entries[m].Vertices[i];

							if (info.length == 2) {
								glm::vec2 comp = *reinterpret_cast<glm::vec2 *>(reinterpret_cast<char *>(vert) + info.offset);
								vertexBuffer.push_back(comp.x);
								vertexBuffer.push_back(comp.y);
							} else if (info.length == 3) {
								glm::vec3 comp = *reinterpret_cast<glm::vec3 *>(reinterpret_cast<char *>(vert) + info.offset);
								vertexBuffer.push_back(comp.x);
								vertexBuffer.push_back(comp.y);
								vertexBuffer.push_back(comp.z);
							} else if (info.length == 4) {
								glm::vec4 comp = *reinterpret_cast<glm::vec4 *>(reinterpret_cast<char *>(vert) + info.offset);
								vertexBuffer.push_back(comp.x);
								vertexBuffer.push_back(comp.y);
								vertexBuffer.push_back(comp.z);
								vertexBuffer.push_back(comp.w);
							}

						}

					}
				}

			}




			//std::shared_ptr<MeshBuffer> meshesDeferred;
			auto meshBuffer = std::make_shared<MeshBuffer>();



			//MeshBuffer meshBuffer;
			meshBuffer->vertexLayout = layout;

			if (layout == defaultLayout) {
				meshBuffer->vertices.size = verticesTest.size() * sizeof(defaultVert);
			} else {
				meshBuffer->vertices.size = vertexBuffer.size() * sizeof(float);
			}

			dim.min *= scale;
			dim.max *= scale;
			dim.size *= scale;

			std::vector<uint32_t> indexBuffer;
			uint32_t indexBase = (uint32_t)indexBuffer.size();
			for (uint32_t i = 0; i < m_Entries[m].Indices.size(); i++) {
				indexBuffer.push_back(m_Entries[m].Indices[i] + indexBase);
			}

			meshBuffer->indexCount = (uint32_t)indexBuffer.size();
			// Use staging buffer to move vertex and index buffer to device local memory
			// Vertex buffer
			if (layout == defaultLayout) {
				meshBuffer->vertices = this->context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, verticesTest);
			} else {
				meshBuffer->vertices = this->context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
			}
			
			// Index buffer
			meshBuffer->indices = this->context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);
			meshBuffer->dim = dim.size;

			meshBuffer->materialIndex = m_Entries[m].materialIndex;
			meshBuffer->materialName = m_Entries[m].materialName;


			meshBuffers.push_back(meshBuffer);
		}



		//this->meshes.resize(this->meshBuffers.size());

		//for (int i = 0; i < meshBuffers.size(); ++i) {
		//	vkx::Mesh m(meshBuffers[i])
		//}

		// if these mesh buffers haven't been stored, store them
		if (!this->assetManager->meshBuffers.present(this->filename)) {
			this->assetManager->meshBuffers.add(filename, meshBuffers);
		}




		auto tEnd = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();
		printf("Load Time: %d\n", duration);

	}

	void MeshLoader::destroy() {

		//for (int i = 0; i < meshBuffers.size(); ++i) {
		//	meshBuffers[i].destroy();
		//}

	}







































	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ SKINNED MESH  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */




	void vkx::MeshLoader::createSkinnedMeshBuffer(const std::vector<VertexComponent> &layout, float scale) {

		this->combinedBuffer = std::make_shared<MeshBuffer>();

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

				// todo: do this by vertex layout, bones make this difficult
				vertex.pos = m_Entries[m].Vertices[i].m_pos;//*scale
				vertex.uv = m_Entries[m].Vertices[i].m_tex;
				vertex.color = m_Entries[m].Vertices[i].m_color;
				vertex.normal = m_Entries[m].Vertices[i].m_normal;
				vertex.tangent = m_Entries[m].Vertices[i].m_tangent;

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
		this->combinedBuffer->indexCount = indexBuffer.size();
		this->combinedBuffer->vertices = context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer);
		this->combinedBuffer->indices = context->stageToDeviceBuffer(vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer);

		this->combinedBuffer->materialIndex = m_Entries[0].materialIndex;
		this->combinedBuffer->materialName = m_Entries[0].materialName;
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

		// get the current time:
		boneData.tNow = std::chrono::high_resolution_clock::now();

		auto timeSinceUpdate = std::chrono::duration<float, std::milli>(boneData.tNow - boneData.tLastUpdate);
		if (timeSinceUpdate.count() < boneData.waitTimeMS) {
			return;
		}

		float TicksPerSecond = (float)(pScene->mAnimations[0]->mTicksPerSecond != 0 ? pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
		float TimeInTicks = time * TicksPerSecond;
		float AnimationTime = fmod(TimeInTicks, (float)pScene->mAnimations[0]->mDuration);

		aiMatrix4x4 identity = aiMatrix4x4();
		readNodeHierarchy(AnimationTime, pScene->mRootNode, identity);

		for (uint32_t i = 0; i < boneData.boneTransforms.size(); i++) {
			boneData.boneTransforms[i] = boneData.boneInfo[i].finalTransformation;
		}

		// update the time since last update to now:
		boneData.tLastUpdate = std::chrono::high_resolution_clock::now();

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