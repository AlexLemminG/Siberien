#pragma once

#include "Serialization.h"
#include "GameObject.h"

class GameObject;

class Scene : public Object {
public:
	std::vector<std::shared_ptr<GameObject>> gameObjects;

	void Init();
	void Update();
	void FixedUpdate();
	void Term();

	static std::shared_ptr<GameObject> FindGameObjectByTag(std::string tag);

	static Scene* Get() { return current; }//TODO remove singletons
private:
	static Scene* current;

	REFLECT_BEGIN(Scene);
	REFLECT_VAR(gameObjects);
	REFLECT_END();
};