#pragma once

#include "Component.h"
#include "Gun.h"
#include "MeshRenderer.h"
#include "Grenade.h"
#include "Sound.h"

class RigidBody;
class PostProcessingEffect;

class PlayerController : public Component {
public:
	void OnEnable()override;
	void FixedUpdate() override;
	void Update() override;
	void AddGun(std::shared_ptr<Gun> gunTemplate);
	void AddGrenades(int grenadesCount) { this->grenadesCount += grenadesCount; }
	void SetWon();
private:

	void UpdateMovement();
	void UpdateLook();
	void UpdateShooting();
	void UpdateGrenading();
	void UpdateZombiesAttacking();
	void Jump();
	bool CanJump();
	bool IsOnGround() { return true; } //TODO
	void OnDeath();

	void UpdateHealth();
	void SetRandomShootingLight();
	void DisableShootingLight();

	std::shared_ptr<Gun> GetCurrentGun();


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
	REFLECT_VAR(grenadePrefab);
	REFLECT_VAR(shootingSounds);
	REFLECT_VAR(footstepSounds);
	REFLECT_VAR(postprocessingEffect);
	REFLECT_END();

	std::vector<std::shared_ptr<AudioClip>> shootingSounds;
	std::vector<std::shared_ptr<AudioClip>> footstepSounds;

	Vector3 prevFootstepPos;

	std::shared_ptr<RigidBody> rigidBody = nullptr;
	std::shared_ptr<GameObject> shootingLightPrefab = nullptr;
	std::shared_ptr<GameObject> shootingLight = nullptr;
	std::shared_ptr<Gun> startGun;
	std::unordered_map< std::shared_ptr<Gun>, std::shared_ptr<Gun>> gunTemplateToAvailableGun;
	std::vector<std::shared_ptr<Gun>> guns;
	std::vector<std::shared_ptr<Gun>>& GetGuns() { return guns; }
	std::shared_ptr<GameObject> grenadePrefab;
	int grenadesCount = 0;
	std::shared_ptr<PostProcessingEffect> postprocessingEffect;
	bool isDead = false;
	float defaultSpeed = 1.f;
	bool won = false;
	float winTime = 0.f;
};