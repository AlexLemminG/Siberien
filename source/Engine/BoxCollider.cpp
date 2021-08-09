#include "BoxCollider.h"
#include "btBulletCollisionCommon.h"
#include "PhysicsSystem.h"
#include "Math.h"
#include "Transform.h"
#include "GameObject.h"

DECLARE_TEXT_ASSET(BoxCollider);
DECLARE_TEXT_ASSET(SphereCollider);

btCollisionShape* BoxCollider::CreateShape() {
	auto* compound = new btCompoundShape();

	auto trans = gameObject()->transform();
	Vector3 scale = GetScale(trans->matrix);
	Vector3 realSize = size;
	realSize = realSize * scale;

	Vector3 realCenter = scale * center;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btConvert(realCenter));

	boxShape.reset(new btBoxShape(btConvert(realSize / 2.f)));
	compound->addChildShape(transform, boxShape.get());
	return compound;
}


btCollisionShape* SphereCollider::CreateShape() {
	auto* compound = new btCompoundShape();

	auto trans = gameObject()->transform();
	Vector3 scale = GetScale(trans->matrix);
	float realRadius = radius;
	realRadius = realRadius * Mathf::Max(scale.x, Mathf::Max(scale.y, scale.z));

	Vector3 realCenter = scale * center;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btConvert(realCenter));

	//TODO move random somewhere else
	sphereShape.reset(new btSphereShape(realRadius + Random::Range(-0.5f, 0.2f) * realRadius));
	compound->addChildShape(transform, sphereShape.get());
	return compound;
}
