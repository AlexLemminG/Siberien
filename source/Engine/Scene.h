#pragma once

#include "Serialization.h"

class GameObject;

class Scene : public Object{
public:
	std::vector<std::shared_ptr<GameObject>> gameObjects;

	void Init();
	void Update();
	void Term();

	REFLECT_BEGIN(Scene);
	REFLECT_VAR(gameObjects, std::vector<std::shared_ptr<GameObject>>());
	REFLECT_END();
};