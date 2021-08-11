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
	return true;
}

void BulletSystem::Update() {
	float dt = Time::deltaTime();
	auto* physics = PhysicsSystem::Get()->dynamicsWorld;

	btSphereShape sphereShape{ bulletSettings->radius };
	btTransform from;
	from.setIdentity();
	btTransform to;
	to.setIdentity();
	for (auto& bullet : bullets) {
		Vector3 bulletPrevPos = bullet.pos;
		bullet.pos += bullet.dir * (dt * bullet.speed);
		bullet.timeLeft -= dt;
		//TODO bullet.dir is not normalized, but maybe for better

		btVector3 castFrom = btConvert(bulletPrevPos - bullet.dir * bulletSettings->radius);
		btVector3 castTo = btConvert(bullet.pos + bullet.dir * bulletSettings->radius);
		from.setOrigin(castFrom);
		to.setOrigin(castTo);
		btCollisionWorld::ClosestConvexResultCallback cb(from.getOrigin(), to.getOrigin());
		cb.m_collisionFilterMask = PhysicsSystem::playerBulletMask;
		cb.m_collisionFilterGroup = PhysicsSystem::playerBulletGroup;
		physics->convexSweepTest(&sphereShape, from, to, cb);
		if (cb.hasHit()) {
			bullet.timeLeft = 0.f;
			auto obj = dynamic_cast<const btRigidBody*>(cb.m_hitCollisionObject);
			if (obj) {
				auto nonConst = const_cast<btRigidBody*>(obj);
				auto localPos = nonConst->getCenterOfMassTransform().inverse() * btConvert(bullet.pos);
				nonConst->activate(true);
   				nonConst->applyImpulse(btConvert(bullet.dir * bulletSettings->impulse), localPos);

				auto gameObject = (GameObject*)nonConst->getUserPointer();
				if (gameObject) {
					auto health = gameObject->GetComponent<Health>();
					if (health) {
						health->DoDamage(bulletSettings->damage);
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
	Dbg::Text("bullets alive: %d, bullets destroyed: %d", bullets.size(), bulletsDestroyed);
}

void BulletSystem::Draw() {
	if (!bulletSettings->renderer) {
		return;
	}

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

	for (uint32_t ii = 0; ii < drawnCubes; ++ii)
	{
		const auto& bullet = bullets[ii];
		//uint32_t yy = ii / m_sideSize;
		//uint32_t xx = ii % m_sideSize;

		Matrix4* mtx = (Matrix4*)data;
		//TODO looks heavy
		*mtx = Matrix4::Transform(bullet.pos, Quaternion::LookAt(bullet.dir, Vector3_up).ToMatrix(), Vector3_one);

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


	bgfx::setVertexBuffer(0, bulletSettings->renderer->mesh->vertexBuffer);
	bgfx::setIndexBuffer(bulletSettings->renderer->mesh->indexBuffer);

	// Set instance data buffer.
	bgfx::setInstanceDataBuffer(&idb);

	// Set render states.
	bgfx::setState(BGFX_STATE_DEFAULT);

	// Submit primitive for rendering to view 0.
	bgfx::submit(0, bulletSettings->renderer->material->shader->program);
}

void BulletSystem::Term() {}

void BulletSystem::CreateBullet(const Vector3& pos, const Vector3& velocity) {
	Bullet bullet;
	bullet.pos = pos;
	bullet.speed = velocity.Length();
	bullet.dir = velocity / bullet.speed;
	bullet.color = Colors::red;

	bullets.push_back(bullet);
}

