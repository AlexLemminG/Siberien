#pragma once
#include <memory>
#include <vector>

class SystemBase {
public:
	SystemBase() {
	}
	virtual ~SystemBase() {
	}
	virtual bool Init() { return true; }
	virtual void Update() {}
	virtual void FixedUpdate() {}
	virtual void Draw() {}
	virtual void Term() {  }
};

template<typename T>
class System : public SystemBase {
public:
	System() {
		instance = (T*)this;
	}
	virtual ~System() {
		instance = nullptr;
	}

	static T* Get() { return instance; }
private:
	static T* instance;
};

class SystemRegistratorBase {
public:
	SystemRegistratorBase() {}
	virtual ~SystemRegistratorBase() {}
	virtual std::shared_ptr<SystemBase> CreateSystem() = 0;
};

class SystemsManager {
public:
	static void Register(SystemRegistratorBase* registrator) {
		GetRegistrators().push_back(registrator);
	}
	static SystemsManager* manager;
	static SystemsManager* Get() {
		return manager;
	}

	SystemsManager() {}

	bool Init() {
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

	void Update() {
		for (auto system : systems) {
			system->Update();
		}
	}

	void FixedUpdate() {
		for (auto system : systems) {
			system->FixedUpdate();
		}
	}

	void Draw() {
		for (auto system : systems) {
			system->Draw();
		}
	}

	void Term() {
		//TODO term only inited
		for (int i = systems.size() - 1; i >= 0;i--){
			auto system = systems[i];
			system->Term();
		}
		systems.clear();
		manager = nullptr;
	}

private:
	static std::vector<SystemRegistratorBase*>& GetRegistrators() {
		static std::vector<SystemRegistratorBase*> registrators;
		return registrators;
	}

	std::vector<std::shared_ptr<SystemBase>> systems;
};


template<typename T>
class SystemRegistrator : public SystemRegistratorBase {
public:
	SystemRegistrator() :SystemRegistratorBase() {
		SystemsManager::Register(this);
	}
	~SystemRegistrator() {
	}
	virtual std::shared_ptr<SystemBase> CreateSystem() override { return std::make_shared<T>(); }
};


#define REGISTER_SYSTEM(systemName) \
static SystemRegistrator<##systemName> SystemRegistrator_##systemName{}; \
systemName* ##systemName::instance;

//
//#define DECLARE_TEXT_ASSET(className) \
//static TextAssetImporterRegistrator<SerializedObjectImporter<##className>> AssetImporterRegistrator_##className{#className};