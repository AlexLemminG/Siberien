#include "EnemyCreep.h"
#include "Scene.h"
#include "GameObject.h"
#include "RigidBody.h"
#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include "Time.h"

DECLARE_TEXT_ASSET(EnemyCreepController);

void EnemyCreepController::OnEnable() {
	rb = gameObject()->GetComponent<RigidBody>();
}

void EnemyCreepController::FixedUpdate() {
	if (targetTag == "") {
		return;
	}
	//TODO not so fast
	auto target = Scene::FindGameObjectByTag(targetTag);
	float dt = Time::fixedDeltaTime(); //TODO make just delta Time later
	//TODO why strange beh on freezes

	if (!target) {
		return;
	}

	auto handle = rb->GetHandle();

	handle->activate(true);

	auto targetPos = target->transform()->GetPosition();

	auto currentPos = gameObject()->transform()->GetPosition();

	Vector3 desiredLookDir = (targetPos - currentPos).Normalized();
	Vector3 lookDir = gameObject()->transform()->GetForward();
	Vector3 desiredAngularVelocity = Vector3::CrossProduct(lookDir, desiredLookDir);
	Vector3 currentAngularVelocity = btConvert(handle->getAngularVelocity());

	float maxAnglularSpeedDistance = 0.5f;
	Vector3 angularVelocity = currentAngularVelocity + Mathf::ClampLength(desiredAngularVelocity - currentAngularVelocity, maxAnglularSpeedDistance) / maxAnglularSpeedDistance * angularAcc * dt;
	handle->setAngularVelocity(btConvert(angularVelocity));

	float speedFactorFromAngle = Mathf::Max(0.8f, Vector3::DotProduct(desiredLookDir, lookDir));
	float moveForwardFactor = (1.f - speedFactorFromAngle) * 40.f;


	float maxSpeedDistance = 3.f;
	Vector3 desiredVelocity = Mathf::ClampLength(targetPos - currentPos, maxSpeedDistance) / maxSpeedDistance * speed;
	desiredVelocity += moveForwardFactor * lookDir;
	desiredVelocity = Mathf::ClampLength(desiredVelocity, speed);

	Vector3 currentVelocity = btConvert(handle->getLinearVelocity());

	Vector3 velocity = currentVelocity + Mathf::ClampLength(desiredVelocity - currentVelocity, maxSpeedDistance) / maxSpeedDistance * acc * dt * speedFactorFromAngle;

	handle->setLinearVelocity(btConvert(velocity));
	//Vector3 dir = target
}
