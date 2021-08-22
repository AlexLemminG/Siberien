#include <iostream>
#include "Engine.h"
#include "Serialization.h"
#include "Component.h"

class GameLib : public GameLibrary {
	virtual bool Init(Engine* engine) override {
		if (!GameLibrary::Init(engine)) {
			return false;
		}
		std::cout << "General kenobi" << std::endl;
		engine->DoSomething();
		return true;
	}

	virtual SerializationInfoStorage& GetSerializationInfoStorage() override;
};

DECLARE_LIBRARY(GameLib);