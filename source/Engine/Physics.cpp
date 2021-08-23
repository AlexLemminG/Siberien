#include "Physics.h"
#include "PhysicsSystem.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"

bool Physics::SphereCast(Physics::RaycastHit& hit, Ray ray, float radius, float maxDistance) {
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
	cb.m_collisionFilterMask = PhysicsSystem::playerMask;
	cb.m_collisionFilterGroup = PhysicsSystem::playerGroup;
	PhysicsSystem::Get()->dynamicsWorld->convexSweepTest(&sphereShape, from, to, cb);

	if (cb.hasHit()) {
		hit = Physics::RaycastHit(btConvert(cb.m_hitPointWorld), btConvert(cb.m_hitNormalWorld));
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
		hit = Physics::RaycastHit(btConvert(cb.m_hitPointWorld), btConvert(cb.m_hitNormalWorld));
		return true;
	}
	else {
		return false;
	}
}
