#include "Scene.h"

#include "GameObject.h"

DECLARE_TEXT_ASSET(Scene);

void Scene::Init() {
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
}
