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
DECLARE_TEXT_ASSET(CapsuleCollider);

AABB BoxCollider::GetAABBWithoutTransform() const {
	return AABB(center - size / 2.f, center + size / 2.f);
}

std::shared_ptr<btCollisionShape> BoxCollider::CreateShape() const {
	return std::make_shared<btBoxShape>(btConvert(size / 2.f));
}


std::shared_ptr<btCollisionShape> SphereCollider::CreateShape() const {
	return std::make_shared<btSphereShape>(radius);//WARN scale comes only from x axis here
}


std::shared_ptr<btCollisionShape> CapsuleCollider::CreateShape() const {
	return std::make_shared<btCapsuleShape>(radius, height);
}