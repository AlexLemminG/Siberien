#pragma once

#include "Component.h"

class RigidBody;
class Transform;

class EnemyCreepController : public Component{
public:
	virtual void OnEnable() override;
	virtual void FixedUpdate() override;
private:
	std::shared_ptr<RigidBody> rb = nullptr;
	float speed = 1.f;
	float acc = 1.f;
	float angularAcc = 1.f;
	std::string targetTag;

	REFLECT_BEGIN(EnemyCreepController);
	REFLECT_VAR(targetTag);
	REFLECT_VAR(speed);
	REFLECT_VAR(acc);
	REFLECT_VAR(angularAcc);
	REFLECT_END();
};