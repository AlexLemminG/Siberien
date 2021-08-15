#pragma once

#include "Component.h"
#include "Pickup.h"
#include "Camera.h"

class EnemyCreepController;
class GameplaySceneScenario : public Component {
	void OnEnable();
	void Update();
	void OnDisable();

	void SpawnZombies(std::shared_ptr<GameObject> prefab, Vector3 pos, int amount);
	static Vector2 SunflowerPattern(int n, float seedRadius);

	bool TriggeredSphere(Vector3 pos, float radius, std::string name);

	std::vector<std::string> triggeredEvents;

	std::shared_ptr<GameObject> slowZombiePrefab;
	std::shared_ptr<GameObject> fastZombiePrefab;
	std::shared_ptr<GameObject> chadZombiePrefab;

	REFLECT_BEGIN(GameplaySceneScenario);
	REFLECT_VAR(randomGun);
	REFLECT_VAR(chadZombiePrefab);
	REFLECT_VAR(slowZombiePrefab);
	REFLECT_VAR(fastZombiePrefab);
	REFLECT_VAR(indoorCameraSettings);
	REFLECT_VAR(outdoorCameraSettings);
	REFLECT_END();

	std::shared_ptr<RandomObjectSpawner> randomGun;
	std::shared_ptr<Camera> outdoorCameraSettings;
	std::shared_ptr<Camera> indoorCameraSettings;

	bool IsTriggered(const std::string& name);
	void Trigger(const std::string& name);

private:
	class Door {
	public:
		Door(std::string name);
		void SetOpened(bool isOpened);
	private:
		std::string name;
		bool isOpened = false;
		std::shared_ptr<GameObject> door;
		std::shared_ptr<GameObject> light;
	};
	std::vector<Door> doors;
	void HandleCreepDeath(EnemyCreepController* creep);
	void UpdateCamera();
	bool IsPlayerAtSphere(Vector3 pos, float radius);
	bool isIndoor = false;

	int creepDeathHandler;
	int countDead = 0;
	int countSpawned = 0;
};