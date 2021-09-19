#pragma once

#include "Component.h"
#include "SMath.h"


class SE_CPP_API NavMeshAgent : public Component {
public:
	void SetDestination(const Vector3& destination);
	void SetVelocity(const Vector3& velocity);
	virtual void OnEnable() override;
	virtual void Update() override;
	virtual void OnDisable() override;

	float GetMaxSpeed()const {
		return maxSpeed;
	}
private:
	void UpdatePosition();
	void UpdateRotation();

	bool hasDestination = false;
	Vector3 destination;

	int agentId = 0;

	float maxSpeed = 5.f;
	float maxAcceleration = 8.f;
	float maxAngularSpeed = 180;

	float currentAngleXZ = 0.f;

	REFLECT_BEGIN(NavMeshAgent);
	REFLECT_VAR(maxSpeed);
	REFLECT_VAR(maxAcceleration);
	REFLECT_VAR(maxAngularSpeed);
	REFLECT_END();

};