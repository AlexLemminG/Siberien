#include "RigidBody.h"
#include "GameObject.h"
#include "Transform.h"
#include "Collider.h"
#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include "Common.h"
#include "Camera.h"
#include "MeshRenderer.h"

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

	auto matr = transform->matrix;
	SetScale(matr, Vector3_one);
	btTransform groundTransform = btConvert(matr);


	btScalar mass = isStatic ? 0.f : Mathf::Max(0.001f, this->mass);

	btVector3 localInertia(0, 0, 0);
	if (!isStatic) {
		shape->calculateLocalInertia(mass, localInertia);
	}

	auto offset = btTransform(btMatrix3x3::getIdentity(), -btConvert(centerOfMass));

	//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
	pMotionState = new btDefaultMotionState(groundTransform, offset);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, pMotionState, shape.get(), localInertia);

	rbInfo.m_friction = friction;
	rbInfo.m_restitution = restitution;
	pBody = new btRigidBody(rbInfo);
	if (isKinematic) {
		pBody->forceActivationState(DISABLE_DEACTIVATION);
		pBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
	}

	pBody->setUserPointer(gameObject().get());

	//add the body to the dynamics world
	int group;
	int mask;
	PhysicsSystem::Get()->GetGroupAndMask(layer, group, mask);
	PhysicsSystem::Get()->dynamicsWorld->addRigidBody(pBody, group, mask);
}

void RigidBody::Update() {
	if (!isStatic) {
		auto matr = btConvert(pMotionState->m_graphicsWorldTrans);
		auto scale = GetScale(gameObject()->transform()->matrix);
		SetScale(matr, scale);
		//TODO not so persistent
		gameObject()->transform()->matrix = matr;
	}
	else if (isKinematic) {
		auto trans = btConvert(gameObject()->transform()->matrix);
		pMotionState->m_graphicsWorldTrans = trans;
		//pBody->setCenterOfMassTransform(trans);
	}
#ifdef SE_DBG_OUT
	if(false){
		auto camera = Camera::GetMain();
		if (camera) {
			btVector3 min;
			btVector3 max;
			pBody->getAabb(min, max);
			AABB aabb{ btConvert(min),btConvert(max) };
			if (camera->IsVisible(aabb)) {
				pBody->setCollisionFlags(Bits::SetMaskFalse(pBody->getCollisionFlags(), btCollisionObject::CollisionFlags::CF_DISABLE_VISUALIZE_OBJECT));
			}
			else {
				pBody->setCollisionFlags(Bits::SetMaskTrue(pBody->getCollisionFlags(), btCollisionObject::CollisionFlags::CF_DISABLE_VISUALIZE_OBJECT));
			}
		}
		else {
			pBody->setCollisionFlags(Bits::SetMaskTrue(pBody->getCollisionFlags(), btCollisionObject::CollisionFlags::CF_DISABLE_VISUALIZE_OBJECT));
		}
	}
#endif
}

void RigidBody::OnDisable() {
	if (pBody) {
		PhysicsSystem::Get()->dynamicsWorld->removeCollisionObject(pBody);
	}
	SAFE_DELETE(pMotionState);
	SAFE_DELETE(pBody)
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
	this->centerOfMass = center;
	//TODO offset body
	if (pMotionState) {
		pMotionState->m_centerOfMassOffset.setOrigin(btConvert(-centerOfMass));
	}
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

void RigidBody::Activate() { pBody->activate(); }
