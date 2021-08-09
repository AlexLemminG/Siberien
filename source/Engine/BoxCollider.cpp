#include "BoxCollider.h"
#include "btBulletCollisionCommon.h"
#include "PhysicsSystem.h"
#include "Math.h"

DECLARE_TEXT_ASSET(BoxCollider);
DECLARE_TEXT_ASSET(SphereCollider);

btCollisionShape* BoxCollider::CreateShape() {
	auto* compound = new btCompoundShape();
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btConvert(center));

	boxShape.reset(new btBoxShape(btConvert(size / 2.f)));
	compound->addChildShape(transform, boxShape.get());
	return compound;
}


btCollisionShape* SphereCollider::CreateShape() {
	auto* compound = new btCompoundShape();
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btConvert(center));

	//TODO move random somewhere else
	sphereShape.reset(new btSphereShape(radius + Random::Range(-0.5f, 0.2f) * radius));
	compound->addChildShape(transform, sphereShape.get());
	return compound;
}
