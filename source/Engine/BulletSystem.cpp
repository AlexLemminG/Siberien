#include "BulletSystem.h"
#include "bgfx/bgfx.h"
#include "Resources.h"
#include "MeshRenderer.h"
#include "Time.h"

REGISTER_SYSTEM(BulletSystem);

bool BulletSystem::Init() {
	bulletRenderer = AssetDatabase::Get()->LoadByPath<MeshRenderer>("bullet.asset");
	return true;
}

void BulletSystem::Update() {
	float dt = Time::deltaTime();
	for (auto& bullet : bullets) {
		bullet.pos += bullet.dir * dt;
		bullet.timeLeft -= dt;
	}
	for (int i = bullets.size() - 1; i >= 0; i--) {
		if (bullets[i].timeLeft < 0) {
			bullets[i] = bullets.back();
			bullets.pop_back();
		}
	}
}

void BulletSystem::Draw() {
	if (!bulletRenderer) {
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


	bgfx::setVertexBuffer(0, bulletRenderer->mesh->vertexBuffer);
	bgfx::setIndexBuffer(bulletRenderer->mesh->indexBuffer);

	// Set instance data buffer.
	bgfx::setInstanceDataBuffer(&idb);

	// Set render states.
	bgfx::setState(BGFX_STATE_DEFAULT);

	// Submit primitive for rendering to view 0.
	bgfx::submit(0, bulletRenderer->material->shader->program);
}

void BulletSystem::Term() {}

void BulletSystem::CreateBullet(const Vector3& pos, const Vector3& velocity) {
	Bullet bullet;
	bullet.pos = pos;
	bullet.dir = velocity;
	bullet.color = Colors::red;

	bullets.push_back(bullet);
}

