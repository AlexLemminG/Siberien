#include "ZombiepunkGame.h"

REGISTER_GAME_SYSTEM(GameEvents);
REGISTER_GAME_SYSTEM(ZombiepunkGame);

bool GameEvents::Init() {
	return true;
}

bool ZombiepunkGame::IsGodMode() {
	return true;//TODO
}
