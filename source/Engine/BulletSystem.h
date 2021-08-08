#pragma once

#include "System.h"
#include "Math.h"

class MeshRenderer;

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
	float bulletImpulse = 1.f;
	float bulletRadius = 0.04f;
	std::vector<Bullet> bullets;
	std::shared_ptr<MeshRenderer> bulletRenderer;
};
