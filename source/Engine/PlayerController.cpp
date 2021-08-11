#include "PlayerController.h"

#include "GameObject.h"
#include "Transform.h"
#include "Input.h"
#include "Time.h"
#include "Camera.h"
#include "Dbg.h"
#include "BulletSystem.h"
#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include "RigidBody.h"
#include "Scene.h"
#include <BulletCollision\CollisionDispatch\btGhostObject.h>

void PlayerController::OnEnable() {
	rigidBody = gameObject()->GetComponent<RigidBody>();
	if (shootingLightPrefab) {
		shootingLight = Object::Instantiate(shootingLightPrefab);
		Scene::Get()->AddGameObject(shootingLight);
	}
}

void PlayerController::Update() {
	UpdateShooting();

	if (CanJump()) {
		if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_SPACE)) {
			Jump();
		}
	}
}

void PlayerController::FixedUpdate() {
	UpdateMovement();
	UpdateLook();

	if (rigidBody) {
		float gravityMultiplier = 1.f;
		float upVel = rigidBody->GetHandle()->getLinearVelocity().getY();
		if (upVel < 0.f) {
			gravityMultiplier = Mathf::Lerp(1.f, 2.f, -upVel / 1.f);
		}
		rigidBody->GetHandle()->setGravity(btConvert(PhysicsSystem::Get()->GetGravity() * gravityMultiplier));
	}
	rigidBody->GetHandle()->setAngularFactor(btVector3(0, 1, 0));
}

void PlayerController::UpdateMovement() {
	if (!rigidBody) {
		return;
	}
	rigidBody->GetHandle()->activate(true);
	Matrix4 matrix = gameObject()->transform()->matrix;
	auto rotation = GetRot(matrix);

	Vector3 deltaPos = Vector3_zero;
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_W)) {
		deltaPos += Vector3_forward;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_S)) {
		deltaPos -= Vector3_forward;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_A)) {
		deltaPos -= Vector3_right;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_D)) {
		deltaPos += Vector3_right;
	}
	deltaPos = Mathf::ClampLength(deltaPos, 1.f);

	deltaPos *= speed;
	//*Time::deltaTime();

	auto vel = rigidBody->GetHandle()->getLinearVelocity();
	vel.setX(deltaPos.x);
	vel.setZ(deltaPos.z);

	rigidBody->GetHandle()->setLinearVelocity(vel);
}

void PlayerController::UpdateShooting() {
	if (!Input::GetKey(SDL_SCANCODE_Z)) {//TODO
		DisableShootingLight();
		return;
	}

	auto pos = gameObject()->transform()->GetPosition() + Vector3(0, bulletSpawnOffset, 0);
	auto rot = GetRot(gameObject()->transform()->matrix);

	BulletSystem::Get()->CreateBullet(pos, rot * Vector3_forward * bulletSpeed);

	SetRandomShootingLight();
}


void PlayerController::UpdateLook() {
	Matrix4 matrix = btConvert(rigidBody->GetHandle()->getCenterOfMassTransform());

	Vector3 playerPos = GetPos(matrix);

	if (!Camera::GetMain()) {
		return;
	}

	auto ray = Camera::GetMain()->ScreenPointToRay(Input::GetMousePosition());
	auto plane = Plane{ playerPos , Vector3_up };

	float dist;
	if (!plane.Raycast(ray, dist)) {
		return;
	}


	btCollisionWorld::ClosestRayResultCallback cb(btConvert(ray.origin), btConvert(ray.origin + ray.dir * 100.f));
	PhysicsSystem::Get()->dynamicsWorld->rayTest(btConvert(ray.origin), btConvert(ray.origin + ray.dir * 100.f), cb);

	if (cb.hasHit()) {
		Dbg::Draw(btConvert(cb.m_hitPointWorld), 1);
	}
	else {
		Dbg::Draw(ray.GetPoint(dist));

	}

	auto deltaPos = ray.GetPoint(dist) - playerPos;

	if (deltaPos.Length() < 0.1f) {
		return;
	}

	SetRot(matrix, Quaternion::LookAt(deltaPos, Vector3_up));
	//gameObject()->transform()->matrix = matrix;
	rigidBody->GetHandle()->setCenterOfMassTransform(btConvert(matrix));

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
static std::vector<btRigidBody*> GetOverlaping(Vector3 pos, float radius) {
	std::vector<btRigidBody*> results;

	btSphereShape sphere(radius);
	btPairCachingGhostObject ghost;
	btTransform xform;
	xform.setOrigin(btConvert(pos));
	ghost.setCollisionShape(&sphere);
	ghost.setWorldTransform(xform);

	GetAllContacts_ContactResultCallback cb;
	cb.m_collisionFilterGroup = PhysicsSystem::playerBulletGroup;
	cb.m_collisionFilterMask = PhysicsSystem::playerBulletMask;
	PhysicsSystem::Get()->dynamicsWorld->contactTest(&ghost, cb);

	//TODO may contain doubles 

	for (auto o : cb.objects) {
		auto* rb = dynamic_cast<btRigidBody*>(o);
		if (rb) {
			results.push_back(rb);
		}
	}
	PhysicsSystem::Get()->dynamicsWorld->removeCollisionObject(&ghost);

	return results;

}

void PlayerController::Jump() {
	if (!rigidBody) {
		return;
	}
	if (!CanJump()) {
		return;
	}

	lastJumpTime = Time::time();

	auto vel = rigidBody->GetHandle()->getLinearVelocity();
	vel.setY(jumpVelocity);
	rigidBody->GetHandle()->setLinearVelocity(vel);


	auto pos = rigidBody->GetHandle()->getWorldTransform().getOrigin();
	auto bodies = GetOverlaping(btConvert(pos), jumpPushRadius);
	for (auto body : bodies) {
		if (body == rigidBody->GetHandle()) {
			continue;
		}
		auto bodyPos = body->getCenterOfMassPosition();
		auto deltaPos = bodyPos - pos;
		float impulseMag = Mathf::Lerp(jumpPushImpulse, 0.f, deltaPos.length2() / (jumpPushRadius * jumpPushRadius));
		auto impulse = deltaPos.normalized() * impulseMag;
		body->applyCentralImpulse(impulse);
	}
}

bool PlayerController::CanJump() {
	if (Time::time() - lastJumpTime < jumpDelay) {
		return false;
	}
	if (!IsOnGround()) {
		return false;
	}
	return true;
}

void PlayerController::SetRandomShootingLight() {
	if (!shootingLight) {
		return;
	}
	auto t = gameObject()->transform();
	auto lightPos = t->matrix * shootingLightOffset;
	shootingLight->GetComponent<Transform>()->SetPosition(lightPos);
	auto color = shootingLightPrefab->GetComponent<PointLight>()->color;
	float colorM = Random::Range(0.7f, 1.3f);
	color.r *= colorM;
	color.g *= colorM;
	color.b *= colorM;
	auto radius = shootingLightPrefab->GetComponent<PointLight>()->radius;
	radius *= Random::Range(0.7f, 1.2f);
	shootingLight->GetComponent<PointLight>()->color = color;
	shootingLight->GetComponent<PointLight>()->radius = radius;
}

void PlayerController::DisableShootingLight() {
	if (!shootingLight) {
		return;
	}
	shootingLight->GetComponent<PointLight>()->color = Colors::black;
	shootingLight->GetComponent<PointLight>()->radius = 0.f;
}

DECLARE_TEXT_ASSET(PlayerController);