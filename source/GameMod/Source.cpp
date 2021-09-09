#include <iostream>
#include "Engine.h"
#include "Serialization.h"
#include "Component.h"
#include "System.h"

class GameLib : public GameLibrary {
	virtual bool Init(Engine* engine) override {
		OPTICK_EVENT();
		if (!GameLibrary::Init(engine)) {
			return false;
		}
		std::cout << "Hello there" << std::endl;
		return true;
	}
	void Term() {}
	INNER_LIBRARY();
};

class Sys : public GameSystem<Sys> {
	bool Init()override {
		std::cout << "General kenobi!" << std::endl;
		return true;
	}
};
REGISTER_GAME_SYSTEM(Sys);
DEFINE_LIBRARY(GameLib);