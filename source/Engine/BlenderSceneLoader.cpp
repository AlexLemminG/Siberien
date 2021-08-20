#include "BlenderSceneLoader.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "BoxCollider.h"
#include "RigidBody.h"
#include "assimp/mesh.h"
#include "Mesh.h"
#include "MeshCollider.h"


void BlenderSceneLoader::AddToNodes(const aiScene* scene, aiNode* node, const std::string& baseAssetPath, const Matrix4& parentTransform) {

	Matrix4 matrix = *(Matrix4*)(void*)(&(node->mTransformation.a1));
	matrix = matrix.Transpose();
	matrix = parentTransform * matrix;
	
	if (node->mNumMeshes >= 1) {
		auto meshName = std::string(scene->mMeshes[node->mMeshes[0]]->mName.C_Str());
		auto assetPath = baseAssetPath + "$" + meshName;
		bool isHidden = assetPath.find("hidden") != -1;

		auto meshO = AssetDatabase::Get()->GetLoaded(assetPath);
		auto mesh = std::dynamic_pointer_cast<Mesh>(meshO);
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

	for (int i = 0; i < node->mNumChildren; i++) {
		AddToNodes(scene, node->mChildren[i], baseAssetPath, matrix);
	}
}


void BlenderSceneLoader::OnEnable() {
	if (scene == nullptr || scene->scene == nullptr) {
		return;
	}
	auto path = AssetDatabase::PathDescriptor(AssetDatabase::Get()->GetAssetPath(scene)).assetPath;
	
	AddToNodes(scene->scene.get(), scene->scene->mRootNode, path, Matrix4::Identity());
}

DECLARE_TEXT_ASSET(BlenderSceneLoader);