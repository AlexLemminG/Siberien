#include "Common.h"
#include "System.h"

SystemsManager* SystemsManager::manager;

bool SystemsManager::Init() {
	manager = this;
	for (auto registrator : GetRegistrators()) {
		systems.push_back(registrator->CreateSystem());
	}

	for (auto system : systems) {
		if (!system->Init()) {
			return false;
		}
	}

	return true;
}

void SystemsManager::Update() {
	OPTICK_EVENT();
	for (auto system : systems) {
		system->Update();
	}
}

void SystemsManager::FixedUpdate() {
	OPTICK_EVENT();
	for (auto system : systems) {
		system->FixedUpdate();
	}
}

void SystemsManager::Draw() {
	OPTICK_EVENT();
	for (auto system : systems) {
		system->Draw();
	}
}

void SystemsManager::Term() {
	//TODO term only inited
	for (int i = systems.size() - 1; i >= 0; i--) {
		auto system = systems[i];
		system->Term();
	}
	systems.clear();
	manager = nullptr;
}
