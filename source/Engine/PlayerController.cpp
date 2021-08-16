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
#include "EnemyCreep.h"
#include "GameObject.h"
#include "Health.h"
#include "SceneManager.h"
#include "MeshRenderer.h"
#include "Sound.h"

void PlayerController::OnEnable() {
	rigidBody = gameObject()->GetComponent<RigidBody>();
	if (shootingLightPrefab) {
		shootingLight = Object::Instantiate(shootingLightPrefab);
		Scene::Get()->AddGameObject(shootingLight);
	}
	if (CfgGetBool("godMode")) {
		auto health = gameObject()->GetComponent<Health>();
		health->SetInvinsible(true);
	}
	defaultSpeed = speed;
	if (guns.size() == 0) {
		AddGun(startGun);
	}
}


void PlayerController::UpdateHealth() {
	auto health = gameObject()->GetComponent<Health>();
	//Dbg::Text("Player health: %d", health->GetAmount());

	if (Time::time() - health->GetLastDamageTime() > healDelay && !isDead) {
		if (Time::time() - prevHealTime > 1.f / healPerSecond) {
			prevHealTime = Time::time();
			health->DoHeal(1.f);
		}
	}
	bool wasDead = isDead;
	isDead = health->IsDead();
	if (!wasDead && isDead) {
		OnDeath();
	}

	if (postprocessingEffect) {
		if (isDead) {
			postprocessingEffect->intensity = 1.f;
			postprocessingEffect->intensityFromLastHit = 1.f;
		}
		else {
			float healthPercent = ((float)health->GetAmount()) / health->GetMaxAmount();
			postprocessingEffect->intensity = Mathf::Max(0.f, 1.f - healthPercent);

			float timeFromLastHit = Time::time() - health->GetLastDamageTime();
			postprocessingEffect->intensityFromLastHit = Mathf::Max(0.f, 1.f - Mathf::Clamp01(timeFromLastHit / .5f));
		}

		if (won) {
			postprocessingEffect->winScreenFade = Mathf::Clamp01(Time::time() - winTime);
		}
		else {
			postprocessingEffect->winScreenFade = 0.f;
		}
	}
}

void PlayerController::Update() {
	if (!isDead) {

		UpdateShooting();
		UpdateGrenading();

		if (CanJump()) {
			if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_SPACE)) {
				Jump();
			}
		}

		if (GetCurrentGun()) {
			auto gun = GetCurrentGun();
			int currentAmmoInMagazine = gun->GetCurrentAmmoInMagazine();
			int currentNotInMagazine = gun->GetCurrentAmmoNotInMagazine();
			if (currentAmmoInMagazine == INT_MAX) {
				Dbg::Text("AMMO: inf");
			}
			else if (currentNotInMagazine == INT_MAX) {
				Dbg::Text("AMMO: %d / inf", currentAmmoInMagazine);
			}
			else {
				Dbg::Text("AMMO: %d / %d", currentAmmoInMagazine, currentNotInMagazine);
			}

			if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_R)) {
				gun->Reload();
			}
		}
		if (grenadesCount > 0) {
			Dbg::Text("Grenades: %d", grenadesCount);
		}
		Dbg::Text("MOUSE LEFT - Shoot");
		if (grenadesCount > 0) {
			Dbg::Text("MOUSE RIGHT - Throw grenade");
		}
	}
	else {
		Dbg::Text("SPACE - Restart");
	}

	if (CfgGetBool("godMode")) {
		if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_LSHIFT)) {
			speed = defaultSpeed * 3.f;
		}
		else {
			speed = defaultSpeed;
		}
		if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_LSHIFT) && Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_K)) {
			for (auto go : Scene::Get()->gameObjects) {
				auto health = go->GetComponent<Health>();
				if (health && go != gameObject()) {
					health->DoDamage(health->GetAmount());
				}
			}
		}
	}

	UpdateZombiesAttacking();

	UpdateHealth();

	if (isDead && Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_SPACE)) {
		SceneManager::LoadScene(Scene::Get()->name);
	}

	Vector3 currentPos = gameObject()->transform()->GetPosition();
	if (Vector3::Distance(currentPos, prevFootstepPos) > 2.f) {
		prevFootstepPos = currentPos;
		if (footstepSounds.size() > 0) {
			auto audio = footstepSounds[Random::Range(0, footstepSounds.size())];
			AudioSystem::Get()->Play(audio);
		}
	}


	auto ray = Camera::GetMain()->ScreenPointToRay(Input::GetMousePosition());
	btCollisionWorld::ClosestRayResultCallback cb(btConvert(ray.origin), btConvert(ray.origin + ray.dir * 100.f));
	PhysicsSystem::Get()->dynamicsWorld->rayTest(btConvert(ray.origin), btConvert(ray.origin + ray.dir * 100.f), cb);
	if (cb.hasHit()) {
		auto pos = btConvert(cb.m_hitPointWorld);
		//Dbg::Text("MousePos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
	}
}

void PlayerController::FixedUpdate() {
	if (!isDead) {
		UpdateMovement();
		UpdateLook();
	}

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

void PlayerController::UpdateGrenading() {
	if (Input::GetKey(SDL_SCANCODE_G) || Input::GetMouseButtonDown(1)) {//TODO
		if (grenadesCount > 0 && grenadePrefab) {
			auto ray = Camera::GetMain()->ScreenPointToRay(Input::GetMousePosition());
			btCollisionWorld::ClosestRayResultCallback cb(btConvert(ray.origin), btConvert(ray.origin + ray.dir * 100.f));
			PhysicsSystem::Get()->dynamicsWorld->rayTest(btConvert(ray.origin), btConvert(ray.origin + ray.dir * 100.f), cb);
			if (cb.hasHit()) {
				grenadesCount--;
				auto grenadeGO = Object::Instantiate(grenadePrefab);
				auto pos = gameObject()->transform()->GetPosition() + Vector3(0, bulletSpawnOffset, 0);
				grenadeGO->transform()->SetPosition(pos);
				auto grenade = grenadeGO->GetComponent<Grenade>();
				if (grenade) {
					grenade->ThrowAt(btConvert(cb.m_hitPointWorld));
				}
				Scene::Get()->AddGameObject(grenadeGO);
			}

		}
	}
}
void PlayerController::UpdateShooting() {
	auto pos = gameObject()->transform()->GetPosition() + Vector3(0, bulletSpawnOffset, 0);
	auto rot = GetRot(gameObject()->transform()->matrix);
	auto gunTransform = Matrix4::Transform(pos, rot.ToMatrix(), Vector3_one);

	if (!Input::GetKey(SDL_SCANCODE_Z) && !Input::GetMouseButton(0)) {//TODO
		if (GetCurrentGun()) {
			GetCurrentGun()->ReleaseTrigger();
		}
	}
	else {
		if (GetCurrentGun()) {
			GetCurrentGun()->PullTrigger();
		}
	}


	bool bulletShot = false;
	if (GetCurrentGun()) {
		bulletShot = GetCurrentGun()->Update(gunTransform);
	}
	//TODO move to gun
	if (bulletShot) {
		if (shootingSounds.size() > 0) {
			auto audio = shootingSounds[Random::Range(0, shootingSounds.size())];
			AudioSystem::Get()->Play(audio);
		}
		SetRandomShootingLight(); //TODO move to gun
	}
	else {
		DisableShootingLight();
	}

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


	auto deltaPos = ray.GetPoint(dist) - playerPos;

	if (deltaPos.Length() < 0.1f) {
		return;
	}

	SetRot(matrix, Quaternion::LookAt(deltaPos, Vector3_up));
	//gameObject()->transform()->matrix = matrix;
	rigidBody->GetHandle()->setCenterOfMassTransform(btConvert(matrix));

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

}

bool PlayerController::CanJump() {
	if (!CfgGetBool("godMode")) {
		return false;
	}
	if (Time::time() - lastJumpTime < jumpDelay) {
		return false;
	}
	if (!IsOnGround()) {
		return false;
	}
	return true;
}

void PlayerController::OnDeath() {
	rigidBody->GetHandle()->setFriction(0.5f);
	rigidBody->GetHandle()->setAngularFactor(btVector3(1, 1, 1));
	rigidBody->GetHandle()->setDamping(0.6f, 0.6f);
	DisableShootingLight();
}

void PlayerController::SetRandomShootingLight() {
	if (!shootingLight) {
		return;
	}
	auto t = gameObject()->transform();
	auto lightPos = t->matrix * shootingLightOffset;
	shootingLight->GetComponent<Transform>()->SetPosition(lightPos);
	auto color = shootingLightPrefab->GetComponent<PointLight>()->color;
	if (GetCurrentGun()) {
		color = Color::Lerp(color, GetCurrentGun()->GetBulletColor(), 0.5f);
	}
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

std::shared_ptr<Gun> PlayerController::GetCurrentGun() {
	for (int i = guns.size() - 1; i >= 0; i--) {
		if (guns[i] && guns[i]->HasAmmo()) {
			return guns[i];
		}
	}
	return nullptr;
}

void PlayerController::AddGun(std::shared_ptr<Gun> gunTemplate) {
	if (!gunTemplate) {
		return;
	}
	if (guns.size() == 0 && gunTemplate != startGun) {
		AddGun(startGun);
	}
	auto it = gunTemplateToAvailableGun.find(gunTemplate);
	if (it != gunTemplateToAvailableGun.end()) {
		it->second->AddAmmo(gunTemplate->GetInitialAmmo());
	}
	else {
		auto gun = Object::Instantiate(gunTemplate);
		gunTemplateToAvailableGun[gunTemplate] = gun;
		guns.push_back(gun);
	}
}

void PlayerController::SetWon() {
	if (!won) {
		won = true;
		winTime = Time::time();
	}
}

DECLARE_TEXT_ASSET(PlayerController);

void PlayerController::UpdateZombiesAttacking() {
	auto nearby = PhysicsSystem::Get()->GetOverlaping(gameObject()->transform()->GetPosition(), 1.5f);
	for (auto go : nearby) {
		auto creep = go->GetComponent<EnemyCreepController>();
		if (!creep) {
			continue;
		}

		creep->Attack();
	}
}