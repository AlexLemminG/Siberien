#include "BlenderSceneLoader.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "BoxCollider.h"
#include "RigidBody.h"
#include "assimp/mesh.h"
#include "Mesh.h"
#include "MeshCollider.h"
#include "MeshRenderer.h"
#include "Resources.h"

void BlenderSceneLoader::AddToNodes(const FullMeshAsset_Node& node, const std::string& baseAssetPath, const Matrix4& parentTransform) {

	Matrix4 matrix = parentTransform * node.localTransformMatrix;

	if (node.mesh) {
		auto meshName = node.mesh->name;
		auto assetPath = baseAssetPath + "$" + meshName;
		bool isHidden = assetPath.find("hidden") != -1;

		auto mesh = AssetDatabase::Get()->Load<Mesh>(assetPath);
		if (mesh && !isHidden) {
			auto gameObject = std::make_shared<GameObject>();
			gameObject->flags = Bits::SetMaskTrue(gameObject->flags, GameObject::FLAGS::IS_HIDDEN_IN_INSPECTOR);

			auto transform = std::make_shared<Transform>();
			transform->SetMatrix(matrix);
			gameObject->components.push_back(transform);

			std::shared_ptr<Material> material = this->material;
			for (auto& mat : materialMapping) {
				if (mat.from == mesh->assetMaterialName) {
					ASSERT(mat.to);
					material = mat.to;
				}
			}

			if (material) {
				auto renderer = std::make_shared<MeshRenderer>();
				renderer->material = material;
				renderer->mesh = mesh;
				gameObject->components.push_back(renderer);
			}

			if (addRigidBodies) {
				auto collider = std::make_shared<MeshCollider>();
				collider->mesh = mesh;
				collider->isConvex = dynamicRigidBodies;
				gameObject->components.push_back(collider);

				auto rigidBody = std::make_shared<RigidBody>();
				rigidBody->isStatic = !dynamicRigidBodies;
				rigidBody->layer = "staticGeom";
				gameObject->components.push_back(rigidBody);
			}

			createdObjects.push_back(gameObject);
			Scene::Get()->AddGameObject(gameObject);
		}
	}

	for (const auto& childNode : node.childNodes) {
		AddToNodes(childNode, baseAssetPath, matrix);
	}
}


void BlenderSceneLoader::OnEnable() {
	if (scene == nullptr) {
		return;
	}
	auto rootUid = AssetDatabase::Get()->GetAssetPath(scene);

	AddToNodes(scene->rootNode, rootUid, Matrix4::Identity());
}

void BlenderSceneLoader::OnDisable() {
	for (auto obj : createdObjects) {
		Scene::Get()->RemoveGameObject(obj);
	}
	createdObjects.clear();
}

DECLARE_TEXT_ASSET(BlenderSceneLoader);