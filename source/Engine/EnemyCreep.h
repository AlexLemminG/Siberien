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
private:
	void HandleDeath();

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
	float velocityToAnimatorSpeed = 1.f;
	bool wasDead = false;
	float lastRollTime = 0.f;

	REFLECT_BEGIN(EnemyCreepController);
	REFLECT_VAR(targetTag);
	REFLECT_VAR(speed);
	REFLECT_VAR(acc);
	REFLECT_VAR(angularAcc);
	REFLECT_VAR(angularSpeed);
	REFLECT_VAR(walkAnimation);
	REFLECT_VAR(rollAnimation);
	REFLECT_VAR(velocityToAnimatorSpeed);
	REFLECT_END();
};