#pragma once

#include "System.h"
#include "SMath.h"
#include "MeshRenderer.h"//TODO need for serializing shared_ptr

class MeshRenderer;
class BulletSettings;

class BulletSystem : public System<BulletSystem> {
public:
	BulletSystem() {}

	virtual bool Init() override;
	virtual void Update() override;
	virtual void Draw() override;
	virtual void Term() override;

	void CreateBullet(const Vector3& pos, const Vector3& dir, const Color& color);

private:
	void CreateBullet(int idx, const Vector3& pos, const Vector3& dir, const Color& color);


	class Bullet {
	public:
		Bullet() {}
		Vector3 pos;
		Vector3 dir;
		Color color;
		float speed = 1.f;
		float timeLeft = 5.f;
	};

	std::shared_ptr<BulletSettings> bulletSettings;
	std::shared_ptr<BulletSettings> bulletDamageParticleSettings;
	std::shared_ptr<BulletSettings> bloodParticle;

	class BulletsVector {
	public:
		std::shared_ptr<BulletSettings> settings;
		std::vector<Bullet> bullets;
	};

	void UpdateBullets(BulletsVector& bulletsVector);
	void DrawBullets(BulletsVector& bulletsVector);

	std::vector<BulletsVector> bullets;

	void CreateDamageParticle(const Vector3& pos, const Vector3& dir);
	void CreateBloodParticle(const Vector3& pos, const Vector3& dir);
};

class BulletSettings : public Object{
public:
	std::shared_ptr<MeshRenderer> renderer;
	float impulse = 1.f;
	float radius = 0.04f;
	int damage = 1;
	float lifeTime = 5.f;
	bool hasColision = true;
	bool lifeTimeToScale = false;
	bool applyGravity = false;

	REFLECT_BEGIN(BulletSettings);
	REFLECT_VAR(renderer);
	REFLECT_VAR(radius);
	REFLECT_VAR(damage);
	REFLECT_VAR(impulse);
	REFLECT_VAR(hasColision);
	REFLECT_VAR(lifeTime);
	REFLECT_VAR(lifeTimeToScale);
	REFLECT_VAR(applyGravity);
	REFLECT_END()
};

