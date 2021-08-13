#pragma once

#include "Serialization.h"
#include "GameObject.h"
#include "Render.h"

class GameObject;

class Scene : public Object {
public:
	std::vector<std::shared_ptr<GameObject>> gameObjects;

	void Init();
	void Update();
	void FixedUpdate();
	void Term();

	void AddGameObject(std::shared_ptr<GameObject> go);
	void RemoveGameObject(std::shared_ptr<GameObject> go);

	static std::shared_ptr<GameObject> FindGameObjectByTag(std::string tag);

	static Scene* Get() { return current; }//TODO remove singletons
	virtual void OnBeforeSerializeCallback(SerializationContext& context) const override;

	std::shared_ptr<SphericalHarmonics> sphericalHarmonics; //TODO not here
private:
	static Scene* current;
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