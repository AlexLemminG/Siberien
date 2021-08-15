#include "Pickup.h"
#include "Scene.h"
#include "PlayerController.h"

void Pickup::Update() {
	if (isPicked) {
		return;
	}

	auto player = Scene::Get()->FindGameObjectByTag("Player");
	if (!player) {
		return;
	}
	auto playerController = player->GetComponent<PlayerController>();
	if (!playerController) {
		return;
	}

	auto playerPos = player->transform()->GetPosition();
	auto pos = gameObject()->transform()->GetPosition();

	Vector2 playerPos2d = Vector2(playerPos.x, playerPos.z);
	Vector2 pos2d = Vector2(pos.x, pos.z);
	if (Vector2::Distance(playerPos2d, pos2d) < 1.f) {
		isPicked = true;

		if (gun) {
			playerController->AddGun(gun);
		}
		if (grenadesCount > 0) {
			playerController->AddGrenades(grenadesCount);
		}

		Scene::Get()->RemoveGameObject(gameObject());
	}
}

DECLARE_TEXT_ASSET(Pickup);
DECLARE_TEXT_ASSET(RandomObjectSpawner);

void RandomObjectSpawner::Spawn(Vector3 pos) {
	if (availableObjects.size() == 0) {
		return;
	}

	float totalWeight = 0.f;
	for (auto& obj : availableObjects) {
		totalWeight += obj.weight;
	}
	if (totalWeight <= Mathf::epsilon) {
		return;
	}
	float t = Random::Range(0.f, totalWeight);
	float currentT = 0.f;
	bool found = false;
	std::shared_ptr<GameObject> prefab;
	for (int i = 0; i < availableObjects.size(); i++) {
		currentT += availableObjects[i].weight;
		if (t <= currentT) {
			prefab = availableObjects[i].prefab;
			found = true;
			break;
		}
	}
	if (!found) {
		prefab = availableObjects[availableObjects.size() - 1].prefab;
	}

	if (!prefab) {
		return;
	}
	auto go = Object::Instantiate(prefab);
	go->transform()->SetPosition(pos);
	Scene::Get()->AddGameObject(go);
}
