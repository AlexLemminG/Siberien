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

bool GameLibrariesManager::Init() {

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
		std::shared_ptr<GameLibrary> libPtr;
		libPtr.reset(lib);

		auto handle = LibraryHandle{};
		handle.library = libPtr;
		handle.name = libName;
		handle.objectHandle = objectHandle;

		libraries.push_back(handle);
	}


	auto& thisStaticStorage = GameLibraryStaticStorage::Get();
	{
		auto engineLib = SEngine_CreateLibrary();
		std::shared_ptr<GameLibrary> libPtr;
		libPtr.reset(engineLib);
		if (!engineLib->Init(Engine::Get())) {
			ASSERT(false);
		}
		auto handle = LibraryHandle{};
		handle.library = libPtr;
		handle.name = "EngineLib";
		handle.objectHandle = nullptr;
		libraries.push_back(handle);
	}

	for (auto& lib : libraries) {
		lib.library->Init(Engine::Get());

		//TODO more precise
		if(lib.objectHandle != nullptr){
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
	}

	return true;
}

void GameLibrariesManager::Term() {
	//TODO different oreder
	for (auto& lib : libraries) {

		//TODO more precise
		if (lib.objectHandle != nullptr) {
			auto& systems = lib.library->GetStaticStorage().systemRegistrators;
			//TODO cleaner
			for (auto s : systems) {
				auto& thisRegistrators = GameLibraryStaticStorage::Get().systemRegistrators;
				thisRegistrators.erase(std::find(thisRegistrators.begin(), thisRegistrators.end(), s));
			}

			auto& storage = lib.library->GetStaticStorage().serializationInfoStorage;
			GetSerialiationInfoStorage().Unregister(storage);
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
