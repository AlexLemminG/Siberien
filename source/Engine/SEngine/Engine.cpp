#include "Engine.h"
//#include "libloaderapi.h"
//#include "delayimp.h"
#include <iostream>

//extern "C"
//FARPROC WINAPI __delayLoadHelper2(
//	PCImgDelayDescr pidd,
//	FARPROC * ppfnIATEntry
//);

GameLibrary::GameLibrary() {}
GameLibrary::~GameLibrary() {}

bool GameLibrary::Init(Engine* engine) {
	auto& staticStorage = GetStaticStorage();
	for (auto registrator : staticStorage.importerRegistrators) {
		registrator->Register();
	}
	return true;
}

void GameLibrary::Term() {
}

std::string Engine::GetExeName() {
	return "Engine2.exe";
}

bool Engine::IsEditorMode() const { return false; }

//void LoadDelayed(char* exeName) {

//}
