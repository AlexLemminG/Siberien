#include "ZombiepunkGame.h"
#include "Config.h"

REGISTER_GAME_SYSTEM(GameEvents);
REGISTER_GAME_SYSTEM(ZombiepunkGame);

bool GameEvents::Init() {
	return true;
}

bool ZombiepunkGame::IsGodMode() {
	return CfgGetBool("godMode");
}
