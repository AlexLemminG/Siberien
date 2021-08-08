#pragma once
#include <memory>
#include <vector>

class SystemBase {
public:
	SystemBase() {
	}
	virtual ~SystemBase() {
	}
	virtual bool Init() { }
	virtual void Update() {}
	virtual void Term() {  }
};

template<typename T>
class System {
public:
	System() {
		instance = this;
	}
	virtual ~System() {
		instance = nullptr;
	}

	static T Get() { return instance; }
private:
	static T* instance;
};

class SystemRegistratorBase {
public:
	virtual std::shared_ptr<SystemBase> CreateSystem() = 0;
};

class SystemsManager {
public:
	static void Register(SystemRegistratorBase* registrator) {
		GetRegistrators().push_back(registrator);
	}

	SystemsManager() {}

	bool Init() {
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

	void Update() {
		for (auto system : systems) {
			system->Update();
		}
	}

	void Term() {
		//TODO term only inited
		for (auto system : systems) {
			system->Term();
		}
	}

private:
	static std::vector<SystemRegistratorBase*>& GetRegistrators() {
		static std::vector<SystemRegistratorBase*> registrators;
		return registrators;
	}

	std::vector<std::shared_ptr<SystemBase>> systems;
};


template<typename T>
class SystemRegistrator {
public:
	SystemRegistrator() {
		SystemsManager::Register(this);
	}
	virtual std::shared_ptr<SystemBase> CreateSystem() override { return new T(); }
};


#define REGISTER_SYSTEM(systemName) \
SystemRegistrator<systemName> SystemRegistrator_##SystemRegistrator();