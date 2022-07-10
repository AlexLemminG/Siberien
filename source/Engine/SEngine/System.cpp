

#include "Common.h"
#include "System.h"
#include "SMath.h"
#include "LuaSystem.h"

SystemsManager* SystemsManager::manager;

bool SystemsManager::Init() {
	OPTICK_EVENT();
	manager = this;
	auto& registrators = GameLibraryStaticStorage::Get().systemRegistrators;
	for (auto registrator : registrators) {
		systems.push_back(registrator->CreateSystem());
	}

	std::unordered_map<SystemBase*, std::string> systemNames; //TODO less hacky way
	for (int i = 0; i < registrators.size(); i++) {
		auto system = systems[i];
		systemNames[system.get()] = registrators[i]->systemName;
		//TODO unregister
		LuaSystem::Get()->RegisterFunction(registrators[i]->systemName, std::function([system]() { return system; }));
	}
	std::sort(systems.begin(), systems.end(),
		[](const auto& x, const auto& y) {
			return x->GetPriorityInfo().order < y->GetPriorityInfo().order;
		}
	);

	for (auto system : systems) {
		if (!system->Init()) {
			LogCritical("Failed to init '%s' system", systemNames[system.get()].c_str());
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
	OPTICK_EVENT();
	//TODO term only inited
	for (int i = systems.size() - 1; i >= 0; i--) {
		auto system = systems[i];
		system->Term();
	}
	systems.clear();
	manager = nullptr;
}

SystemBase::SystemBase() {
}

SystemBase::~SystemBase() {
}

bool SystemBase::Init() { return true; }

void SystemBase::Update() {}

void SystemBase::FixedUpdate() {}

void SystemBase::Draw() {}

void SystemBase::Term() {}

SystemBase::PriorityInfo SystemBase::GetPriorityInfo() const {
	return PriorityInfo();
}
