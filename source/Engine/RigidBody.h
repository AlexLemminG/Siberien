#pragma once

#include "Component.h"

class btDefaultMotionState;
class btRigidBody;

class RigidBody : public Component{
public:
	btRigidBody* GetHandle() { return pBody; }
private:
	virtual void OnEnable() override;
	virtual void Update() override;
	virtual void OnDisable() override;

	btDefaultMotionState* pMotionState = nullptr;
	btRigidBody* pBody = nullptr;

	bool isStatic = false;
	bool isKinematic = false;
	float mass = 1.f;
	float friction = 0.5f;
	float restitution = 0.f;
	REFLECT_BEGIN(RigidBody);
	REFLECT_VAR(mass);
	REFLECT_VAR(isStatic);
	REFLECT_VAR(isKinematic);
	REFLECT_VAR(friction);
	REFLECT_VAR(restitution);
	REFLECT_END();
};