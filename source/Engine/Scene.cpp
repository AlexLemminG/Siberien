#include "Scene.h"

#include "GameObject.h"

DECLARE_TEXT_ASSET(Scene);

Scene* Scene::current = nullptr;
void Scene::Init() {
	//TOOD remove nullptr components here ?
	current = this;

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
}

void Scene::Update() {
	for (auto go : gameObjects) {
		for (auto c : go->components) {
			c->Update();
		}
	}
}

void Scene::Term() {
	for (auto go : gameObjects) {
		for (auto c : go->components) {
			c->OnDisable();
		}
	}
	current = nullptr;
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
