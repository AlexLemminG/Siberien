#pragma once

#include "Collider.h"
#include "MeshRenderer.h"

class btCollisionShape;
class btBoxShape;
class btSphereShape;
class btTriangleIndexVertexArray;

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
public:
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

class MeshCollider : public Collider {
public:
	virtual btCollisionShape* CreateShape() override;

	std::shared_ptr<btTriangleIndexVertexArray> indexVertexArray;
	std::shared_ptr<Mesh> mesh;
private:
	REFLECT_BEGIN(MeshCollider);
	REFLECT_VAR(mesh);
	REFLECT_END();
};