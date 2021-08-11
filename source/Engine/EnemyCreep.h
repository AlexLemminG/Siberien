#pragma once

#include "Component.h"

class RigidBody;
class Transform;
class Health;

class EnemyCreepController : public Component{
public:
	virtual void OnEnable() override;
	virtual void FixedUpdate() override;
private:
	void HandleDeath();

	std::shared_ptr<RigidBody> rb = nullptr;
	float speed = 1.f;
	float acc = 1.f;
	float angularAcc = 1.f;
	float angularSpeed = 1.f;
	std::string targetTag;
	std::shared_ptr<Health> health;
	bool wasDead = false;

	REFLECT_BEGIN(EnemyCreepController);
	REFLECT_VAR(targetTag);
	REFLECT_VAR(speed);
	REFLECT_VAR(acc);
	REFLECT_VAR(angularAcc);
	REFLECT_VAR(angularSpeed);
	REFLECT_END();
};