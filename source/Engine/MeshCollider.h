#pragma once

#include "Collider.h"

class btTriangleIndexVertexArray;
class Mesh;

class MeshCollider : public Collider {
public:
	virtual std::shared_ptr<btCollisionShape> CreateShape() override;

	std::shared_ptr<btTriangleIndexVertexArray> indexVertexArray;
	std::shared_ptr<Mesh> mesh;
private:
	REFLECT_BEGIN(MeshCollider);
	REFLECT_VAR(mesh);
	REFLECT_END();
};