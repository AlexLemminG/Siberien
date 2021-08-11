#pragma once

#include "System.h"
#include "Math.h"
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

	void CreateBullet(const Vector3& pos, const Vector3& dir);

private:
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

	std::vector<Bullet> bullets;
};

class BulletSettings : public Object{
public:
	std::shared_ptr<MeshRenderer> renderer;
	float impulse = 1.f;
	float radius = 0.04f;
	int damage = 1;

	REFLECT_BEGIN(BulletSettings);
	REFLECT_VAR(renderer);
	REFLECT_VAR(radius);
	REFLECT_VAR(damage);
	REFLECT_VAR(impulse);
	REFLECT_END()
};

