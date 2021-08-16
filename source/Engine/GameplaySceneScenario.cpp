#include "GameplaySceneScenario.h"

#include "GameObject.h"
#include "Transform.h"
#include "Scene.h"
#include "GameEvents.h"
#include "EnemyCreep.h"
#include "MeshRenderer.h"
#include "Time.h"
#include "PlayerController.h"
#include "Gun.h"

DECLARE_TEXT_ASSET(GameplaySceneScenario);


void GameplaySceneScenario::OnEnable() {

	doors.push_back(Door("door1"));
	doors.push_back(Door("door2"));
	doors.push_back(Door("door3"));
	doors.push_back(Door("door4"));
	doors.push_back(Door("door5"));

	indoorHidePlane = Scene::Get()->FindGameObjectByTag("indoorHidePlane");
	creepDeathHandler = GameEvents::Get()->creepDeath.Subscribe([this](auto creep) {HandleCreepDeath(creep); });

	LoadLastCheckpoint();

}

void GameplaySceneScenario::Update() {
	UpdateCamera();

	if (!IsTriggered("init")) {
		Trigger("init");
		SpawnZombies(slowZombiePrefab, Vector3(-6, 1.f, 26), 15);
	}

	if (!IsTriggered("roof")) {
		if (IsPlayerAtSphere(Vector3(-3.7f, 0.f, 43.5), 7.5f)) {
			Trigger("roof");
			SpawnZombies(fastZombiePrefab, Vector3(-16.34, 5, 43.97), 40);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door1_open")) {
		if (countSpawned == countDead) {
			Trigger("door1_open");
			doors[0].SetOpened(true);
			SaveCheckpoint(0, Vector3(-2.75, 0, 61.54));
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door1_close")) {
		if (IsPlayerAtSphere(Vector3(-5.8f, 2, 75.6), 3.5f)) {
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
			SaveCheckpoint(1, Vector3(-22.5, 3, 126.8));
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door2_close")) {
		if (IsPlayerAtSphere(Vector3(-17.35f, 3, 131.0), 1.67f)) {
			if (indoorHidePlane) {
				Scene::Get()->RemoveGameObject(indoorHidePlane);
			}
			Trigger("door2_close");
			doors[1].SetOpened(false);
			SpawnZombies(slowZombiePrefab, Vector3(-11.8, 3, 124.7), 55);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door3")) {
		if (countSpawned == countDead) {
			Trigger("door3");
			doors[2].SetOpened(true);
			SaveCheckpoint(2, Vector3(-8.4, 3, 124.3));
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door3_close")) {
		if (IsPlayerAtSphere(Vector3(7.38f, 3, 125.67), 1.9f)) {
			Trigger("door3_close");
			doors[2].SetOpened(false);
			SpawnZombies(mediumZombiePrefab, Vector3(20.57, 5, 161.2), 300);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door4")) {
		if (countSpawned == countDead) {
			Trigger("door4");
			doors[3].SetOpened(true);
			SaveCheckpoint(3, Vector3(0, 3, 159.6));
		}
		else {
			return;
		}
	}
	if (!IsTriggered("door4_close")) {
		if (IsPlayerAtSphere(Vector3(-25.36, 3.0, 153.2), 3.8f)) {
			Trigger("door4_close");
			doors[3].SetOpened(false);
			SpawnZombies(mediumZombiePrefab, Vector3(-48, 5.6, 141.3), 100);
			SpawnZombies(mediumZombiePrefab, Vector3(-37, 3, 166), 200);
			SpawnZombies(slowZombiePrefab, Vector3(-39, 3, 178), 200);
		}
		else {
			return;
		}
	}

	if (!IsTriggered("door5")) {
		if (countSpawned == countDead) {
			Trigger("door5");
			doors[4].SetOpened(true);
			//SaveCheckpoint(4);
		}
		else {
			return;
		}
	}
	if (IsPlayerAtSphere(Vector3(-88.98, 8.6, 149.7), 1.0f)) {
		auto player = Scene::Get()->FindGameObjectByTag("Player");
		if (player) {
			auto controller = player->GetComponent<PlayerController>();
			controller->SetWon();
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


void GameplaySceneScenario::SaveCheckpoint(int idx, Vector3 pos) {
	hasCheckpoint = true;
	lastCheckpoint = Checkpoint();
	lastCheckpoint.pos = pos;
	lastCheckpoint.idx = idx;
	lastCheckpoint.triggers = this->triggeredEvents;
}

bool GameplaySceneScenario::hasCheckpoint = false;
GameplaySceneScenario::Checkpoint GameplaySceneScenario::lastCheckpoint;

void GameplaySceneScenario::LoadLastCheckpoint() {
	if (!hasCheckpoint) {
		return;
	}
	if (lastCheckpoint.idx < 0 || lastCheckpoint.idx >= checkpointAmmo.size()) {
		ASSERT(false);
		return;
	}

	auto player = Scene::Get()->FindGameObjectByTag("Player");
	player->transform()->SetPosition(lastCheckpoint.pos);

	auto guns = checkpointAmmo[lastCheckpoint.idx].guns;
	for (auto gun : guns) {
		player->GetComponent<PlayerController>()->AddGun(gun);
	}
	player->GetComponent<PlayerController>()->grenadesCount = checkpointAmmo[lastCheckpoint.idx].grenades;
	this->triggeredEvents = lastCheckpoint.triggers;

	if (lastCheckpoint.idx < doors.size()) {
		doors[lastCheckpoint.idx].SetOpened(true);
	}
	if (lastCheckpoint.idx >= 2) {
		if (indoorHidePlane) {
			Scene::Get()->RemoveGameObject(indoorHidePlane);
		}
	}
}
