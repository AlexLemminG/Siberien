#pragma once

#include "Object.h"
#include "Math.h"



class Gun : public Object {
public:
	void PullTrigger() { isTriggerPulled = true; }
	void ReleaseTrigger() { isTriggerPulled = false; }

	void OnAfterDeserializeCallback(const SerializationContext& context) override {
		currentShotsInMagazine = shotsInMagazine;
		if (ammo < 0) {
			currentAmmo = INT_MAX;
		}
		else {
			currentAmmo = ammo;
		}
	}
	bool Update(const Matrix4& bulletSpawnMatrix);

	const Color& GetBulletColor() { return bulletColor; }
	bool HasAmmo() { return currentAmmo > 0; }

	void AddAmmo(int extraAmmo) {
		if (extraAmmo == INT_MAX || currentAmmo == INT_MAX) {
			currentAmmo = INT_MAX;
		}
		else {
			currentAmmo += extraAmmo;
		}
	}
	void Reload();
	int GetInitialAmmo() {
		return ammo;
	}
	int GetCurrentAmmo() {
		return currentAmmo;
	}
	int GetCurrentAmmoInMagazine() {
		return reloadingTime > 0.f ? currentShotsInMagazine : currentAmmo;
	}
	int GetCurrentAmmoNotInMagazine() {
		if (currentAmmo == INT_MAX) {
			return INT_MAX;
		}
		else {
			return currentAmmo - GetCurrentAmmoInMagazine();
		}
	}
private:
	bool isTriggerPulled = false;
	bool wasTriggerPulled = false;
	int currentShotsInMagazine = 0;
	float bulletReloadingTimer = 0.f;
	float magazineReloadingTimer = 0.f;
	int currentAmmo = 0;


	bool shootOnlyOnTrigger = false;
	float shotsPerSecond = 1.f;
	float reloadingTime = 0.f;
	int shotsInMagazine = 1;
	float spread = 0.f;
	float spreadPow = 1.f;
	float bulletSpeed = 1.f;
	int bulletsInShot = 1;
	Color bulletColor = Colors::white;

	int ammo = INT_MAX;

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
	REFLECT_VAR(ammo);
	REFLECT_END();
};