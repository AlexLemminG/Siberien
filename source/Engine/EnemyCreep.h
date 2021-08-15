#pragma once

#include "Component.h"

class RigidBody;
class Transform;
class Health;
class MeshAnimation;
class Animator;

class EnemyCreepController : public Component{
public:
	virtual void OnEnable() override;
	virtual void FixedUpdate() override;
	virtual void Update() override;
	void Attack();
private:
	void HandleDeath();
	void HandleSomeTimeAfterDeath();

	std::shared_ptr<RigidBody> rb = nullptr;
	std::shared_ptr<Animator> animator = nullptr;
	float speed = 1.f;
	float acc = 1.f;
	float angularAcc = 1.f;
	float angularSpeed = 1.f;
	std::string targetTag;
	std::shared_ptr<Health> health;
	std::shared_ptr<MeshAnimation> walkAnimation;
	std::shared_ptr<MeshAnimation> rollAnimation;
	std::shared_ptr<MeshAnimation> deadAnimation;
	std::shared_ptr<MeshAnimation> attackAnimation;
	float velocityToAnimatorSpeed = 1.f;
	bool wasDead = false;
	bool wasSomeTimeAfterDead = false;
	float lastRollTime = 0.f;
	float attackTimeLeft = 0.f;
	bool isReadyToAttack = false;
	bool didDamageFromAttack = false;
	float deadTimer = 0.f;
	Vector3 posAtDeath;

	REFLECT_BEGIN(EnemyCreepController);
	REFLECT_VAR(targetTag);
	REFLECT_VAR(speed);
	REFLECT_VAR(acc);
	REFLECT_VAR(angularAcc);
	REFLECT_VAR(angularSpeed);
	REFLECT_VAR(walkAnimation);
	REFLECT_VAR(attackAnimation);
	REFLECT_VAR(rollAnimation);
	REFLECT_VAR(deadAnimation);
	REFLECT_VAR(velocityToAnimatorSpeed);
	REFLECT_END();

	float clipPlaneY = -10.f;
};