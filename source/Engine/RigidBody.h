#pragma once

#include "Component.h"

class btDefaultMotionState;
class btRigidBody;

class RigidBody : public Component{
public:
	btRigidBody* GetHandle() { return pBody; }
	virtual void OnEnable() override;
	virtual void Update() override;
	virtual void OnDisable() override;

	std::string layer;
	float friction = 0.5f;
	bool isStatic = false;
	Vector3 localOffset = Vector3_zero;
private:

	btDefaultMotionState* pMotionState = nullptr;
	btRigidBody* pBody = nullptr;

	bool isKinematic = false;
	float mass = 1.f;
	float restitution = 0.f;
	REFLECT_BEGIN(RigidBody);
	REFLECT_VAR(mass);
	REFLECT_VAR(isStatic);
	REFLECT_VAR(isKinematic);
	REFLECT_VAR(friction);
	REFLECT_VAR(restitution);
	REFLECT_VAR(localOffset);
	REFLECT_VAR(layer);
	REFLECT_END();
};