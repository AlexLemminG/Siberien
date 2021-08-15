#include "GameplaySceneScenario.h"

#include "GameObject.h"
#include "Transform.h"
#include "Scene.h"
#include "GameEvents.h"
#include "EnemyCreep.h"
#include "MeshRenderer.h"
#include "Time.h"

DECLARE_TEXT_ASSET(GameplaySceneScenario);


void GameplaySceneScenario::OnEnable() {
	SpawnZombies(slowZombiePrefab, Vector3(-6, 1.f, 26), 15);
	creepDeathHandler = GameEvents::Get()->creepDeath.Subscribe([this](auto creep) {HandleCreepDeath(creep); });

	doors.push_back(Door("door1"));
	doors.push_back(Door("door2"));
	doors.push_back(Door("door3"));
	doors.push_back(Door("door4"));
}

void GameplaySceneScenario::Update() {
	UpdateCamera();

	if (!IsTriggered("roof")) {
		if (IsPlayerAtSphere(Vector3(-3.7f, 0.f, 43.5), 7.5f)) {
			Trigger("roof");
			SpawnZombies(fastZombiePrefab, Vector3(-11.9, 5, 41.7), 40);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door1_open")) {
		if (countSpawned == countDead) {
			Trigger("door1_open");
			doors[0].SetOpened(true);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door1_close")) {
		if (IsPlayerAtSphere(Vector3(-5.1f, 0.76f, 73.2), 7.5f)) {
			Trigger("door1_close");
			doors[0].SetOpened(false);
			SpawnZombies(chadZombiePrefab, Vector3(-34.3, 3, 82.9), 1);
			SpawnZombies(fastZombiePrefab, Vector3(-21.6, 3, 102.6), 50);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door2_open")) {
		if (countSpawned == countDead) {
			Trigger("door2_open");
			doors[1].SetOpened(true);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door2_close")) {
		if (IsPlayerAtSphere(Vector3(-17.4f, 3, 131.4), 1.8f)) {
			Trigger("door2_close");
			doors[1].SetOpened(false);
			SpawnZombies(slowZombiePrefab, Vector3(-11.8, 3, 124.7), 100);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door3")) {
		if (countSpawned == countDead) {
			Trigger("door3");
			doors[2].SetOpened(true);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door3_close")) {
		if (IsPlayerAtSphere(Vector3(7.38f, 3, 125.67), 1.9f)) {
			Trigger("door3_close");
			doors[2].SetOpened(false);
			SpawnZombies(slowZombiePrefab, Vector3(12.39, 3, 143.4), 300);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door4")){
		if (countSpawned == countDead) {
			Trigger("door4");
			doors[3].SetOpened(true);
		}
		else {
			return;
		}
	}
	if (!IsTriggered("door4_close")) {
		if (IsPlayerAtSphere(Vector3(-27.4, 3.0, 153.7), 2.8f)) {
			Trigger("door4_close");
			doors[3].SetOpened(false);
			SpawnZombies(fastZombiePrefab, Vector3(-48, 5.6, 141.3), 100);
			SpawnZombies(slowZombiePrefab, Vector3(-37, 3, 166), 200);
			SpawnZombies(slowZombiePrefab, Vector3(-39, 3, 178), 200);
		}
		else {
			return;
		}
	}
}


void GameplaySceneScenario::HandleCreepDeath(EnemyCreepController* creep) {
	if (randomGun) {
		randomGun->Spawn(creep->gameObject()->transform()->GetPosition());
	}
	countDead++;
}

void GameplaySceneScenario::UpdateCamera()
{
	if (
		IsPlayerAtSphere(Vector3(-9.26, 3, 135), 11.86f) ||
		IsPlayerAtSphere(Vector3(-6.34, 3, 124.43), 12.737f)
		) {
		isIndoor = true;
	}
	else {
		isIndoor = false;
	}
	auto mainCamera = Camera::GetMain();
	if (!mainCamera || !indoorCameraSettings || !outdoorCameraSettings) {
		return;
	}
	auto targetCamera = isIndoor ? indoorCameraSettings : outdoorCameraSettings;
	float speed = 2.f;
	mainCamera->SetFov(Mathf::Lerp(mainCamera->GetFov(), targetCamera->GetFov(), Time::deltaTime() * speed));
}

void GameplaySceneScenario::OnDisable() {
	GameEvents::Get()->creepDeath.Unsubscribe(creepDeathHandler);
}

Vector2 GameplaySceneScenario::SunflowerPattern(int n, float seedRadius) {
	float angle = Mathf::pi * 2 / Mathf::Pow(Mathf::phi, 2) * n;
	float radius = seedRadius * Mathf::Sqrt(n) * seedRadius;

	return Vector2(Mathf::Sin(angle), Mathf::Cos(angle)) * radius;
}
void GameplaySceneScenario::SpawnZombies(std::shared_ptr<GameObject> prefab, Vector3 centerPos, int amount) {
	for (int i = 0; i < amount; i++) {
		auto sun = SunflowerPattern(i, 0.5f);
		auto pos = centerPos + Vector3(sun.x, 0.f, sun.y);
		auto go = Object::Instantiate(prefab);
		go->transform()->SetPosition(pos);
		auto rot = Quaternion::FromAngleAxis(Random::Range(0.f, Mathf::pi * 2), Vector3_up);
		go->transform()->SetRotation(rot);
		Scene::Get()->AddGameObject(go);
		countSpawned++;
	}
}
bool GameplaySceneScenario::IsTriggered(const std::string& name) {
	return std::find(triggeredEvents.begin(), triggeredEvents.end(), name) != triggeredEvents.end();
}
void GameplaySceneScenario::Trigger(const std::string& name) {
	triggeredEvents.push_back(name);
}

bool GameplaySceneScenario::IsPlayerAtSphere(Vector3 center, float radius) {
	auto player = Scene::Get()->FindGameObjectByTag("Player");
	if (Vector3::Distance(player->transform()->GetPosition(), center) < radius) {
		return true;
	}
	else {
		return false;
	}

}
bool GameplaySceneScenario::TriggeredSphere(Vector3 center, float radius, std::string name) {
	bool wasTriggered = IsTriggered(name);
	if (wasTriggered) {
		return false;
	}
	if (IsPlayerAtSphere(center, radius)) {
		Trigger(name);
		return true;
	}
	return false;
}

GameplaySceneScenario::Door::Door(std::string name) {
	this->name = name;
	door = Scene::Get()->FindGameObjectByTag(name);
	light = Scene::Get()->FindGameObjectByTag(name + "_light");
}

void GameplaySceneScenario::Door::SetOpened(bool isOpened) {
	if (this->isOpened == isOpened) {
		return;
	}
	this->isOpened = isOpened;
	if (isOpened) {
		if (door) {
			Scene::Get()->RemoveGameObject(door);
		}
		if (light) {
			light->GetComponent<PointLight>()->color = Colors::green;
		}
	}
	else {
		if (door) {
			Scene::Get()->AddGameObject(door);
		}
		if (light) {
			light->GetComponent<PointLight>()->color = Colors::red;
		}
	}
}
