#pragma once

#include "Component.h"

class RigidBody;

class PlayerController : public Component {
public:
	void OnEnable()override;
	void FixedUpdate() override;
	void Update() override;
private:

	void UpdateMovement();
	void UpdateLook();
	void UpdateShooting();
	void Jump();
	bool CanJump();
	bool IsOnGround() { return true; } //TODO

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


	REFLECT_BEGIN(PlayerController);
	REFLECT_VAR(speed);
	REFLECT_VAR(jumpVelocity);
	REFLECT_VAR(jumpPushImpulse);
	REFLECT_VAR(jumpPushRadius);
	REFLECT_VAR(bulletSpeed);
	REFLECT_VAR(bulletSpawnOffset);
	REFLECT_VAR(shootingLightPrefab);
	REFLECT_VAR(shootingLightOffset)
	REFLECT_END();

	std::shared_ptr<RigidBody> rigidBody = nullptr;
	std::shared_ptr<GameObject> shootingLightPrefab = nullptr;
	std::shared_ptr<GameObject> shootingLight = nullptr;
};