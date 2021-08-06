#include "MeshRenderer.h"
#include "assimp/Importer.hpp"
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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
		}

		return nullptr;
	}
private:
};

DECLARE_BINARY_ASSET(Mesh, MeshAssetImporter);
DECLARE_TEXT_ASSET(Material);
DECLARE_TEXT_ASSET(MeshRenderer);

std::vector<MeshRenderer*> MeshRenderer::enabledMeshRenderers;
