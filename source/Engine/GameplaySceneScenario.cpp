#include "GameplaySceneScenario.h"

#include "GameObject.h"
#include "Transform.h"
#include "Scene.h"

DECLARE_TEXT_ASSET(GameplaySceneScenario);

void GameplaySceneScenario::OnEnable() {
	SpawnZombies(slowZombiePrefab, Vector3(-6, 1.f, 26), 15);

}

void GameplaySceneScenario::Update() {

	if (TriggeredSphere(Vector3(-3.7f, 0.f, 43.5), 7.5f, "roof")) {
		SpawnZombies(fastZombiePrefab, Vector3(-11.9, 5, 41.7), 40);
	}
}

void GameplaySceneScenario::OnDisable() {}

Vector2 GameplaySceneScenario::SunflowerPattern(int n, float seedRadius) {
	float angle = Mathf::pi * 2 / Mathf::Pow(Mathf::phi, 2) * n;
	float radius = seedRadius * Mathf::Sqrt(n) * seedRadius;

	return Vector2(Mathf::Sin(angle), Mathf::Cos(angle)) * radius;
}
void GameplaySceneScenario::SpawnZombies(std::shared_ptr<GameObject> prefab, Vector3 centerPos, int amount) {
	for (int i = 0; i < amount; i++) {
		auto sun = SunflowerPattern(i, 1.f);
		auto pos = centerPos + Vector3(sun.x, 0.f, sun.y);
		auto go = Object::Instantiate(prefab);
		go->transform()->SetPosition(pos);
		auto rot = Quaternion::FromAngleAxis(Random::Range(0.f, Mathf::pi * 2), Vector3_up);
		go->transform()->SetRotation(rot);
		Scene::Get()->AddGameObject(go);
	}
}

bool GameplaySceneScenario::TriggeredSphere(Vector3 center, float radius, std::string name) {
	bool wasTriggered = std::find(triggeredSpheres.begin(), triggeredSpheres.end(), name) != triggeredSpheres.end();
	if (wasTriggered) {
		return false;
	}
	auto player = Scene::Get()->FindGameObjectByTag("Player");
	if (Vector3::Distance(player->transform()->GetPosition(), center) < radius) {
		triggeredSpheres.push_back(name);
		return true;
	}
	return false;
}
