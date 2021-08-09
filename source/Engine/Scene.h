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

	void AddGameObject(std::shared_ptr<GameObject> go);

	static std::shared_ptr<GameObject> FindGameObjectByTag(std::string tag);

	static Scene* Get() { return current; }//TODO remove singletons
	virtual void OnBeforeSerializeCallback(SerializationContext& context) const override;

private:
	static Scene* current;
	bool isInited = false;

	std::vector<std::shared_ptr<GameObject>> addedGameObjects;

	void ProcessAddedGameObjects();

	REFLECT_BEGIN(Scene);
	REFLECT_VAR(gameObjects);
	REFLECT_END();
};