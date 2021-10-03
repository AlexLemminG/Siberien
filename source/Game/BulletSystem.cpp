#include "BulletSystem.h"
#include "bgfx/c99/bgfx.h"
#include "Resources.h"
#include "MeshRenderer.h"
#include "STime.h"
#include "Dbg.h"
#include "GameObject.h"
#include "Physics.h"
#include "Health.h"
#include "RigidBody.h"
#include "Material.h"
#include "Shader.h"
#include "Mesh.h"
#include "Light.h"
#include "Scene.h"
#include "GameObject.h"

REGISTER_GAME_SYSTEM(BulletSystem);
DECLARE_TEXT_ASSET(BulletSettings);

bool BulletSystem::Init() {
	bulletSettings = AssetDatabase::Get()->Load<BulletSettings>("bullet.asset");
	bulletDamageParticleSettings = AssetDatabase::Get()->Load<BulletSettings>("bulletDamageParticle.asset");
	bloodParticle = AssetDatabase::Get()->Load<BulletSettings>("bloodParticle.asset");

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
		int collisionMask = Physics::GetLayerCollisionMask("playerBullet");
		for (auto& bullet : bullets) {
			Vector3 bulletPrevPos = bullet.pos;
			bullet.pos += bullet.dir * (dt * bullet.speed);
			bullet.timeLeft -= dt;
			//TODO bullet.dir is not normalized, but maybe for better
			Ray ray{ bulletPrevPos - bullet.dir * bulletsVector.settings->radius , bullet.dir * bulletsVector.settings->radius * 2.f };
			Physics::RaycastHit hit;
			//TODO layer
			if (Physics::SphereCast(hit, ray, bulletsVector.settings->radius, bullet.dir.Length() * bulletsVector.settings->radius * 2.f, collisionMask)) {
				bullet.timeLeft = 0.f;
				bool hitCharacter = false;
				auto rb = hit.GetRigidBody();
				if (rb) {
					bool needImpulse = true;

					auto gameObject = rb->gameObject();
					if (gameObject) {
						auto health = gameObject->GetComponent<Health>();
						if (health) {
							if (health->IsDead()) {
								needImpulse = false;
							}
							health->DoDamage(bulletsVector.settings->damage);
							hitCharacter = true;
						}
					}

					if (needImpulse) {
						rb->ApplyLinearImpulse(bullet.dir * bulletsVector.settings->impulse, bullet.pos);
					}
				}
				{
					Vector3 pos = hit.GetPoint();
					if (hitCharacter) {
						CreateBloodParticle(pos, bullet.dir * bullet.speed);
					}
					else {
						Vector3 normalVel = hit.GetNormal();
						Vector3 velFromBullet = bullet.dir - normalVel * Vector3::DotProduct(bullet.dir, normalVel);
						Vector3 particleVel = Mathf::Lerp(normalVel, velFromBullet, 0.5f) * bullet.speed * 0.7f;
						CreateDamageParticle(pos, particleVel);
					}
				}
			}
		}

		//TODO lights without game objects
		for (int i = 0; i < bullets.size(); i++) {
			if (i >= addedLightsCount) {
				if (lights.size() > i) {
					Scene::Get()->AddGameObject(lights[i]);
				}
				else {
					auto go = std::make_shared<GameObject>();
					go->components.push_back(std::make_shared<Transform>());
					go->components.push_back(std::make_shared<PointLight>());

					const auto& bullet = bullets[i];
					go->transform()->SetPosition(bullets[i].pos);
					auto light = go->GetComponent<PointLight>();
					lights.push_back(go);
					light->innerRadius = 0.0f;
					light->radius = 5.0f;
					Scene::Get()->AddGameObject(lights[i]);
				}
				addedLightsCount++;
			}
			auto go = lights[i];
			auto light = go->GetComponent<PointLight>();
			const auto& bullet = bullets[i];
			go->transform()->SetPosition(bullets[i].pos);
			light->color = Color::Lerp(bullet.color, Colors::black, 0.2f);
		}
		for (int i = bullets.size(); i < addedLightsCount; i++) {
			Scene::Get()->RemoveGameObject(lights[i]);
		}
		addedLightsCount = bullets.size();
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
	size_t totalCubes = bullets.size();

	if (totalCubes == 0) {
		return;
	}

	// figure out how big of a buffer is available
	uint32_t drawnCubes = bgfx_get_avail_instance_data_buffer(totalCubes, instanceStride);
	

	// save how many we couldn't draw due to buffer room so we can display it
	int m_lastFrameMissing = totalCubes - drawnCubes;

	bgfx::InstanceDataBuffer idb;
	bgfx_alloc_instance_data_buffer((bgfx_instance_data_buffer_t*)&idb, drawnCubes, instanceStride);
	

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


	bgfx_set_vertex_buffer(0, *(bgfx_vertex_buffer_handle_t*)(&bulletsVector.settings->renderer->mesh->vertexBuffer), 0, UINT32_MAX);
	bgfx_set_index_buffer(*(bgfx_index_buffer_handle_t*)(&bulletsVector.settings->renderer->mesh->indexBuffer), 0, UINT32_MAX);
	

	// Set instance data buffer.
	bgfx_set_instance_data_buffer((bgfx_instance_data_buffer_t*)&idb, 0, UINT32_MAX);
	// Set render states.
	bgfx_set_state(BGFX_STATE_DEFAULT, 0);

	// Submit primitive for rendering to view kRenderPassGeometry.
	const int kRenderPassGeometry = 11;//TODO use var from render

	bgfx_submit(kRenderPassGeometry, *(bgfx_program_handle_t*)(&bulletsVector.settings->renderer->material->shader->program), 0, BGFX_DISCARD_ALL);
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

