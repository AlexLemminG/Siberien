#pragma once

#include "Serialization.h"
#include "GameObject.h"

class GameObject;

class SphericalHarmonics;

class SE_CPP_API Scene : public Object {
	friend class Component;
public:
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

	const std::vector<std::shared_ptr<GameObject>>& GetAllGameObjects() { return gameObjects; }

	std::shared_ptr<SphericalHarmonics> sphericalHarmonics; //TODO not here
private:
	std::vector<std::shared_ptr<GameObject>> gameObjects; //all gameObjects

	std::vector<std::shared_ptr<GameObject>> activeGameObjects;

	bool isInited = false;

	std::vector<std::shared_ptr<GameObject>> addedGameObjects;
	std::vector<std::shared_ptr<GameObject>> removedGameObjects;

	//WOW you love danger!
	std::vector<Component*> enabledUpdateComponents;
	std::vector<Component*> enabledFixedUpdateComponents;

	int currentUpdateIdx = -1;//not really efficient
	int currentFixedUpdateIdx = -1;

	void ActivateGameObjectInternal(std::shared_ptr<GameObject>& gameObject);
	void DeactivateGameObjectInternal(std::shared_ptr<GameObject>& gameObject);

	void SetComponentEnabledInternal(Component* component, bool isEnabled);

	void ProcessAddedGameObjects();
	void ProcessRemovedGameObjects();

	REFLECT_BEGIN(Scene);
	REFLECT_VAR(sphericalHarmonics);
	REFLECT_VAR(gameObjects);
	REFLECT_END();
};