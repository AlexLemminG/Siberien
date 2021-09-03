#include "BoxCollider.h"
#include "btBulletCollisionCommon.h"
#include "PhysicsSystem.h"
#include "SMath.h"
#include "Transform.h"
#include "GameObject.h"
#include "Mesh.h"
#include "assimp/mesh.h"

DECLARE_TEXT_ASSET(BoxCollider);
DECLARE_TEXT_ASSET(SphereCollider);

std::shared_ptr<btCollisionShape> BoxCollider::CreateShape() {
	auto compound = std::make_shared<btCompoundShape>();

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


std::shared_ptr<btCollisionShape> SphereCollider::CreateShape() {
	auto compound = std::make_shared<btCompoundShape>();

	auto trans = gameObject()->transform();
	Vector3 scale = GetScale(trans->matrix);
	float realRadius = radius;
	realRadius = realRadius * Mathf::Max(scale.x, Mathf::Max(scale.y, scale.z));

	Vector3 realCenter = scale * center;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btConvert(realCenter));

	//TODO move random somewhere else
	sphereShape.reset(new btSphereShape(realRadius));
	compound->addChildShape(transform, sphereShape.get());
	return compound;
}