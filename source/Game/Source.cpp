#include <iostream>
#include "Engine.h"
#include "Serialization.h"
#include "Component.h"

class GameLib : public GameLibrary {
	virtual bool Init(Engine* engine) override {
		OPTICK_EVENT();
		if (!GameLibrary::Init(engine)) {
			return false;
		}
		return true;
	}
	INNER_LIBRARY();
};

DEFINE_LIBRARY(GameLib);