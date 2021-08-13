#pragma once

#include "Object.h"
#include "Math.h"

class Gun : public Object {
public:
	void PullTrigger() { isTriggerPulled = true; }
	void ReleaseTrigger() { isTriggerPulled = false; }

	void OnAfterDeserializeCallback(const SerializationContext& context) override {
		currentShotsInMagazine = shotsInMagazine;
	}
	bool Update(const Matrix4& bulletSpawnMatrix);

	const Color& GetBulletColor() { return bulletColor; }

private:
	bool isTriggerPulled = false;
	bool wasTriggerPulled = false;
	int currentShotsInMagazine = 0;
	float bulletReloadingTimer = 0.f;
	float magazineReloadingTimer = 0.f;


	bool shootOnlyOnTrigger = false;
	float shotsPerSecond = 1.f;
	float reloadingTime = 0.f;
	int shotsInMagazine = 1;
	float spread = 0.f;
	float spreadPow = 1.f;
	float bulletSpeed = 1.f;
	int bulletsInShot = 1;
	Color bulletColor = Colors::white;


	REFLECT_BEGIN(Gun);
	REFLECT_VAR(shootOnlyOnTrigger);
	REFLECT_VAR(shotsPerSecond);
	REFLECT_VAR(reloadingTime);
	REFLECT_VAR(shotsInMagazine);
	REFLECT_VAR(spread);
	REFLECT_VAR(spreadPow);
	REFLECT_VAR(bulletSpeed);
	REFLECT_VAR(bulletsInShot);
	REFLECT_VAR(bulletColor);
	REFLECT_END();
};