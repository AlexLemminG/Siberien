#pragma once

#include "Collider.h"

class btCollisionShape;
class btBoxShape;
class btSphereShape;

class BoxCollider : public Collider {
protected:
	virtual btCollisionShape* CreateShape() override;

	Vector3 size = Vector3_one;
	Vector3 center = Vector3_zero;

private:
	std::shared_ptr<btBoxShape> boxShape;
	REFLECT_BEGIN(BoxCollider);
	REFLECT_VAR(size);
	REFLECT_VAR(center);
	REFLECT_END();
};

class SphereCollider : public Collider {
protected:
	virtual btCollisionShape* CreateShape() override;

	float radius = 1.f;
	Vector3 center = Vector3_zero;

private:
	std::shared_ptr<btSphereShape> sphereShape;
	REFLECT_BEGIN(SphereCollider);
	REFLECT_VAR(radius);
	REFLECT_VAR(center);
	REFLECT_END();
};