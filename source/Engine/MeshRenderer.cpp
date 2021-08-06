#include "MeshRenderer.h"
#include "assimp/Importer.hpp"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "bgfx/bgfx.h"
//#include "bgfx_utils.h"

class VertexLayout {
public:
	bgfx::VertexLayout bgfxLayout;

	VertexLayout& Begin() {
		bgfxLayout.begin();
		return *this;
	}

	VertexLayout& AddPosition() {
		bgfxLayout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
		attributes.push_back(bgfx::Attrib::Position);
		return *this;
	}

	VertexLayout& End() {
		bgfxLayout.end();
		return *this;
	}
	std::vector<bgfx::Attrib::Enum> attributes;

	std::vector<uint8_t> CreateBuffer(aiMesh* mesh) {
		int numVerts = mesh->mNumVertices;
		int stride = bgfxLayout.m_stride;
		std::vector<uint8_t> buffer;
		buffer.resize(bgfxLayout.getSize(numVerts));
		for (int i = 0; i < attributes.size(); i++) {
			auto attribute = attributes[i];
			int offset = bgfxLayout.getOffset(attribute);
			if (attribute == bgfx::Attrib::Position) {
				aiVector3D* srcBuffer = mesh->mVertices;
				uint8_t* dstBuffer = &buffer[offset];
				for (int iVertex = 0; iVertex < numVerts; iVertex++) {
					memcpy(dstBuffer, srcBuffer, sizeof(float) * 3);
					srcBuffer += 1;
					dstBuffer += stride;
				}
			}
		}
		return buffer;
	}
};

static VertexLayout layout_pos;
void InitVertexLayouts() {
	layout_pos.Begin().AddPosition().End();
}

class FullMeshAsset : public Object {
public:
	std::shared_ptr<const aiScene> scene;
};

class MeshAssetImporter : public AssetImporter {
public:
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const std::string& path) override
	{
		//TODO importer could be heavy
		Assimp::Importer importer{};
		//TODO return void for all importers
		auto fullPath = AssetDatabase::assetsFolderPrefix + path; //TODO pass full path or file as argument
		auto* scene = importer.ReadFile(fullPath, aiProcess_ValidateDataStructure);
		if (!scene) {
			return nullptr;
		}
		scene = importer.GetOrphanedScene();
		auto fullAsset = std::make_shared<FullMeshAsset>();
		fullAsset->scene.reset(scene);
		//TODO pass separate database handle
		database.AddAsset(fullAsset, path, "FullMeshAsset");

		for (int iMesh = 0; iMesh < scene->mNumMeshes; iMesh++) {
			auto* aiMesh = scene->mMeshes[iMesh];
			auto mesh = std::make_shared<Mesh>();
			mesh->originalMeshPtr = aiMesh;
			database.AddAsset(mesh, path, aiMesh->mName.C_Str());

			//TODO should keep buffer ?
			auto buffer = layout_pos.CreateBuffer(aiMesh);
			mesh->vertexBuffer = bgfx::createVertexBuffer(bgfx::makeRef(&buffer.begin(), buffer.size()), layout_pos.bgfxLayout);

			mesh->indexBuffer = bgfx::createIndexBuffer(bgfx::makeRef(aiMesh->mFaces, aiMesh->mNumFaces));


			//bgfx::createVertexBuffer(bgfx::makeRef(aiMesh->mVertices, aiMesh->mNumVertices), VertexLayouts::);
		}


		return nullptr;
	}
private:
};

class ShaderAssetImporter : public AssetImporter {
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const std::string& path) override
	{
		auto shader = std::shared_ptr<Shader>();

		//shader->program = loadProgram(path.c_str(), path.c_str());

		database.AddAsset(shader, path, "");

		return nullptr;
	}
};

DECLARE_BINARY_ASSET(Mesh, MeshAssetImporter);
DECLARE_TEXT_ASSET(Material);
DECLARE_BINARY_ASSET(Shader, ShaderAssetImporter);
DECLARE_TEXT_ASSET(MeshRenderer);

std::vector<MeshRenderer*> MeshRenderer::enabledMeshRenderers;
