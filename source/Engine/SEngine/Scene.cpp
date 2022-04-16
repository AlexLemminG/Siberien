

#include "Scene.h"

#include "GameObject.h"
#include "SceneManager.h"
#include "Common.h"
#include "Prefab.h"
#include "Editor.h"
#include "Resources.h"

DECLARE_TEXT_ASSET(Scene);

void Scene::Init(bool isEditMode) {
	OPTICK_EVENT();
	this->isEditMode = isEditMode;
	for (const auto& pi : prefabInstances) {
		auto go = pi.CreateGameObject();
		ASSERT(go);
		gameObjects.push_back(go);
	}
	if (isEditMode) {
		//TODO handle failed to create prefabs
		for (int i = gameObjects.size() - prefabInstances.size(); i < gameObjects.size(); i++) {
			instantiatedPrefabs.push_back(gameObjects[i]);
		}
		gameObjectEditedHandle = Editor::Get()->onGameObjectEdited.Subscribe([this](auto go) {HandleGameObjectEdited(go); });
	}

	//TOOD remove nullptr components here ?
	for (int iG = gameObjects.size() - 1; iG >= 0; iG--) {
		if (gameObjects[iG] == nullptr) {
			gameObjects.erase(gameObjects.begin() + iG);
			continue;
		}
		for (int iC = gameObjects[iG]->components.size() - 1; iC >= 0; iC--) {
			if (gameObjects[iG]->components[iC] == nullptr) {
				gameObjects[iG]->components.erase(gameObjects[iG]->components.begin() + iC);
			}
		}
	}
	for (auto& go : gameObjects) {
		if (!go->transform()) {
			go->components.push_back(std::make_shared<Transform>());
		}
	}


	for (auto& go : gameObjects) {
		for (auto c : go->components) {
			c->m_gameObject = go;
		}
	}

	for (auto& go : gameObjects) {
		ActivateGameObjectInternal(go);
	}

	isInited = true;

	ProcessRemovedGameObjects();
	ProcessAddedGameObjects();
}

void Scene::Update() {
	OPTICK_EVENT();
	for (currentUpdateIdx = 0; currentUpdateIdx < enabledUpdateComponents.size(); currentUpdateIdx++) {
		enabledUpdateComponents[currentUpdateIdx]->Update();
	}
	currentUpdateIdx = -1;
	ProcessRemovedGameObjects();
	ProcessAddedGameObjects();
}

void Scene::FixedUpdate() {
	OPTICK_EVENT();
	for (currentFixedUpdateIdx = 0; currentFixedUpdateIdx < enabledFixedUpdateComponents.size(); currentFixedUpdateIdx++) {
		enabledFixedUpdateComponents[currentFixedUpdateIdx]->FixedUpdate();
	}
	currentFixedUpdateIdx = -1;
}

void Scene::RemoveGameObjectImmediately(std::shared_ptr<GameObject> gameObject) {
	RemoveGameObject(gameObject);
	ProcessRemovedGameObjects();
}

void Scene::RemoveGameObject(std::shared_ptr<GameObject> gameObject) {
	removedGameObjects.push_back(gameObject);
}


void Scene::ProcessRemovedGameObjects() {
	OPTICK_EVENT();
	for (auto& gameObject : removedGameObjects) {
		auto it = std::find(gameObjects.begin(), gameObjects.end(), gameObject);
		if (it != gameObjects.end()) {
			if (isInited) {
				DeactivateGameObjectInternal(gameObject);
			}
			gameObjects.erase(it);
		}
		else {
			it = std::find(addedGameObjects.begin(), addedGameObjects.end(), gameObject);
			if (it != addedGameObjects.end()) {
				addedGameObjects.erase(it);
			}
			else {
				ASSERT_FAILED("Trying to remove game object '%s' but it's not added", gameObject->GetDbgName().c_str());
			}
		}

	}
	removedGameObjects.clear();
}

void Scene::HandleGameObjectEdited(std::shared_ptr<GameObject>& go) {
	for (int i = 0; i < prefabInstances.size(); i++) {
		const auto& pi = prefabInstances[i];
		if (pi.GetOriginalPrefab() == go) {
			//reloading instantiated gameobject
			auto newIp = pi.CreateGameObject();
			RemoveGameObject(instantiatedPrefabs[i]);
			AddGameObject(newIp);
			instantiatedPrefabs[i] = newIp;
		}
	}
}

int Scene::GetInstantiatedPrefabIdx(const GameObject* gameObject)const {
	if (!isEditMode) {
		return -1;
	}
	for (int i = 0; i < instantiatedPrefabs.size(); i++) {
		if (instantiatedPrefabs[i].get() == gameObject) {
			return i;
		}
	}
	return -1;
}

//TODO less strange name

std::shared_ptr<GameObject> Scene::GetSourcePrefab(const GameObject* instantiatedGameObject) const {
	int idx = GetInstantiatedPrefabIdx(instantiatedGameObject);
	if (idx == -1) {
		return nullptr;
	}
	//TODO assert
	return prefabInstances[idx].GetOriginalPrefab();
}

void Scene::Term() {
	OPTICK_EVENT();

	if (isEditMode) {
		Editor::Get()->onGameObjectEdited.Unsubscribe(gameObjectEditedHandle);
	}
	for (auto& go : gameObjects) {
		DeactivateGameObjectInternal(go);
	}
	if (isEditMode) {
		//cleaning up created prefabs
		//TODO cleaner solution
		for (int i = gameObjects.size() - 1; i >= 0; i--) {
			auto go = gameObjects[i];
			if (AssetDatabase::Get()->GetAssetUID(go).empty()) {
				//not an asset, must be instantiated prefab or some other garbage
				gameObjects.erase(gameObjects.begin() + i);
			}
		}
		instantiatedPrefabs.clear();
	}
	isInited = false;
}

void Scene::AddGameObject(std::shared_ptr<GameObject> go) {
	addedGameObjects.push_back(go);
}


void Scene::ActivateGameObjectInternal(std::shared_ptr<GameObject>& gameObject) {
	//TODO assert no active
	activeGameObjects.push_back(gameObject);
	for (auto& component : gameObject->components) {
		SetComponentEnabledInternal(component.get(), true);
	}
}

void Scene::DeactivateGameObjectInternal(std::shared_ptr<GameObject>& gameObject) {
	//TODO assert active
	auto it = std::find(activeGameObjects.begin(), activeGameObjects.end(), gameObject);
	activeGameObjects[it - activeGameObjects.begin()] = activeGameObjects.back();
	activeGameObjects.pop_back();
	for (int iC = gameObject->components.size() - 1; iC >= 0; iC--) {
		auto& component = gameObject->components[iC];
		SetComponentEnabledInternal(component.get(), false);
	}
}


void Scene::SetComponentEnabledInternal(Component* component, bool isEnabled) {
	if (component->IsEnabled() == isEnabled) {
		return;
	}
	component->SetFlags(Bits::SetMask(component->GetFlags(), Component::FLAGS::IS_ENABLED, isEnabled));
	if (isEditMode && std::find_if(component->GetType()->attributes.begin(), component->GetType()->attributes.end(),
		[](const auto& x) {
			return dynamic_cast<ExecuteInEditModeAttribute*>(x.get()) != nullptr;
		}
	) == component->GetType()->attributes.end()) {
		return;
	}
	if (isEnabled) {
		component->OnEnable();
		if (component->GetType()->HasTag("HasUpdate") && !component->HasFlag(Component::FLAGS::IGNORE_UPDATE)) {
			enabledUpdateComponents.push_back(component);
		}
		if (component->GetType()->HasTag("HasFixedUpdate") && !component->HasFlag(Component::FLAGS::IGNORE_FIXED_UPDATE)) {
			enabledFixedUpdateComponents.push_back(component);
		}
	}
	else {
		if (component->GetType()->HasTag("HasUpdate") && !component->HasFlag(Component::FLAGS::IGNORE_UPDATE)) {
			auto it = std::find(enabledUpdateComponents.begin(), enabledUpdateComponents.end(), component);
			int idx = it - enabledUpdateComponents.begin();
			if (currentUpdateIdx < idx) {
				enabledUpdateComponents[idx] = enabledUpdateComponents.back();//cache shuffled
				enabledUpdateComponents.pop_back();
			}
			else {
				//TODO test
				enabledUpdateComponents[idx] = enabledUpdateComponents[currentUpdateIdx];//cache shuffled
				enabledUpdateComponents[currentUpdateIdx] = enabledUpdateComponents.back();
				currentUpdateIdx--;
				enabledUpdateComponents.pop_back();
			}
		}
		if (component->GetType()->HasTag("HasFixedUpdate") && !component->HasFlag(Component::FLAGS::IGNORE_FIXED_UPDATE)) {
			auto it = std::find(enabledFixedUpdateComponents.begin(), enabledFixedUpdateComponents.end(), component);
			int idx = it - enabledFixedUpdateComponents.begin();
			if (currentFixedUpdateIdx < idx) {
				enabledFixedUpdateComponents[idx] = enabledFixedUpdateComponents.back();//cache shuffled
				enabledFixedUpdateComponents.pop_back();
			}
			else {
				//TODO test
				enabledFixedUpdateComponents[idx] = enabledFixedUpdateComponents[currentFixedUpdateIdx];//cache shuffled
				enabledFixedUpdateComponents[currentFixedUpdateIdx] = enabledFixedUpdateComponents.back();
				currentFixedUpdateIdx--;
				enabledFixedUpdateComponents.pop_back();
			}
		}
		component->OnDisable();
	}
}

void Scene::ProcessAddedGameObjects() {
	OPTICK_EVENT();
	for (auto& go : addedGameObjects) {
		//TODO share code with Init()

		if (go == nullptr) {
			continue;
		}

		gameObjects.push_back(go);

		for (int iC = go->components.size() - 1; iC >= 0; iC--) {
			if (go->components[iC] == nullptr) {
				go->components.erase(go->components.begin() + iC);
			}
		}
		if (!go->transform()) {
			go->components.push_back(std::make_shared<Transform>());
		}

		for (auto& c : go->components) {
			c->m_gameObject = go;
		}

		ActivateGameObjectInternal(go);
	}
	addedGameObjects.clear();
}

std::shared_ptr<GameObject> Scene::FindGameObjectByTag(std::string tag) {
	OPTICK_EVENT();
	for (auto& go : Get()->gameObjects) {
		if (go->tag == tag) {
			return go;
		}
	}
	return nullptr;
}

//TODO remove singletons

std::shared_ptr<Scene> Scene::Get() { return SceneManager::GetCurrentScene(); }

void Scene::OnBeforeSerializeCallback(SerializationContext& context) const {
	for (auto go : gameObjects) {
		context.AddAllowedToSerializeObject(go);
	}
}
