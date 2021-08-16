#include "TopDownCameraRig.h"
#include "Transform.h"
#include "GameObject.h"
#include "Scene.h"
#include "Time.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "PhysicsSystem.h"

DECLARE_TEXT_ASSET(TopDownCameraRig);

void TopDownCameraRig::OnEnable()
{
	auto target = Scene::FindGameObjectByTag(targetTag);
	if (target != nullptr) {
		auto trans = gameObject()->transform();
		auto targetPos = target->transform()->GetPosition() + offset;

		trans->SetPosition(targetPos);
		currentPosWithoutCollision = trans->GetPosition();
	}
}

void TopDownCameraRig::Update() {
	auto target = Scene::FindGameObjectByTag(targetTag);
	if (target != nullptr) {
		auto trans = gameObject()->transform();

		auto targetPos = target->transform()->GetPosition() + offset;

		currentPosWithoutCollision = (Mathf::Lerp(currentPosWithoutCollision, targetPos, Time::deltaTime() * lerpT));
		auto safePos = currentPosWithoutCollision;
		safePos.z = target->transform()->GetPosition().z;

		float radius = 0.5f;
		btSphereShape sphereShape{ radius };
		btTransform from;
		from.setIdentity();
		btTransform to;
		to.setIdentity();

		btVector3 castFrom = btConvert(safePos);
		btVector3 castTo = btConvert(currentPosWithoutCollision);
		from.setOrigin(castFrom);
		to.setOrigin(castTo);
		btCollisionWorld::ClosestConvexResultCallback cb(from.getOrigin(), to.getOrigin());
		cb.m_collisionFilterMask = PhysicsSystem::playerMask;
		cb.m_collisionFilterGroup = PhysicsSystem::playerGroup;
		PhysicsSystem::Get()->dynamicsWorld->convexSweepTest(&sphereShape, from, to, cb);

		if (cb.hasHit()) {
			targetPos = btConvert(cb.m_hitPointWorld + cb.m_hitNormalWorld * radius);

			trans->SetPosition(Mathf::Lerp(trans->GetPosition(), targetPos, Time::deltaTime() * collisionLerpT));
		}
		else {
			trans->SetPosition(Mathf::Lerp(trans->GetPosition(), currentPosWithoutCollision, Time::deltaTime() * collisionLerpT));
		}
	}
}
