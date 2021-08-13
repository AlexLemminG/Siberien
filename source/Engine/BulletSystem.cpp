#include "BulletSystem.h"
#include "bgfx/bgfx.h"
#include "Resources.h"
#include "MeshRenderer.h"
#include "Time.h"
#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include "Dbg.h"
#include "GameObject.h"
#include "Health.h"

REGISTER_SYSTEM(BulletSystem);
DECLARE_TEXT_ASSET(BulletSettings);

bool BulletSystem::Init() {
	bulletSettings = AssetDatabase::Get()->LoadByPath<BulletSettings>("bullet.asset");
	bulletDamageParticleSettings = AssetDatabase::Get()->LoadByPath<BulletSettings>("bulletDamageParticle.asset");
	bloodParticle = AssetDatabase::Get()->LoadByPath<BulletSettings>("bloodParticle.asset");

	bullets.push_back(BulletsVector{ bulletSettings, std::vector<Bullet>() });
	bullets.push_back(BulletsVector{ bulletDamageParticleSettings, std::vector<Bullet>() });
	bullets.push_back(BulletsVector{ bloodParticle, std::vector<Bullet>() });
	return true;
}

void BulletSystem::Update() {
	for (auto& bulletsVector : bullets) {
		UpdateBullets(bulletsVector);
	}
}

void BulletSystem::UpdateBullets(BulletsVector& bulletsVector) {

	auto& bullets = bulletsVector.bullets;
	float dt = Time::deltaTime();
	auto* physics = PhysicsSystem::Get()->dynamicsWorld;


	if (bulletsVector.settings->applyGravity) {
		for (auto& bullet : bullets) {
			bullet.dir.y -= 10.f * dt / bullet.speed;
		}
	}

	if (!bulletsVector.settings->hasColision) {
		for (auto& bullet : bullets) {
			Vector3 bulletPrevPos = bullet.pos;
			bullet.pos += bullet.dir * (dt * bullet.speed);
			bullet.timeLeft -= dt;
		}
	}
	else {
		btSphereShape sphereShape{ bulletsVector.settings->radius };
		btTransform from;
		from.setIdentity();
		btTransform to;
		to.setIdentity();

		for (auto& bullet : bullets) {
			Vector3 bulletPrevPos = bullet.pos;
			bullet.pos += bullet.dir * (dt * bullet.speed);
			bullet.timeLeft -= dt;
			//TODO bullet.dir is not normalized, but maybe for better

			btVector3 castFrom = btConvert(bulletPrevPos - bullet.dir * bulletsVector.settings->radius);
			btVector3 castTo = btConvert(bullet.pos + bullet.dir * bulletsVector.settings->radius);
			from.setOrigin(castFrom);
			to.setOrigin(castTo);
			btCollisionWorld::ClosestConvexResultCallback cb(from.getOrigin(), to.getOrigin());
			cb.m_collisionFilterMask = PhysicsSystem::playerBulletMask;
			cb.m_collisionFilterGroup = PhysicsSystem::playerBulletGroup;
			physics->convexSweepTest(&sphereShape, from, to, cb);
			if (cb.hasHit()) {
				bullet.timeLeft = 0.f;
				bool hitCharacter = false;
				auto obj = dynamic_cast<const btRigidBody*>(cb.m_hitCollisionObject);
				if (obj) {
					auto nonConst = const_cast<btRigidBody*>(obj);
					auto localPos = nonConst->getCenterOfMassTransform().inverse() * btConvert(bullet.pos);
					nonConst->activate(true);
					nonConst->applyImpulse(btConvert(bullet.dir * bulletsVector.settings->impulse), localPos);

					auto gameObject = (GameObject*)nonConst->getUserPointer();
					if (gameObject) {
						auto health = gameObject->GetComponent<Health>();
						if (health) {
							health->DoDamage(bulletsVector.settings->damage);
							hitCharacter = true;
						}
					}
				}
				{
					Vector3 pos = btConvert(cb.m_hitPointWorld);
					if (hitCharacter) {
						CreateBloodParticle(pos, bullet.dir * bullet.speed);
					}
					else {
						Vector3 normalVel = btConvert(cb.m_hitNormalWorld);
						Vector3 velFromBullet = bullet.dir - normalVel * Vector3::DotProduct(bullet.dir, normalVel);
						Vector3 particleVel = Mathf::Lerp(normalVel, velFromBullet, 0.5f) * bullet.speed * 0.7f;
						CreateDamageParticle(pos, particleVel);
					}
				}
			}
		}
	}

	int bulletsCountBefore = bullets.size();
	for (int i = bullets.size() - 1; i >= 0; i--) {
		if (bullets[i].timeLeft <= 0) {
			bullets[i] = bullets.back();
			bullets.pop_back();
		}
	}
	int bulletsDestroyed = bulletsCountBefore - bullets.size();
	int k = 0;
	//Dbg::Text("bullets alive: %d, bullets destroyed: %d", bullets.size(), bulletsDestroyed);
}

void BulletSystem::Draw() {
	for (auto& bulletsVector : bullets) {
		DrawBullets(bulletsVector);
	}
}
void BulletSystem::DrawBullets(BulletsVector& bulletsVector) {
	if (!bulletsVector.settings->renderer) {
		return;
	}

	auto& bullets = bulletsVector.bullets;

	// 80 bytes stride = 64 bytes for 4x4 matrix + 16 bytes for RGBA color.
	const uint16_t instanceStride = 80;
	// to total number of instances to draw
	uint32_t totalCubes = bullets.size();

	if (totalCubes == 0) {
		return;
	}

	// figure out how big of a buffer is available
	uint32_t drawnCubes = bgfx::getAvailInstanceDataBuffer(totalCubes, instanceStride);

	// save how many we couldn't draw due to buffer room so we can display it
	int m_lastFrameMissing = totalCubes - drawnCubes;

	bgfx::InstanceDataBuffer idb;
	bgfx::allocInstanceDataBuffer(&idb, drawnCubes, instanceStride);

	uint8_t* data = idb.data;

	
	float scaleMult = bulletsVector.settings->radius * 2.f;
	float timeLeftToScaleMult = 1.f / bulletsVector.settings->lifeTime;
	for (uint32_t ii = 0; ii < drawnCubes; ++ii)
	{
		const auto& bullet = bullets[ii];
		//uint32_t yy = ii / m_sideSize;
		//uint32_t xx = ii % m_sideSize;

		Matrix4* mtx = (Matrix4*)data;
		float scale = (bulletsVector.settings->lifeTimeToScale ? bullet.timeLeft * timeLeftToScaleMult : 1.f) * scaleMult;
		//TODO looks heavy
		*mtx = Matrix4::Transform(bullet.pos, Quaternion::LookAt(bullet.dir, Vector3_up).ToMatrix(), Vector3(scale, scale, Mathf::Max(1.f, bullet.speed / 7.f) * scale));

		Color* color = (Color*)&data[sizeof(Matrix4)];
		*color = bullet.color;
		//SetPos(*mtx, bullet.pos);

		//bx::mtxRotateXY(mtx, time + xx * 0.21f, time + yy * 0.37f);
		//mtx[12] = -15.0f + float(xx) * 3.0f;
		//mtx[13] = -15.0f + float(yy) * 3.0f;
		//mtx[14] = 0.0f;

		//float* color = (float*)&data[64];
		//color[0] = bx::sin(time + float(xx) / 11.0f) * 0.5f + 0.5f;
		//color[1] = bx::cos(time + float(yy) / 11.0f) * 0.5f + 0.5f;
		//color[2] = bx::sin(time * 3.0f) * 0.5f + 0.5f;
		//color[3] = 1.0f;

		data += instanceStride;
	}


	bgfx::setVertexBuffer(0, bulletsVector.settings->renderer->mesh->vertexBuffer);
	bgfx::setIndexBuffer(bulletsVector.settings->renderer->mesh->indexBuffer);

	// Set instance data buffer.
	bgfx::setInstanceDataBuffer(&idb);

	// Set render states.
	bgfx::setState(BGFX_STATE_DEFAULT);

	// Submit primitive for rendering to view 0.
	bgfx::submit(0, bulletsVector.settings->renderer->material->shader->program);
}

void BulletSystem::Term() {}

void BulletSystem::CreateBullet(const Vector3& pos, const Vector3& velocity, const Color& color) {
	CreateBullet(0, pos, velocity, color);
}
void BulletSystem::CreateBullet(int idx, const Vector3& pos, const Vector3& velocity, const Color& color) {
	Bullet bullet;
	bullet.pos = pos;
	bullet.speed = velocity.Length();
	bullet.dir = velocity / bullet.speed;
	bullet.color = color;
	bullet.timeLeft = bullets[idx].settings->lifeTime;

	bullets[idx].bullets.push_back(bullet);
}

void BulletSystem::CreateDamageParticle(const Vector3& pos, const Vector3& velocity) {
	int count = (int)Random::Range(3.f, 10.f);
	for (int i = 0; i < count; i++) {
		Color color;
		color.a = 1.f;
		color.r = Random::Range(0.3f, 0.5f);
		color.g = Random::Range(0.3f, 0.4f);
		color.b = Random::Range(0.3f, 0.4f);
		Vector3 vel = velocity + Random::InsideUnitSphere() * velocity.Length();
		CreateBullet(1, pos, vel, color);
	}
}
void BulletSystem::CreateBloodParticle(const Vector3& pos, const Vector3& velocity) {
	int count = (int)Random::Range(3.f, 10.f);
	for (int i = 0; i < count; i++) {
		Color color;
		color.a = 1.f;
		color.r = Random::Range(0.4f, 0.5f);
		color.g = Random::Range(0.0f, 0.1f);
		color.b = Random::Range(0.0f, 0.1f);
		Vector3 vel = (velocity + Random::InsideUnitSphere() * (velocity.Length() * 0.5f)) * 0.4f;
		CreateBullet(2, pos, vel, color);
	}
}

