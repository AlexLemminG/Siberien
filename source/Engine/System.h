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
	virtual void Term() {}

	struct PriorityInfo {
		int order = 0;
	};
	virtual PriorityInfo GetPriorityInfo() const {
		return PriorityInfo ();
	}
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

	bool Init();

	void Update();

	void FixedUpdate();

	void Draw();

	void Term();

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