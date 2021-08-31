#include "Scene.h"

#include "GameObject.h"
#include "SceneManager.h"
#include "Common.h"

DECLARE_TEXT_ASSET(Scene);

void Scene::Init() {
	OPTICK_EVENT();
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
				ASSERT(false);
			}
		}

	}
	removedGameObjects.clear();
}

void Scene::Term() {
	OPTICK_EVENT();
	for (auto& go : gameObjects) {
		DeactivateGameObjectInternal(go);
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
	activeGameObjects.erase(std::find(activeGameObjects.begin(), activeGameObjects.end(), gameObject));
	for (int iC = gameObject->components.size() - 1; iC >= 0; iC--) {
		auto& component = gameObject->components[iC];
		SetComponentEnabledInternal(component.get(), false);
	}
}

void Scene::SetComponentEnabledInternal(Component* component, bool isEnabled) {
	if (component->isEnabled == isEnabled) {
		return;
	}
	component->isEnabled = isEnabled;
	if (isEnabled) {
		component->OnEnable();
		if (component->GetType()->HasTag("HasUpdate") && !component->ignoreUpdate) {
			enabledUpdateComponents.push_back(component);
		}
		if (component->GetType()->HasTag("HasFixedUpdate") && !component->ignoreFixedUpdate) {
			enabledFixedUpdateComponents.push_back(component);
		}
	}
	else {
		if (component->GetType()->HasTag("HasUpdate") && !component->ignoreUpdate) {
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
		if (component->GetType()->HasTag("HasFixedUpdate") && !component->ignoreFixedUpdate) {
			auto it = std::find(enabledFixedUpdateComponents.begin(), enabledFixedUpdateComponents.end(), component);
			int idx = it - enabledFixedUpdateComponents.begin();
			if (currentUpdateIdx < idx) {
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
