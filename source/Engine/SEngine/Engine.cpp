#include "Engine.h"
#include <iostream>


bool GameLibrary::Init(Engine* engine) {
	return true;
}

void GameLibrary::Term() {
}


bool Engine::IsEditorMode() const { return false; }
