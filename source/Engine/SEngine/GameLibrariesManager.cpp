#include "GameLibrariesManager.h"
#include "Config.h"
#include "Engine.h"
#include "SDL.h"
#include "Common.h"

class EngineLib : public GameLibrary {
public:
	virtual bool Init(Engine* engine) override {
		if (!GameLibrary::Init(engine)) { return false; }
		return true;
	}
	virtual void Term()override {}
	INNER_LIBRARY(EngineLib);
};
DEFINE_LIBRARY(EngineLib);

static bool alreadyEngineInitedOnce = false; //HACK to prevent engine from registering types multiple times (since it's the only thjing ::Init is doing right now
bool GameLibrariesManager::Init() {

	auto& thisStaticStorage = GameLibraryStaticStorage::Get();
	{
		auto engineLib = SEngine_CreateLibrary();
		std::shared_ptr<GameLibrary> libPtr;
		libPtr.reset(engineLib);
		//if (!engineLib->Init(Engine::Get())) {
		//	ASSERT(false);
		//}
		auto handle = LibraryHandle{};
		handle.library = libPtr;
		handle.name = "EngineLib";
		handle.objectHandle = nullptr;
		libraries.push_back(handle);
	}

	auto libNames = CfgGetNode("libraries");
	for (auto node : libNames) {
		auto libName = node.as<std::string>();
		void* objectHandle = nullptr;
		char* createLibraryFuncName = "SEngine_CreateLibrary";
		GameLibrary* (*createLibraryFunc)();

		objectHandle = SDL_LoadObject(libName.c_str());
		if (!objectHandle) {
			auto error = SDL_GetError();
			ASSERT(false);
			continue;
		}

		createLibraryFunc = (GameLibrary * (*)())SDL_LoadFunction(objectHandle, createLibraryFuncName);
		if (createLibraryFunc == NULL) {
			ASSERT(false);
			continue;
		}

		auto lib = createLibraryFunc();
		if (lib == nullptr) {
			ASSERT(false);
			continue;
		}
		auto handle = LibraryHandle{};
		handle.library.reset(lib);
		handle.name = libName;
		handle.objectHandle = objectHandle;

		libraries.push_back(handle);
	}
	if (!alreadyEngineInitedOnce) {
		libraries[0].library->Init(Engine::Get());
		alreadyEngineInitedOnce = true;
	}
	for (int i = 1; i < libraries.size(); i++) {
		auto& lib = libraries[i];

		lib.library->Init(Engine::Get());
		//TODO get static global storage for engine stuff
		const auto& libStaticStorage = lib.library->GetStaticStorage();
		auto& storage = libStaticStorage.serializationInfoStorage;
		GetSerialiationInfoStorage().Register(storage);

		auto& libSystems = libStaticStorage.systemRegistrators;
		//TODO cleaner
		for (auto s : libSystems) {
			thisStaticStorage.systemRegistrators.push_back(s);
		}
	}

	return true;
}

void GameLibrariesManager::Term() {
	auto& thisStaticStorage = GameLibraryStaticStorage::Get();
	for (int i = libraries.size() - 1; i >= 0; i--) {
		auto& lib = libraries[i];

		auto& systems = lib.library->GetStaticStorage().systemRegistrators;
		//TODO cleaner
		if (i != 0) {
			for (auto s : systems) {
				auto& thisRegistrators = GameLibraryStaticStorage::Get().systemRegistrators;
				thisRegistrators.erase(std::find(thisRegistrators.begin(), thisRegistrators.end(), s));
			}

			auto& storage = lib.library->GetStaticStorage().serializationInfoStorage;
			GetSerialiationInfoStorage().Unregister(storage);
		}
		else {
			//handling engineLib as 'special'
			//TODO there should be no special case for engineLib
			//thisStaticStorage = GameLibraryStaticStorage();//TODO is this legal ?
		}
		lib.library->Term();
		ASSERT(lib.library.use_count() == 1);
		lib.library = nullptr;
		if (lib.objectHandle != nullptr) {
			SDL_UnloadObject(lib.objectHandle);
		}
	}

	libraries.clear();
}
