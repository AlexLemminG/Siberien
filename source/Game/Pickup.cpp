#include "Pickup.h"
#include "Scene.h"
#include "PlayerController.h"
#include "STime.h"
#include "GhostBody.h"

void Pickup::OnEnable() {
	ghost = gameObject()->GetComponent<GhostBody>().get();
}

void Pickup::Update() {
	auto trans = gameObject()->transform();
	trans->SetRotation(Quaternion::FromAngleAxis(Time::time() / Mathf::pi2 * 15.f, Vector3_up));

	if (isPicked) {
		return;
	}
	if (!ghost) {
		return;
	}

	std::shared_ptr< PlayerController> playerController;
	auto objs = ghost->GetOverlappedObjects();
	for (auto& go : objs) {
		playerController = go->GetComponent<PlayerController>();
		if (playerController != nullptr) {
			break;
		}
	}
	if (playerController == nullptr) {
		return;
	}

	isPicked = true;

	if (gun) {
		playerController->AddGun(gun);
	}
	if (grenadesCount > 0) {
		playerController->AddGrenades(grenadesCount);
	}

	Scene::Get()->RemoveGameObject(gameObject());
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
