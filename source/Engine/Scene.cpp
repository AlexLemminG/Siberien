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

	isInited = true;
}

void Scene::Update() {
	for (auto go : gameObjects) {
		for (auto c : go->components) {
			c->Update();
		}
	}
}

void Scene::FixedUpdate() {
	for (auto go : gameObjects) {
		for (auto c : go->components) {
			c->FixedUpdate();
		}
	}
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
	//TODO share code with Init()

	if (go == nullptr) {
		return;
	}

	gameObjects.push_back(go);

	if (!isInited) {
		return;
	}

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
