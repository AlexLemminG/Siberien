#pragma once

#include "Component.h"
#include "Pickup.h"
#include "Camera.h"
#include "Gun.h"

class EnemyCreepController;

class CheckpointAmmo {
public:
	std::vector<std::shared_ptr<Gun>> guns;
	int grenades = 0;
	REFLECT_BEGIN(CheckpointAmmo);
	REFLECT_VAR(guns);
	REFLECT_VAR(grenades);
	REFLECT_END();
};

class GameplaySceneScenario : public Component {
public:
	void OnEnable();
	void Update();
	void OnDisable();

	void SpawnZombies(std::shared_ptr<GameObject> prefab, Vector3 pos, int amount);
	static Vector2 SunflowerPattern(int n, float seedRadius);

	bool TriggeredSphere(Vector3 pos, float radius, std::string name);

	std::vector<std::string> triggeredEvents;

	std::shared_ptr<GameObject> slowZombiePrefab;
	std::shared_ptr<GameObject> fastZombiePrefab;
	std::shared_ptr<GameObject> mediumZombiePrefab;
	std::shared_ptr<GameObject> chadZombiePrefab;
	std::shared_ptr<GameObject> indoorHidePlane;

	REFLECT_BEGIN(GameplaySceneScenario);
	REFLECT_VAR(randomGun);
	REFLECT_VAR(chadZombiePrefab);
	REFLECT_VAR(slowZombiePrefab);
	REFLECT_VAR(mediumZombiePrefab);
	REFLECT_VAR(fastZombiePrefab);
	REFLECT_VAR(indoorCameraSettings);
	REFLECT_VAR(outdoorCameraSettings);
	REFLECT_VAR(checkpointAmmo);
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

	GameEventHandle creepDeathHandler;
	int countDead = 0;
	int countSpawned = 0;

	std::vector<CheckpointAmmo> checkpointAmmo;

	class Checkpoint {
	public:
		int idx;
		Vector3 pos;
		std::vector<std::string> triggers;
	};

	static bool hasCheckpoint;
	static Checkpoint lastCheckpoint;

	void SaveCheckpoint(int idx, Vector3 pos);
	void LoadLastCheckpoint();
};