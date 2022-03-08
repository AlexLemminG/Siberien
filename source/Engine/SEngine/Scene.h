#pragma once

#include "Serialization.h"
#include "GameObject.h"
#include "Prefab.h"
#include "GameEvents.h"

class GameObject;

class SphericalHarmonics;

class SE_CPP_API Scene : public Object {
	friend class Component;//TODO why?
	friend class InspectorWindow;
public:
	std::string name;

	void Init(bool isEditMode);
	void Update();
	void FixedUpdate();
	void Term();

	void AddGameObject(std::shared_ptr<GameObject> go);
	void RemoveGameObject(std::shared_ptr<GameObject> go);
	void RemoveGameObjectImmediately(std::shared_ptr<GameObject> go);

	static std::shared_ptr<GameObject> FindGameObjectByTag(std::string tag);

	static std::shared_ptr<Scene> Get();//TODO remove singletons
	virtual void OnBeforeSerializeCallback(SerializationContext& context) const override;

	const std::vector<std::shared_ptr<GameObject>>& GetAllGameObjects() { return gameObjects; }

	bool IsInEditMode()const { return isEditMode; }
	int GetInstantiatedPrefabIdx(const GameObject* gameObject) const;//TODO less strange name
	std::shared_ptr<GameObject> GetSourcePrefab(const GameObject* instantiatedGameObject) const;//TODO are you sure this stuff belongs here?

	std::shared_ptr<SphericalHarmonics> sphericalHarmonics; //TODO not here
private:
	std::vector<std::shared_ptr<GameObject>> gameObjects; // all gameObjects

	std::vector<PrefabInstance> prefabInstances; //+ some extra game objects which are not included in 'all' before init

	std::vector<std::shared_ptr<GameObject>> activeGameObjects;

	bool isInited = false;
	bool isEditMode = false;

	std::vector<std::shared_ptr<GameObject>> addedGameObjects;
	std::vector<std::shared_ptr<GameObject>> removedGameObjects;

	//WOW you love danger!
	std::vector<Component*> enabledUpdateComponents;
	std::vector<Component*> enabledFixedUpdateComponents;

	std::vector<std::shared_ptr<GameObject>> instantiatedPrefabs;

	int currentUpdateIdx = -1;//not really efficient
	int currentFixedUpdateIdx = -1;

	void ActivateGameObjectInternal(std::shared_ptr<GameObject>& gameObject);
	void DeactivateGameObjectInternal(std::shared_ptr<GameObject>& gameObject);

	void SetComponentEnabledInternal(Component* component, bool isEnabled);

	void ProcessAddedGameObjects();
	void ProcessRemovedGameObjects();

	void HandleGameObjectEdited(std::shared_ptr<GameObject>& go);

	GameEventHandle gameObjectEditedHandle;

	REFLECT_BEGIN(Scene);
	REFLECT_VAR(prefabInstances);
	REFLECT_VAR(sphericalHarmonics);
	REFLECT_VAR(gameObjects);
	REFLECT_END();
};