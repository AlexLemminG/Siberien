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

			auto transform = std::make_shared<Transform>();
			transform->matrix = matrix;
			gameObject->components.push_back(transform);

			auto material = this->material;
			if (meshName.find("Road") != -1) {
				material = materialRoads;
			}
			else if (meshName.find("Sign_Ad") != -1) {
				material = materialSigns;
			}
			else if (meshName.find("Sign_Neon") != -1) {
				material = materialNeon;
			}
			else if (meshName.find("Posters") != -1 || meshName.find("Billboard") != -1) {
				material = materialPosters;
			}

			//LogError("%s - %s", node->mName.C_Str(), meshName.c_str());

			auto renderer = std::make_shared<MeshRenderer>();
			renderer->material = material;
			renderer->mesh = mesh;
			gameObject->components.push_back(renderer);

			auto collider = std::make_shared<MeshCollider>();
			collider->mesh = mesh;
			gameObject->components.push_back(collider);

			auto rigidBody = std::make_shared<RigidBody>();
			rigidBody->isStatic = true;
			rigidBody->layer = "staticGeom";
			gameObject->components.push_back(rigidBody);

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

DECLARE_TEXT_ASSET(BlenderSceneLoader);