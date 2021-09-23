#include "TopDownCameraRig.h"
#include "Transform.h"
#include "GameObject.h"
#include "STime.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "Physics.h"

DECLARE_TEXT_ASSET(TopDownCameraRig);

void TopDownCameraRig::OnEnable()
{
	auto target = GameObject::FindWithTag(targetTag);
	if (target != nullptr) {
		auto trans = gameObject()->transform();
		auto targetPos = target->transform()->GetPosition() + offset;

		trans->SetPosition(targetPos);
		currentPosWithoutCollision = trans->GetPosition();
	}
}

void TopDownCameraRig::Update() {
	auto target = GameObject::FindWithTag(targetTag);
	if (target != nullptr) {
		auto trans = gameObject()->transform();

		auto targetPos = target->transform()->GetPosition() + offset;

		currentPosWithoutCollision = (Mathf::Lerp(currentPosWithoutCollision, targetPos, Time::deltaTime() * lerpT));
		auto safePos = currentPosWithoutCollision;
		safePos.z = target->transform()->GetPosition().z;

		Ray ray{ safePos, currentPosWithoutCollision - safePos };
		Physics::RaycastHit hit;

		float radius = 0.5f;
		int layerMask = Physics::GetLayerCollisionMask("staticGeom");
		if (Physics::SphereCast(hit, ray, radius, (currentPosWithoutCollision - safePos).Length(), layerMask)) {
			targetPos = hit.GetPoint() + hit.GetNormal() * radius;
			trans->SetPosition(Mathf::Lerp(trans->GetPosition(), targetPos, Time::deltaTime() * collisionLerpT));
		}
		else {
			trans->SetPosition(Mathf::Lerp(trans->GetPosition(), currentPosWithoutCollision, Time::deltaTime() * collisionLerpT));
		}
	}
}
