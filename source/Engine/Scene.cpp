#include "Scene.h"

#include "GameObject.h"

DECLARE_TEXT_ASSET(Scene);

Scene* Scene::current = nullptr;
void Scene::Init() {
	//TOOD remove nullptr components here ?
	current = this;

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
	for (auto go : gameObjects) {
		if (!go->transform()) {
			go->components.push_back(std::make_shared<Transform>());
		}
	}


	for (auto go : gameObjects) {
		for (auto c : go->components) {
			c->m_gameObject = go;
		}
	}

	for (auto go : gameObjects) {
		for (auto c : go->components) {
			c->OnEnable();
		}
	}

	ProcessRemovedGameObjects();
	ProcessAddedGameObjects();

	isInited = true;
}

void Scene::Update() {
	for (auto go : gameObjects) {
		for (auto c : go->components) {
			c->Update();
		}
	}
	ProcessRemovedGameObjects();
	ProcessAddedGameObjects();
}

void Scene::FixedUpdate() {
	for (auto go : gameObjects) {
		for (auto c : go->components) {
			c->FixedUpdate();
		}
	}
}

void Scene::RemoveGameObject(std::shared_ptr<GameObject> gameObject) {
	removedGameObjects.push_back(gameObject);
}


void Scene::ProcessRemovedGameObjects() {
	for (auto gameObject : removedGameObjects) {
		auto it = std::find(gameObjects.begin(), gameObjects.end(), gameObject);
		if (it != gameObjects.end()) {
			if (isInited) {
				for (int iC = gameObject->components.size() - 1; iC >= 0; iC--) {
					gameObject->components[iC]->OnDisable();
				}
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
	for (auto go : gameObjects) {
		for (int iC = go->components.size() - 1; iC >= 0; iC--) {
			go->components[iC]->OnDisable();
		}
	}
	isInited = false;
	current = nullptr;
}

void Scene::AddGameObject(std::shared_ptr<GameObject> go) {
	addedGameObjects.push_back(go);

}

void Scene::ProcessAddedGameObjects() {
	for (auto go : addedGameObjects) {
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


		for (auto c : go->components) {
			c->m_gameObject = go;
		}

		for (auto c : go->components) {
			c->OnEnable();
		}
	}
	addedGameObjects.clear();
}

std::shared_ptr<GameObject> Scene::FindGameObjectByTag(std::string tag) {
	if (current == nullptr) {
		return nullptr;
	}
	for (auto go : current->gameObjects) {
		if (go->tag == tag) {
			return go;
		}
	}
	return nullptr;
}

//TODO remove singletons

void Scene::OnBeforeSerializeCallback(SerializationContext& context) const {
	for (auto go : gameObjects) {
		context.AddAllowedToSerializeObject(go);
	}
}
