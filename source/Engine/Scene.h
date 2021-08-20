#pragma once

#include "Serialization.h"
#include "GameObject.h"
#include "Render.h"

class GameObject;

class SphericalHarmonics;

class Scene : public Object {
public:
	std::vector<std::shared_ptr<GameObject>> gameObjects;
	std::string name;

	void Init();
	void Update();
	void FixedUpdate();
	void Term();

	void AddGameObject(std::shared_ptr<GameObject> go);
	void RemoveGameObject(std::shared_ptr<GameObject> go);

	static std::shared_ptr<GameObject> FindGameObjectByTag(std::string tag);

	static std::shared_ptr<Scene> Get();//TODO remove singletons
	virtual void OnBeforeSerializeCallback(SerializationContext& context) const override;

	std::shared_ptr<SphericalHarmonics> sphericalHarmonics; //TODO not here
private:
	bool isInited = false;

	std::vector<std::shared_ptr<GameObject>> addedGameObjects;
	std::vector<std::shared_ptr<GameObject>> removedGameObjects;
	void ProcessAddedGameObjects();
	void ProcessRemovedGameObjects();

	REFLECT_BEGIN(Scene);
	REFLECT_VAR(sphericalHarmonics);
	REFLECT_VAR(gameObjects);
	REFLECT_END();
};