#include "GameLibrariesManager.h"
#include "Config.h"
#include "Engine.h"
#include "SDL.h"
#include "Common.h"

bool GameLibrariesManager::Init() {

	auto libNames = CfgGetNode("libraries");
	for (auto node : libNames) {
		auto libName = node.as<std::string>();
		void* objectHandle = nullptr;
		char* createLibraryFuncName = "SEngine_CreateLibrary";

		GameLibrary* (*createLibraryFunc)();

		objectHandle = SDL_LoadObject(libName.c_str());
		if (!objectHandle) {
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


	Engine engine;

	auto& thisStaticStorage = GameLibraryStaticStorage::Get();
	for (auto& lib : libraries) {
		lib.library->Init(&engine);
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
	//TODO different oreder
	for (auto& lib : libraries) {
		auto& systems = lib.library->GetStaticStorage().systemRegistrators;
		//TODO cleaner
		for (auto s : systems) {
			auto& thisRegistrators = GameLibraryStaticStorage::Get().systemRegistrators;
			thisRegistrators.erase(std::find(thisRegistrators.begin(), thisRegistrators.end(), s));
		}

		auto& storage = lib.library->GetStaticStorage().serializationInfoStorage;
		GetSerialiationInfoStorage().Unregister(storage);
		lib.library->Term();
		ASSERT(lib.library.use_count() == 1);
		lib.library = nullptr;
		SDL_UnloadObject(lib.objectHandle);
	}
	libraries.clear();
}
