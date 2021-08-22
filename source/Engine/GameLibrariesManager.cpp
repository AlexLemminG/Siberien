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
		handle.objectHandle = objectHandle;

		libraries.push_back(handle);
	}


	Engine engine;

	for (auto& lib : libraries) {
		lib.library->Init(&engine);
		auto& storage = lib.library->GetSerializationInfoStorage();
		GetSerialiationInfoStorage().Register(storage);
	}

	return true;
}

void GameLibrariesManager::Term() {
	//TODO different oreder
	for (auto& lib : libraries) {
		auto& storage = lib.library->GetSerializationInfoStorage();
		GetSerialiationInfoStorage().Unregister(storage);
		lib.library->Term();
		SDL_UnloadObject(lib.objectHandle);
	}
	libraries.clear();
}

REGISTER_SYSTEM(GameLibrariesManager);