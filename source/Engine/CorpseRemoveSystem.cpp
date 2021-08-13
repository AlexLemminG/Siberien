#include "CorpseRemoveSystem.h"
#include "Scene.h"

REGISTER_SYSTEM(CorpseRemoveSystem);

void CorpseRemoveSystem::Update() {
	int corpsesCountToRemove = corpses.size() - maxCorpsesCount;

	if (corpsesCountToRemove <= 0) {
		return;
	}
	//TODO remove not last, but random ?
	for (int i = 0; i < corpsesCountToRemove; i++) {
		Scene::Get()->RemoveGameObject(corpses[i]);
	}
	corpses.erase(corpses.begin(), corpses.begin() + corpsesCountToRemove);
}
