#pragma once

#include "Component.h"

class Grenade : public Component {
public:
	float radius = 1.f;
	float pushImpulse = 1.f;
	bool explodeOnCollision = false;
	float explodeDelay = -1.f;
	float startSpeed = 1.f;
	int damage = 1;

	void ThrowAt(Vector3 pos);
	void FlyAt(Vector3 pos);

	void OnEnable() override;
	void Update() override;
private:
	void Explode();
	float explodeTimer = 0.f;
	Vector3 throwTarget;

	REFLECT_BEGIN(Grenade);
	REFLECT_VAR(radius);
	REFLECT_VAR(pushImpulse);
	REFLECT_VAR(explodeOnCollision);
	REFLECT_VAR(explodeDelay);
	REFLECT_VAR(startSpeed);
	REFLECT_VAR(damage);
	REFLECT_END();
};