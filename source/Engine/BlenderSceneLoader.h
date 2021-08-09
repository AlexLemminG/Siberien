#pragma once

#include "Component.h"
#include "GameObject.h"
#include "MeshRenderer.h"

class BlenderClassLoaderAssetMapping {
public:
	std::shared_ptr<GameObject> prefab;
	std::string name;

	REFLECT_BEGIN(BlenderClassLoaderAssetMapping);
	REFLECT_VAR(prefab);
	REFLECT_VAR(name);
	REFLECT_END();
};

class BlenderSceneLoader : public Component {
	std::vector<BlenderClassLoaderAssetMapping> mapping;
	std::shared_ptr<FullMeshAsset> scene;

	virtual void OnEnable() override;

	REFLECT_BEGIN(BlenderSceneLoader);
	REFLECT_VAR(mapping);
	REFLECT_VAR(scene);
	REFLECT_END();

private:
	void AddToNodes(const aiScene* scene, aiNode* node, std::shared_ptr<GameObject> prefab, std::string nodeMeshName);
};

