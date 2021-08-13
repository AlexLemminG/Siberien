#pragma once

#include "Component.h"

class GameplaySceneScenario : public Component {
	void OnEnable();
	void Update();
	void OnDisable();

	void SpawnZombies(std::shared_ptr<GameObject> prefab, Vector3 pos, int amount);
	static Vector2 SunflowerPattern(int n, float seedRadius);

	bool TriggeredSphere(Vector3 pos, float radius, std::string name);

	std::vector<std::string> triggeredSpheres;

	std::shared_ptr<GameObject> slowZombiePrefab;
	std::shared_ptr<GameObject> fastZombiePrefab;

	REFLECT_BEGIN(GameplaySceneScenario);
	REFLECT_VAR(slowZombiePrefab);
	REFLECT_VAR(fastZombiePrefab);
	REFLECT_END();
};