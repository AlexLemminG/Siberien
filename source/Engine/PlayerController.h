#pragma once

#include "Component.h"
#include "Gun.h"

class RigidBody;

class PlayerController : public Component {
public:
	void OnEnable()override;
	void FixedUpdate() override;
	void Update() override;
private:

	void SetGun(std::shared_ptr<Gun> gunTemplate);
	void UpdateMovement();
	void UpdateLook();
	void UpdateShooting();
	void UpdateZombiesAttacking();
	void Jump();
	bool CanJump();
	bool IsOnGround() { return true; } //TODO

	void UpdateHealth();
	void SetRandomShootingLight();
	void DisableShootingLight();

	float speed = 1.f;
	float bulletSpeed = 10.f;
	float jumpVelocity = 10.f;
	float lastJumpTime = 0.f;
	float jumpDelay = 0.25f;
	float jumpPushImpulse = 100.f;
	float jumpPushRadius = 5.f;
	float bulletSpawnOffset = 1.0f;
	Vector3 shootingLightOffset;
	float healPerSecond = 1.f;
	float healDelay = 5.f;
	float prevHealTime = -1000.f;


	REFLECT_BEGIN(PlayerController);
	REFLECT_VAR(speed);
	REFLECT_VAR(jumpVelocity);
	REFLECT_VAR(jumpPushImpulse);
	REFLECT_VAR(jumpPushRadius);
	REFLECT_VAR(bulletSpeed);
	REFLECT_VAR(bulletSpawnOffset);
	REFLECT_VAR(shootingLightPrefab);
	REFLECT_VAR(shootingLightOffset);
	REFLECT_VAR(healPerSecond);
	REFLECT_VAR(healDelay);
	REFLECT_VAR(startGun);
	REFLECT_END();

	std::shared_ptr<RigidBody> rigidBody = nullptr;
	std::shared_ptr<GameObject> shootingLightPrefab = nullptr;
	std::shared_ptr<GameObject> shootingLight = nullptr;
	std::shared_ptr<Gun> gun;
	std::shared_ptr<Gun> startGun;
};