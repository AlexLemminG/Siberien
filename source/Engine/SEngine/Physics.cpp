

#include "Physics.h"
#include "PhysicsSystem.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "GameObject.h"
#include "RigidBody.h"
#include <BulletCollision\CollisionDispatch\btGhostObject.h>
#include "Asserts.h"

bool Physics::SphereCast(Physics::RaycastHit& hit, Ray ray, float radius, float maxDistance, int layerMask) {
	btSphereShape sphereShape{ radius };
	btTransform from;
	from.setIdentity();
	btTransform to;
	to.setIdentity();

	btVector3 castFrom = btConvert(ray.origin);
	btVector3 castTo = btConvert(ray.GetPoint(maxDistance));
	from.setOrigin(castFrom);
	to.setOrigin(castTo);
	btCollisionWorld::ClosestConvexResultCallback cb(from.getOrigin(), to.getOrigin());
	cb.m_collisionFilterMask = layerMask;
	PhysicsSystem::Get()->dynamicsWorld->convexSweepTest(&sphereShape, from, to, cb);

	if (cb.hasHit()) {
		hit = Physics::RaycastHit(btConvert(cb.m_hitPointWorld), btConvert(cb.m_hitNormalWorld), GetRigidBody(cb.m_hitCollisionObject));
		return true;
	}
	else {
		return false;
	}
}

bool Physics::Raycast(RaycastHit& hit, Ray ray, float maxDistance) {
	btCollisionWorld::ClosestRayResultCallback cb(btConvert(ray.origin), btConvert(ray.origin + ray.dir * maxDistance));
	PhysicsSystem::Get()->dynamicsWorld->rayTest(btConvert(ray.origin), btConvert(ray.origin + ray.dir * maxDistance), cb);
	if (cb.hasHit()) {
		hit = Physics::RaycastHit(btConvert(cb.m_hitPointWorld), btConvert(cb.m_hitNormalWorld), GetRigidBody(cb.m_collisionObject));
		return true;
	}
	else {
		return false;
	}
}


class GetAllContacts_ContactResultCallback : public btCollisionWorld::ContactResultCallback {
public:
	// Inherited via ContactResultCallback
	virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
	{
		objects.push_back(const_cast<btCollisionObject*>(colObj0Wrap->getCollisionObject()));
		return btScalar();
	}
	std::vector<btCollisionObject*> objects;
};


std::vector<RigidBody*> Physics::OverlapSphere(const Vector3& pos, float radius, int layerMask) {
	std::vector<RigidBody*> results;

	btSphereShape sphere(radius);
	btPairCachingGhostObject ghost;
	btTransform xform;
	xform.setOrigin(btConvert(pos));
	ghost.setCollisionShape(&sphere);
	ghost.setWorldTransform(xform);

	GetAllContacts_ContactResultCallback cb;
	cb.m_collisionFilterMask = layerMask;
	PhysicsSystem::Get()->dynamicsWorld->contactTest(&ghost, cb);

	//TODO may contain doubles 

	for (auto o : cb.objects) {
		auto* rb = o;
		if (rb) {
			auto rigidBody = GetRigidBody(rb);
			if (rigidBody) {
				results.push_back(rigidBody);
			}
			else {
				//TODO return not rigidbodies but gameObjects or colliders
				//ASSERT(false);
			}
		}
	}
	return results;
}

Vector3 Physics::GetGravity() {
	return btConvert(PhysicsSystem::Get()->dynamicsWorld->getGravity());
}

void Physics::SetGravity(const Vector3& gravity) {
	PhysicsSystem::Get()->dynamicsWorld->setGravity(btConvert(gravity));
}

RigidBody* Physics::GetRigidBody(const btCollisionObject* collisionObject) {
	if (!collisionObject) {
		return nullptr;
	}
	auto rb = (PhysicsBody*)collisionObject->getUserPointer();
	if (rb) {
		return rb->AsRigidBody();
	}
	else {
		return nullptr;
	}
}
int Physics::GetLayerCollisionMask(const std::string& layer) {
	int group;
	int mask;
	PhysicsSystem::Get()->GetGroupAndMask(layer, group, mask);
	return mask;
}
int Physics::GetLayerAsMask(const std::string& layer) {
	int group;
	int mask;
	PhysicsSystem::Get()->GetGroupAndMask(layer, group, mask);
	return group;
}

std::shared_ptr<GameObject> Physics::GetGameObject(const btCollisionObject* collisionObject) {
	if (!collisionObject) {
		return nullptr;
	}
	auto rb = (PhysicsBody*)collisionObject->getUserPointer();
	if (rb) {
		return rb->gameObject();
	}
	else {
		return nullptr;
	}
}
