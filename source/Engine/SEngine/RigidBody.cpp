

#include "RigidBody.h"
#include "GameObject.h"
#include "Transform.h"
#include "Collider.h"
#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "Common.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include "Editor.h"
#include "Dbg.h"

DECLARE_TEXT_ASSET(RigidBody);

RigidBody::RigidBody() { isRigidBody = true; }

void RigidBody::OnEnable() {
	auto collider = gameObject()->GetComponent<Collider>();
	if (!collider) {
		return;
	}
	//TODO multiple shapes
	auto shape = collider->CreateShape();
	if (!shape) {
		return;
	}
	transform = gameObject()->transform().get();
	if (!transform) {
		return;
	}
	auto scale = transform->GetScale();
	shape->setLocalScaling(btConvert(scale));

	originalShape = shape;
	//TODO remove intermediate shape when possible
	offsetedShape = std::make_shared<btCompoundShape>();
	offsetedShape->addChildShape(btTransform(btMatrix3x3::getIdentity(), -btConvert((centerOfMass - collider->GetCenterOffset()) * scale)), shape.get());
	shape = offsetedShape;

	auto matr = transform->GetMatrix();
	SetScale(matr, Vector3_one);//TODO optimize
	btTransform groundTransform = btConvert(matr);

	btScalar mass = isStatic ? 0.f : Mathf::Max(0.001f, this->mass);

	btVector3 localInertia(0, 0, 0);

	if (!isStatic) {
		shape->calculateLocalInertia(mass, localInertia);
	}

	auto offset = btTransform(btMatrix3x3::getIdentity(), -btConvert(centerOfMass * scale));
	//TODO apply offset to shape

	//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
	pMotionState = new btDefaultMotionState(groundTransform, offset);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, pMotionState, shape.get(), localInertia);

	rbInfo.m_friction = friction;
	rbInfo.m_restitution = restitution;
	pBody = new btRigidBody(rbInfo);
	if (isKinematic) {
		pBody->forceActivationState(DISABLE_DEACTIVATION);//TODO optimize
		pBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
	}

	pBody->setUserPointer(this);

	//add the body to the dynamics world
	int group;
	int mask;
	PhysicsSystem::Get()->GetGroupAndMask(layer, group, mask);
	PhysicsSystem::Get()->dynamicsWorld->addRigidBody(pBody, group, mask);

	bool isEditMode = Editor::Get()->IsInEditMode(); // PERF always either true or false
	SetFlags(Bits::SetMask(GetFlags(), FLAGS::IGNORE_UPDATE, isStatic && !isEditMode));
}

void RigidBody::Update() {
	if (pBody == nullptr) { // PERF required only because pBody can become nullptr after disable/enable
		return;
	}
	bool isEditMode = Editor::Get()->IsInEditMode(); // PERF called every frame!!!
	if (isEditMode) {
		auto trans = transform->GetMatrix();
		SetScale(trans, Vector3_one);//TODO optimize
		auto scale = transform->GetScale();
		btTransform offset = btTransform(btMatrix3x3::getIdentity(), btConvert(centerOfMass * scale));
		pBody->setWorldTransform(btConvert(trans) * offset);
		return;
	}
	if (isStatic) {
		return;
	}
	//TODO test isKinematic (it seems to be broken)
	if (isKinematic) {
		auto trans = transform->GetMatrix();
		SetScale(trans, Vector3_one);//TODO optimize
		pMotionState->m_graphicsWorldTrans = btConvert(trans);
	}
	else {
		auto matr = btConvert(pMotionState->m_graphicsWorldTrans);
		auto scale = transform->GetScale();
		SetScale(matr, scale);
		//TODO not so persistent
		transform->SetMatrix(matr);
	}

}

void RigidBody::OnDisable() {
	if (pBody) {
		PhysicsSystem::Get()->dynamicsWorld->removeRigidBody(pBody);
	}
	offsetedShape = nullptr;
	SAFE_DELETE(pMotionState);
	SAFE_DELETE(pBody)
}

void PhysicsBody::OnValidate() {
	if (!IsEnabled()) {
		return;
	}
	OnDisable();
	OnEnable();
}

void RigidBody::OnDrawGizmos()
{
	if (pBody == nullptr || PhysicsSystem::Get()->dynamicsWorld == nullptr) {
		return;
	}
	PhysicsSystem::Get()->dynamicsWorld->debugDrawObject(pBody->getWorldTransform(), pBody->getCollisionShape(), btVector3(0.4f, 1.f, 0.4f));
	Dbg::Draw(btConvert(pBody->getWorldTransform()));
}

Vector3 RigidBody::GetLinearVelocity() const { return btConvert(pBody->getLinearVelocity()); }

void RigidBody::SetLinearVelocity(const Vector3& velocity) { pBody->setLinearVelocity(btConvert(velocity)); }

Vector3 RigidBody::GetAngularVelocity() const { return btConvert(pBody->getAngularVelocity()); }

void RigidBody::SetAngularVelocity(const Vector3& velocity) { pBody->setAngularVelocity(btConvert(velocity)); }

float RigidBody::GetLinearDamping() const { return pBody->getLinearDamping(); }

void RigidBody::SetLinearDamping(float damping) { pBody->setDamping(damping, pBody->getAngularDamping()); }

float RigidBody::GetAngularDamping() const { return pBody->getAngularDamping(); }

void RigidBody::SetAngularDamping(float damping) { pBody->setDamping(pBody->getLinearDamping(), damping); }

float RigidBody::GetFriction() const { return pBody->getFriction(); }

void RigidBody::SetFriction(float friction) { pBody->setFriction(friction); }

void RigidBody::OverrideWorldGravity(const Vector3& gravity) {
	pBody->setFlags(pBody->getFlags() | BT_DISABLE_WORLD_GRAVITY);
	pBody->setGravity(btConvert(gravity));
}

void RigidBody::UseWorldGravity() {
	pBody->setGravity(PhysicsSystem::Get()->dynamicsWorld->getGravity());
	pBody->setFlags(pBody->getFlags() & (-1 ^ BT_DISABLE_WORLD_GRAVITY));
}

void RigidBody::SetAngularFactor(const Vector3& factor) { pBody->setAngularFactor(btConvert(factor)); }

void RigidBody::SetCenterOfMassLocal(const Vector3& center) {
	Vector3 deltaPos = (this->centerOfMass - center);
	this->centerOfMass = center;
	if (!pBody) {
		return;
	}
	auto scale = transform->GetScale();
	if (offsetedShape != nullptr) {
		offsetedShape->updateChildTransform(0, btTransform(btMatrix3x3::getIdentity(), -btConvert(centerOfMass * scale)));
	}
	//TODO offset body
	if (pMotionState) {
		pMotionState->m_centerOfMassOffset.setOrigin(btConvert(-centerOfMass * scale));
	}
	pBody->translate(btConvert(deltaPos * scale));
}

Vector3 RigidBody::GetCenterOfMassLocal() const {
	return centerOfMass;
}

Vector3 RigidBody::GetCenterOfMassWorld() const {
	return btConvert(pBody->getCenterOfMassTransform().getOrigin());
}

Matrix4 RigidBody::GetTransform() const { return btConvert(pBody->getCenterOfMassTransform() * pMotionState->m_centerOfMassOffset); }

void RigidBody::SetTransform(const Matrix4& transform) { pBody->setCenterOfMassTransform(btConvert(transform) * pMotionState->m_centerOfMassOffset.inverse()); }

void RigidBody::ApplyLinearImpulse(Vector3 impulse) {
	pBody->applyCentralImpulse(btConvert(impulse));
	pBody->activate();
}
void RigidBody::ApplyLinearImpulse(Vector3 impulse, Vector3 worldPos) {
	auto localPos = pBody->getCenterOfMassTransform().inverse() * btConvert(worldPos);
	pBody->applyImpulse(btConvert(impulse), localPos);
	pBody->activate();
}

float RigidBody::GetMass() const { return pBody->getMass(); }

void RigidBody::Activate() { pBody->activate(); }

RigidBody* PhysicsBody::AsRigidBody() {
	if (isRigidBody) {
		return (RigidBody*)this;
	}
	else {
		return nullptr;
	}
}

GhostBody* PhysicsBody::AsGhostBody() {
	if (!isRigidBody) {
		return (GhostBody*)this;
	}
	else {
		return nullptr;
	}
}

const RigidBody* PhysicsBody::AsRigidBody() const {
	if (isRigidBody) {
		return (RigidBody*)this;
	}
	else {
		return nullptr;
	}
}

const GhostBody* PhysicsBody::AsGhostBody() const {
	if (!isRigidBody) {
		return (GhostBody*)this;
	}
	else {
		return nullptr;
	}
}
