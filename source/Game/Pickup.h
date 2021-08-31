#pragma once
#include "Component.h"
#include "Gun.h"
#include "GameObject.h"

class Pickup : public Component {
public:
	void Update();

	std::shared_ptr<Gun> gun;
	int grenadesCount = 0;

	bool isPicked = false;

	REFLECT_BEGIN(Pickup);
	REFLECT_VAR(gun);
	REFLECT_VAR(grenadesCount);
	REFLECT_END();
};

class PrefabWithWeight {
public:
	std::shared_ptr<GameObject> prefab;
	float weight = 0.f;
	REFLECT_BEGIN(PrefabWithWeight);
	REFLECT_VAR(prefab);
	REFLECT_VAR(weight);
	REFLECT_END();
};

class RandomObjectSpawner : public Object {
public:
	std::vector<PrefabWithWeight> availableObjects;
	REFLECT_BEGIN(RandomObjectSpawner);
	REFLECT_VAR(availableObjects);
	REFLECT_END();

	void Spawn(Vector3 pos);
};