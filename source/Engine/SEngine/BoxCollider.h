#pragma once

#include "Collider.h"
#include "SMath.h"

class btCollisionShape;
class btBoxShape;
class btSphereShape;
class btCapsuleShape;
class btTriangleIndexVertexArray;
class Mesh;

class BoxCollider : public Collider {
public:
	AABB GetAABBWithoutTransform() const;
protected:
	virtual std::shared_ptr<btCollisionShape> CreateShape() override;

	Vector3 size = Vector3_one;
	Vector3 center = Vector3_zero;

private:
	std::shared_ptr<btBoxShape> boxShape;
	REFLECT_BEGIN(BoxCollider, Collider);
	REFLECT_VAR(size);
	REFLECT_VAR(center);
	REFLECT_END();
};

class SphereCollider : public Collider {
public:
	virtual std::shared_ptr<btCollisionShape> CreateShape() override;

	float radius = 1.f;
	Vector3 center = Vector3_zero;

private:
	std::shared_ptr<btSphereShape> sphereShape;
	REFLECT_BEGIN(SphereCollider, Collider);
	REFLECT_VAR(radius);
	REFLECT_VAR(center);
	REFLECT_END();
};

class CapsuleCollider : public Collider {
public:
	virtual std::shared_ptr<btCollisionShape> CreateShape() override;

	float radius = 1.f;
	float height = 1.f;
	Vector3 center = Vector3_zero;

private:
	std::shared_ptr<btCapsuleShape> capsuleShape;
	REFLECT_BEGIN(CapsuleCollider, Collider);
	REFLECT_VAR(radius);
	REFLECT_VAR(height);
	REFLECT_VAR(center);
	REFLECT_END();
};