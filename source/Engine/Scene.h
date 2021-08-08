#pragma once

#include "Serialization.h"

class GameObject;

class Scene : public Object{
public:
	std::vector<std::shared_ptr<GameObject>> gameObjects;

	void Init();
	void Update();
	void Term();

	static std::shared_ptr<GameObject> FindGameObjectByTag(std::string tag);
private:
	static Scene* current;

	REFLECT_BEGIN(Scene);
	REFLECT_VAR(gameObjects);
	REFLECT_END();
};