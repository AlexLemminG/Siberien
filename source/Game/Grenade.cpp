#include "Grenade.h"
#include "STime.h"
#include "Scene.h"
#include "RigidBody.h"
#include "GameObject.h"
#include "Health.h"
#include "Physics.h"

void Grenade::ThrowAt(Vector3 pos) {

	throwTarget = pos;
}

void Grenade::FlyAt(Vector3 pos) {}

void Grenade::OnEnable() {
	if(explodeOnCollision){
		explodeTimer = FLT_MAX;
	}
	else {
		explodeTimer = explodeDelay;
	}

	float t = explodeTimer;
	Vector3 g = Physics::GetGravity();
	Vector3 pos0 = gameObject()->transform()->GetPosition();
	Vector3 pos = throwTarget;
	Vector3 v = (pos - pos0 - g * t * t / 2.f) / t;
	auto dxLength = Vector2(v.x, v.z).Length();
	v.y = Mathf::Min(v.y, dxLength);
	v = Mathf::ClampLength(v, startSpeed);

	gameObject()->GetComponent<RigidBody>()->SetLinearVelocity(v);
}
void Grenade::Update() {
	if (explodeOnCollision) {
		//TODO
	}
	else {
		explodeTimer -= Time::deltaTime();
		if (explodeTimer < 0.f) {
			Explode();
		}
	}
}

void Grenade::Explode() {
	auto pos = gameObject()->transform()->GetPosition();
	auto rbs = Physics::OveplapSphere(pos, radius);
	auto player = Scene::Get()->FindGameObjectByTag("Player");
	for (auto rb : rbs) {
		auto go = rb->gameObject();
		if (go == player) {
			continue;
		}
		auto bodyPos = rb->GetCenterOfMassWorld();
		auto deltaPos = bodyPos - pos;
		float impulseMag = Mathf::Lerp(pushImpulse, 0.f, deltaPos.LengthSquared() / (radius * radius));
		auto impulse = deltaPos.Normalized() * impulseMag;
		rb->ApplyLinearImpulse(impulse);

		auto health = go->GetComponent<Health>();
		if (health) {
			int currentDamage = Mathf::Round(damage * impulseMag / pushImpulse);
			health->DoDamage(currentDamage);
		}
	}

	Scene::Get()->RemoveGameObject(gameObject());
}

DECLARE_TEXT_ASSET(Grenade);
