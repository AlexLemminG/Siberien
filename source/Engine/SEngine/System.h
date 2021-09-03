#pragma once
#include <memory>
#include <vector>
#include "Defines.h"
#include "Engine.h"

class SE_CPP_API SystemBase {
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
		return PriorityInfo();
	}
};

template<typename T>
class SE_CPP_API System : public SystemBase {
public:
	System() {
		GetInternal() = (T*)this;
	}
	virtual ~System() {
		GetInternal() = nullptr;
	}

	static T* Get() { return GetInternal(); }
private:
	static T*& GetInternal();
};
template<typename T>
class GameSystem : public SystemBase {
public:
	GameSystem() {
		GetInternal() = (T*)this;
	}
	virtual ~GameSystem() {
		GetInternal() = nullptr;
	}

	static T* Get() { return GetInternal(); }
private:
	static T*& GetInternal();
};

class SystemRegistratorBase {
public:
	SystemRegistratorBase() {}
	virtual ~SystemRegistratorBase() {}
	virtual std::shared_ptr<SystemBase> CreateSystem() = 0;
};

class SE_CPP_API SystemsManager {
public:
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

	std::vector<std::shared_ptr<SystemBase>> systems;
};


template<typename T>
class SystemRegistrator : public SystemRegistratorBase {
public:
	SystemRegistrator() :SystemRegistratorBase() {
		GameLibraryStaticStorage::Get().systemRegistrators.push_back(this);//TODO remove ?
	}
	~SystemRegistrator() {
	}
	virtual std::shared_ptr<SystemBase> CreateSystem() override { return std::make_shared<T>(); }
};

//TODO remove from include/
#define REGISTER_SYSTEM(systemName) \
static SystemRegistrator<##systemName> SystemRegistrator_##systemName{}; \
template<> \
##systemName*& System<##systemName>::GetInternal() { \
	static systemName* instance = nullptr; \
	return instance; \
}

#define REGISTER_GAME_SYSTEM(systemName) \
static SystemRegistrator<##systemName> SystemRegistrator_##systemName{}; \
template<> \
##systemName*& GameSystem<##systemName>::GetInternal() { \
	static systemName* instance = nullptr; \
	return instance; \
}