#pragma once

#include "Collider.h"
#include "System.h"
#include "GameEvents.h"

class btTriangleIndexVertexArray;
class Mesh;
class btBvhTriangleMeshShape;

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



class MeshColliderStorageSystem : public System<MeshColliderStorageSystem> {
public:
	std::shared_ptr<btBvhTriangleMeshShape> GetStored(std::shared_ptr<Mesh> mesh);

	bool Init() override;

	void Term() override;

	struct StoredMesh {
		std::shared_ptr<btBvhTriangleMeshShape> shape;
		std::shared_ptr<btTriangleIndexVertexArray> triangles;
	};
	StoredMesh Create(std::shared_ptr<Mesh> mesh, bool buildBvh);
private:
	int totalGets = 0;
	int totalNew = 0;
	GameEventHandle unloadHandle;
	void OnUnloaded();
	std::shared_ptr<btBvhTriangleMeshShape> AddNew(std::shared_ptr<Mesh> mesh);

	std::unordered_map<std::shared_ptr<Mesh>, StoredMesh> meshes;
};