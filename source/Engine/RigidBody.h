#pragma once

#include "Component.h"
#include "SMath.h"

class btDefaultMotionState;
class btRigidBody;

class RigidBody : public Component {
public:
	btRigidBody* GetHandle() { return pBody; }
	virtual void OnEnable() override;
	virtual void Update() override;
	virtual void OnDisable() override;

	std::string layer;
	float friction = 0.5f;
	bool isStatic = false;
	Vector3 centerOfMass = Vector3_zero;

	Vector3 GetLinearVelocity() const;
	void SetLinearVelocity(const Vector3& velocity);

	Vector3 GetAngularVelocity() const;
	void SetAngularVelocity(const Vector3& velocity);

	float GetLinearDamping() const;
	void SetLinearDamping(float damping);

	float GetAngularDamping() const;
	void SetAngularDamping(float damping);

	float GetFriction() const;
	void SetFriction(float friction);

	void OverrideWorldGravity(const Vector3& gravity);
	void UseWorldGravity();

	void SetAngularFactor(const Vector3& factor);

	void SetCenterOfMass(const Vector3& center);

	Vector3 GetCenterOfMass() const;

	Matrix4 GetTransform() const;
	void SetTransform(const Matrix4& transform);

	//Vector3 GetCurrentPosition() const { 
	//	return btConvert((pBody->getCenterOfMassTransform() * pMotionState->m_centerOfMassOffset).getOrigin()); 
	//}
	//void SetCurrentPosition(const Vector3& pos) {
	//	auto currentTrans = pBody->getCenterOfMassTransform();
	//	currentTrans.setOrigin(btConvert(pos) + pMotionState->m_centerOfMassOffset.getOrigin());
	//	pBody->setCenterOfMassTransform(currentTrans);
	//}
	//Quaternion GetCurrentRotation() const { 
	//	return btConvert((pBody->getCenterOfMassTransform() * pMotionState->m_centerOfMassOffset).getRotation()); 
	//}
	//void SetCurrentPosition(const Quaternion& rotation) {
	//	auto currentTrans = pBody->getCenterOfMassTransform();
	//	currentTrans.setRotation(btConvert(rotation) * pMotionState->m_centerOfMassOffset.getRotation());
	//	pBody->setCenterOfMassTransform(currentTrans);
	//}

	void Activate();
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
	REFLECT_VAR(centerOfMass);
	REFLECT_VAR(layer);
	REFLECT_END();
};