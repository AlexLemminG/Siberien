#pragma once

#include "Component.h"
#include "GameObject.h"

class FullMeshAsset;
class Material;
class aiScene;
class aiNode;
class FullMeshAsset_Node;

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
	std::shared_ptr<Material> material;
	std::shared_ptr<Material> materialNeon;
	std::shared_ptr<Material> materialSigns;
	std::shared_ptr<Material> materialRoads;
	std::shared_ptr<Material> materialPosters;
	std::shared_ptr<FullMeshAsset> scene;

	bool addRigidBodies = true;
	bool dynamicRigidBodies = false;

	virtual void OnEnable() override;
	virtual void OnDisable() override;

	REFLECT_BEGIN(BlenderSceneLoader);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_VAR(material);
	REFLECT_VAR(materialNeon);
	REFLECT_VAR(materialSigns);
	REFLECT_VAR(materialRoads);
	REFLECT_VAR(materialPosters);
	REFLECT_VAR(scene);
	REFLECT_VAR(addRigidBodies);
	REFLECT_VAR(dynamicRigidBodies);
	REFLECT_END();

	std::vector<std::shared_ptr<GameObject>> createdObjects;
private:
	void AddToNodes(const FullMeshAsset_Node& node, const std::string& baseAssetPath, const Matrix4& parentTransform);
};

