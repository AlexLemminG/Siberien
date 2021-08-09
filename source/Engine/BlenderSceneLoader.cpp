#include "BlenderSceneLoader.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "assimp/mesh.h"


void BlenderSceneLoader::AddToNodes(const aiScene* scene, aiNode* node, std::shared_ptr<GameObject> prefab, std::string nodeMeshName) {
	if (node->mNumMeshes>=1 && scene->mMeshes[node->mMeshes[0]]->mName.C_Str() == nodeMeshName) {
		auto gameObject = Object::Instantiate(prefab);
		
		gameObject->transform()->matrix = *(Matrix4*)(void*)(&(node->mTransformation.a1));
		gameObject->transform()->matrix = gameObject->transform()->matrix.Transpose();

		Scene::Get()->AddGameObject(gameObject);
	}

	for (int i = 0; i < node->mNumChildren; i++) {
		AddToNodes(scene, node->mChildren[i], prefab, nodeMeshName);
	}
}


void BlenderSceneLoader::OnEnable() {
	if (scene == nullptr || scene->scene == nullptr) {
		return;
	}
	for (auto it : mapping) {
		if (it.prefab == nullptr) {
			continue;
		}
		AddToNodes(scene->scene.get(), scene->scene->mRootNode, it.prefab, it.name);
	}
}

DECLARE_TEXT_ASSET(BlenderSceneLoader);