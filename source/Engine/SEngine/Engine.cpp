#include "Engine.h"
//#include "libloaderapi.h"
//#include "delayimp.h"
#include <iostream>
#include "DbgVars.h"

//extern "C"
//FARPROC WINAPI __delayLoadHelper2(
//	PCImgDelayDescr pidd,
//	FARPROC * ppfnIATEntry
//);

DBG_VAR_BOOL(dbg_EditorMode, "Editor mode", false);

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

bool Engine::IsEditorMode() const { return dbg_EditorMode; }

bool Engine::IsQuitPending() const {
	return isQuitPending;
}

void Engine::Quit() {
	isQuitPending = true;
}

//void LoadDelayed(char* exeName) {

//}
