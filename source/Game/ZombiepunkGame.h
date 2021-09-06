#pragma once

#include "System.h"
#include "GameEvents.h"

class ZombiepunkGame : public GameSystem<ZombiepunkGame> {
public:
	bool IsGodMode() {
		return false;//TODO
	}
};


class EnemyCreepController;
class GameEvents : public GameSystem<GameEvents> {
	bool Init() override;
public:
	GameEvent<EnemyCreepController*> creepDeath;
};