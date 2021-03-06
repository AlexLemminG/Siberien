#include "Gun.h"
#include "STime.h"
#include "BulletSystem.h"
#include "Dbg.h"

DECLARE_TEXT_ASSET(Gun);

void Gun::Reload() {
	if (currentShotsInMagazine < shotsInMagazine && GetCurrentAmmoNotInMagazine() > 0) {
		magazineReloadingTimer = reloadingTime;
	}
}

bool Gun::Update(const Matrix4& bulletSpawnMatrix) {
	bool isJustPulled = !wasTriggerPulled && isTriggerPulled;
	wasTriggerPulled = isTriggerPulled;

	if (magazineReloadingTimer > 0.f) {
		magazineReloadingTimer -= Time::deltaTime();
		if (magazineReloadingTimer <= 0.f) {
			bulletReloadingTimer = 0.f;
			currentShotsInMagazine = Mathf::Min(shotsInMagazine, currentAmmo);
		}
		else {
			Dbg::Text("Reloading: %d", (int)magazineReloadingTimer);
			return false;
		}
	}
	if (bulletReloadingTimer > 0.f) {
		bulletReloadingTimer -= Time::deltaTime();
		if (bulletReloadingTimer > 0.f) {
			return false;
		}
	}

	if (!isJustPulled && shootOnlyOnTrigger) {
		return false;
	}
	if (!isTriggerPulled) {
		return false;
	}
	if (currentAmmo <= 0) {
		return false;
	}

	for (int i = 0; i < bulletsInShot; i++) {
		auto pos = GetPos(bulletSpawnMatrix);
		float spreadPercent = Mathf::Pow(Random::Range(0.f, 1.f), spreadPow);
		auto randomRot = Quaternion::FromAngleAxis(Random::Range(0.f, Mathf::pi2), Vector3_forward) * Quaternion::FromEulerAngles(Mathf::DegToRad(spreadPercent * spread / 2.f), 0.f, 0.f);
		auto localDir = randomRot * Vector3_forward;
		localDir.y /= 5.f;
		auto dir = GetRot(bulletSpawnMatrix) * localDir * bulletSpeed;

		BulletSystem::Get()->CreateBullet(pos, dir, bulletColor);
		bulletReloadingTimer = 1.f / Mathf::Max(shotsPerSecond, Mathf::epsilon);
	}
	if (currentAmmo != INT_MAX) {
		currentAmmo--;
	}
	currentShotsInMagazine--;
	if (currentShotsInMagazine <= 0) {
		magazineReloadingTimer = reloadingTime;
	}
	return true;
}
