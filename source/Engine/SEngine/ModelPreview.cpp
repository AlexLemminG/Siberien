

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
#include "Input.h"
#include "Editor.h"

DECLARE_TEXT_ASSET(ModelPreview);
static int current = 0;
static char buff[255];

void ModelPreview::OnEnable() {
	//TODO filter to get gameObjects only
	allGameObjects = AssetDatabase::Get()->GetAllAssetNames();
	//TODO get rid of std::vector
	for (int i = (int)allGameObjects.size() - 1; i >= 0; i--) {
		if (allGameObjects[i].find(".asset") == -1) {
			allGameObjects.erase(allGameObjects.begin() + i);
			continue;
		}
		auto go = AssetDatabase::Get()->Load<GameObject>(allGameObjects[i]);
		if (!go) {
			allGameObjects.erase(allGameObjects.begin() + i);
			continue;
		}
	}
}

void ModelPreview::Update() {
	UpdateSelection();
	UpdateAnimator();
	UpdateCamera();
}

void ModelPreview::UpdateAnimator() {
	if (!currentGameObject) {
		return;
	}
	auto animator = currentGameObject->GetComponent<Animator>();
	if (!animator) {
		return;
	}

	auto rootUid = AssetDatabase::Get()->GetAssetPath(currentPrefab->GetComponent<MeshRenderer>()->mesh);
	auto fullSceneAsset = AssetDatabase::Get()->Load<FullMeshAsset>(rootUid);
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
			SelectPrefab(AssetDatabase::Get()->Load<GameObject>(allGameObjects[i]));
			std::cout << "selected" << allGameObjects[i] << std::endl;
		}
		drawn++;
		if (drawn >= 100) {
			break;
		}
	}

	ImGui::End();
}

void ModelPreview::UpdateCamera() {
	if (Input::GetMouseButton(0)) {
		if (!wasMouseDown) {
			wasMouseDown = true;
			mouseDownPos = Input::GetMousePosition();
			mouseDownRotation = currentRotation;
		}
		auto deltaRot = Input::GetMousePosition() - mouseDownPos;
		currentRotation = mouseDownRotation + deltaRot * 0.15f;

		auto camera = Scene::FindGameObjectByTag("camera");
		if (camera) {
			Matrix4 matr = (Quaternion::FromAngleAxis(Mathf::DegToRad(currentRotation.x), Vector3_up) * Quaternion::FromAngleAxis(Mathf::DegToRad(currentRotation.y), Vector3_right)).ToMatrix4();
			SetPos(matr, matr * Vector3_forward * (-5.f));
			camera->transform()->SetMatrix(matr);
		}
	}
	else {
		wasMouseDown = false;
	}
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

	Editor::Get()->selectedObject = prefab;
	currentPrefab = prefab;
	if (Editor::Get()->IsInEditMode()) {
		currentGameObject = prefab;
	}
	else {
		currentGameObject = Object::Instantiate(prefab);
		if (currentGameObject->transform() != nullptr) {
			currentGameObject->transform()->SetPosition(Vector3_zero);
			currentGameObject->transform()->SetRotation(Quaternion::identity);
		}
	}
	/*for (int i = currentGameObject->components.size() - 1; i >= 0; i--) {
		auto c = currentGameObject->components[i];

		if (std::dynamic_pointer_cast<Transform>(c)
			|| std::dynamic_pointer_cast<MeshRenderer>(c)
			|| std::dynamic_pointer_cast<Animator>(c)
			) {
			continue;
		}
		currentGameObject->components.erase(currentGameObject->components.begin() + i);
	}*/

	auto animator = currentGameObject->GetComponent<Animator>();
	if (animator) {
		animator->speed = 1.f;
	}
	Scene::Get()->AddGameObject(currentGameObject);
}
