#include "RigidBody.h"
#include "GameObject.h"
#include "Transform.h"
#include "Collider.h"
#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"

DECLARE_TEXT_ASSET(RigidBody);

void RigidBody::OnEnable() {
	auto collider = gameObject()->GetComponent<Collider>();
	if (!collider) {
		return;
	}

	auto shape = collider->shape;
	if (!shape) {
		return;
	}

	auto transform = gameObject()->transform();
	if (!transform) {
		return;
	}

	btTransform groundTransform = btConvert(transform->matrix);

	btScalar mass = isStatic ? 0.f : Mathf::Max(0.001f, this->mass);

	btVector3 localInertia(0, 0, 0);
	if (!isStatic) {
		shape->calculateLocalInertia(mass, localInertia);
	}


	//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
	pMotionState = new btDefaultMotionState(groundTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, pMotionState, shape, localInertia);
	rbInfo.m_friction = friction;
	rbInfo.m_restitution = restitution;
	pBody = new btRigidBody(rbInfo);

	if (isKinematic) {
		pBody->forceActivationState(DISABLE_DEACTIVATION);
		pBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
	}

	//add the body to the dynamics world
	PhysicsSystem::Get()->dynamicsWorld->addRigidBody(pBody);
}

void RigidBody::Update() {
	if (!isStatic) {
		auto matr = btConvert(pMotionState->m_graphicsWorldTrans);
		gameObject()->transform()->matrix = matr;
	}
	else if (isKinematic) {
		auto trans = btConvert(gameObject()->transform()->matrix);
		pMotionState->m_graphicsWorldTrans = trans;
		//pBody->setCenterOfMassTransform(trans);
	}
}

void RigidBody::OnDisable() {
	if (pBody) {
		PhysicsSystem::Get()->dynamicsWorld->removeCollisionObject(pBody);
	}
	SAFE_DELETE(pMotionState);
	SAFE_DELETE(pBody)
}