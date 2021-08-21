#include "EnemyCreep.h"
#include "Scene.h"
#include "GameObject.h"
#include "RigidBody.h"
#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include "Time.h"
#include "Health.h"
#include "MeshRenderer.h"
#include "CorpseRemoveSystem.h"
#include "BoxCollider.h"
#include "Animation.h"
#include "GameEvents.h"

DECLARE_TEXT_ASSET(EnemyCreepController);

void EnemyCreepController::OnEnable() {
	rb = gameObject()->GetComponent<RigidBody>();
	animator = gameObject()->GetComponent<Animator>();
	health = gameObject()->GetComponent<Health>();

	auto tr = gameObject()->transform();
	//tr->SetScale(tr->GetScale() * Random::Range(0.8f, 1.2f));
}


void EnemyCreepController::HandleSomeTimeAfterDeath() {
	posAtDeath = gameObject()->transform()->GetPosition();
}
void EnemyCreepController::HandleDeath() {
	animator->SetAnimation(deadAnimation);
	CorpseRemoveSystem::Get()->Add(gameObject());

	auto collider = gameObject()->GetComponent<SphereCollider>();
	auto center = collider->center;
	auto angVel = rb->GetHandle()->getAngularVelocity();
	auto linVel = rb->GetHandle()->getLinearVelocity();
	rb->SetEnabled(false);
	collider->SetEnabled(false);

	float radiusScale = 0.9f;

	center.z = -center.y;
	center.y = 0;
	rb->layer = "enemyCorpse";
	collider->center = center * radiusScale;
	collider->radius *= radiusScale;
	rb->friction = 0.7f;
	rb->localOffset = Vector3(0, -0.4, 0);

	collider->SetEnabled(true);
	rb->SetEnabled(true);

	rb->GetHandle()->setDamping(0.5f, 0.8f);
	rb->GetHandle()->setLinearVelocity(linVel);
	rb->GetHandle()->setAngularVelocity(angVel);

	GameEvents::Get()->creepDeath.Invoke(this);
}


void EnemyCreepController::Update() {
	if (attackTimeLeft > 0.f) {
		attackTimeLeft -= Time::deltaTime();
		if (attackTimeLeft <= 0.3f && !didDamageFromAttack) {
			didDamageFromAttack = true;
			auto target = Scene::FindGameObjectByTag(targetTag);
			if (target) {
				auto health = target->GetComponent<Health>();
				if (health) {
					health->DoDamage(damage);
				}
			}
		}
	}
	if (health && health->IsDead()) {
		if (!wasDead) {
			wasDead = true;
			deadTimer = 3.f;
			HandleDeath();
		}
		if (!wasSomeTimeAfterDead) {
			deadTimer -= Time::deltaTime();
			if (deadTimer <= 0.f) {
				deadTimer = 6.f;
				wasSomeTimeAfterDead = true;
				HandleSomeTimeAfterDeath();
			}
		}
		else {
			if (deadTimer > 0.f) {
				deadTimer -= Time::deltaTime();
				if (deadTimer < 3.f) {
					gameObject()->transform()->SetPosition(posAtDeath - (3.f - deadTimer) * Vector3_up / 3.f * 0.2f);
					rb->SetEnabled(false);
				}
				else {
					posAtDeath = gameObject()->transform()->GetPosition();
				}
			}
		}
		return;
	}
}

void EnemyCreepController::FixedUpdate() {
	if (wasDead) {
		return;
	}
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

	if (currentPos.y < clipPlaneY&& health) {
		health->DoDamage(health->GetAmount());
	}

	Vector3 desiredLookDir = (targetPos - currentPos).Normalized();
	Vector3 lookDir = gameObject()->transform()->GetForward().Normalized();
	Vector3 desiredAngularVelocity = Vector3::CrossProduct(lookDir, desiredLookDir) * angularSpeed;
	if (Vector3::DotProduct(lookDir, desiredLookDir) < 0) {
		if (desiredAngularVelocity.LengthSquared() > 0.0001f) {
			desiredAngularVelocity = desiredAngularVelocity.Normalized() * angularSpeed;
		}
	}
	isReadyToAttack = Vector3::DotProduct(lookDir, desiredLookDir) > 0.6f;
	Vector3 currentAngularVelocity = btConvert(handle->getAngularVelocity());

	bool shouldRoll = Vector3::DotProduct(gameObject()->transform()->GetUp(), Vector3_up) < 0.1f;
	if (attackTimeLeft > 0.f && !shouldRoll) {
		return;
	}
	attackTimeLeft = 0.f;
	if (shouldRoll) {
		animator->SetAnimation(rollAnimation);
		lastRollTime = Time::time();
	}
	else {
		if (Time::time() - lastRollTime > 1.f) {
			animator->SetAnimation(walkAnimation);
		}
	}


	float maxAnglularSpeedDistance = 0.5f;
	Vector3 angularVelocity = currentAngularVelocity + Mathf::ClampLength(desiredAngularVelocity - currentAngularVelocity, maxAnglularSpeedDistance) / maxAnglularSpeedDistance * angularAcc * dt;
	handle->setAngularVelocity(btConvert(angularVelocity));

	float speedFactorFromAngle = Mathf::Max(0.8f, Vector3::DotProduct(desiredLookDir, lookDir));
	float moveForwardFactor = (1.f - speedFactorFromAngle) * 40.f * (shouldRoll ? 0.1f : 1.f);

	if (shouldRoll) {
		rb->GetHandle()->setDamping(0.1f, 0.4f);
	}
	else {
		rb->GetHandle()->setDamping(0.0f, 0.1f);
	}


	float maxSpeedDistance = 3.f;
	Vector3 desiredVelocity = Mathf::ClampLength(targetPos - currentPos, maxSpeedDistance) / maxSpeedDistance * speed;
	desiredVelocity += moveForwardFactor * lookDir;
	desiredVelocity = Mathf::ClampLength(desiredVelocity, speed);

	Vector3 currentVelocity = btConvert(handle->getLinearVelocity());

	Vector3 velocity = currentVelocity + Mathf::ClampLength(desiredVelocity - currentVelocity, maxSpeedDistance) / maxSpeedDistance * acc * dt * speedFactorFromAngle;
	animator->speed = Mathf::Max(1.f, currentVelocity.Length() * velocityToAnimatorSpeed);

	handle->setLinearVelocity(btConvert(velocity));
	//Vector3 dir = target
}

void EnemyCreepController::Attack() {
	if (attackTimeLeft > 0 || Time::time() - lastRollTime < 1.f || !isReadyToAttack || wasDead) {
		return;
	}
	animator->SetAnimation(attackAnimation);
	animator->currentTime = 0.f;
	animator->speed = 1.f;
	attackTimeLeft = 0.7f;
	didDamageFromAttack = false;
}
