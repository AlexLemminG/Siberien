#include "ModelPreview.h"
#include "dear-imgui/imgui.h"
#include "Resources.h"
#include "SMath.h"
#include "Common.h"
#include "GameObject.h"
#include "Scene.h"
#include "MeshRenderer.h"
#include "Animation.h"
#include "Mesh.h"

DECLARE_TEXT_ASSET(ModelPreview);
static int current = 0;
static char buff[255];

void ModelPreview::OnEnable() {
	allGameObjects = AssetDatabase::Get()->GetAllAssetNames();
	//TODO get rid of std::vector
	for (int i = (int)allGameObjects.size() - 1; i >= 0; i--) {
		if (allGameObjects[i].find(".asset") == -1) {
			allGameObjects.erase(allGameObjects.begin() + i);
		}
		auto go = AssetDatabase::Get()->LoadByPath<GameObject>(allGameObjects[i]);
		if (!go) {
			allGameObjects.erase(allGameObjects.begin() + i);
		}
	}
}

void ModelPreview::Update() {
	UpdateSelection();
	UpdateAnimator();
}

void ModelPreview::UpdateAnimator() {
	if (!currentGameObject) {
		return;
	}
	auto animator = currentGameObject->GetComponent<Animator>();
	if (!animator) {
		return;
	}

	auto assetPath = AssetDatabase::Get()->GetAssetPath(currentPrefab->GetComponent<MeshRenderer>()->mesh);
	assetPath = AssetDatabase::PathDescriptor(assetPath).assetPath;
	auto fullSceneAsset = AssetDatabase::Get()->LoadByPath<FullMeshAsset>(assetPath);
	if (!fullSceneAsset) {
		return;
	}


	ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(100, 500), ImGuiCond_Once);
	ImGui::Begin("select animation");

	ImGui::DragFloat("speed", &animator->speed);
	for (auto animation : fullSceneAsset->animations) {
		if (ImGui::Selectable(animation->name.c_str())) {
			animator->SetAnimation(animation);
		}
	}
	ImGui::End();
}

void ModelPreview::UpdateSelection() {
	ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);
	ImGui::Begin("select prefab");

	ImGui::InputText("gameObject", buff, 255);

	int drawn = 0;
	std::string searchString = buff;
	for (int i = 0; i < allGameObjects.size(); i++) {
		if (allGameObjects[i].find(searchString) == -1) {
			continue;
		}
		if (ImGui::Selectable(allGameObjects[i].c_str())) {
			SelectPrefab(AssetDatabase::Get()->LoadByPath<GameObject>(allGameObjects[i]));
			std::cout << "selected" << allGameObjects[i] << std::endl;
		}
		drawn++;
		if (drawn >= 10) {
			break;
		}
	}

	ImGui::End();
}

void ModelPreview::OnDisable() {
	SelectPrefab(nullptr);
}

void ModelPreview::SelectPrefab(std::shared_ptr<GameObject> prefab) {
	if (currentGameObject) {
		Scene::Get()->RemoveGameObject(currentGameObject);
		currentGameObject = nullptr;
		currentPrefab = nullptr;
	}
	if (!prefab) {
		return;
	}
	currentPrefab = prefab;
	currentGameObject = Object::Instantiate(prefab);
	currentGameObject->transform()->SetPosition(Vector3_zero);
	currentGameObject->transform()->SetRotation(Quaternion::identity);
	for (int i = currentGameObject->components.size() - 1; i >= 0; i--) {
		auto c = currentGameObject->components[i];
		if (std::dynamic_pointer_cast<Transform>(c)
			|| std::dynamic_pointer_cast<MeshRenderer>(c)
			|| std::dynamic_pointer_cast<Animator>(c)) {
			continue;
		}
		currentGameObject->components.erase(currentGameObject->components.begin() + i);
	}

	auto animator = currentGameObject->GetComponent<Animator>();
	if (animator) {
		animator->speed = 1.f;
	}
	Scene::Get()->AddGameObject(currentGameObject);
}
