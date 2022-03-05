

#include "MeshCollider.h"
#include "Mesh.h"
#include "btBulletDynamicsCommon.h"
#include "PhysicsSystem.h"
#include "GameObject.h"
#include "Transform.h"
#include "assimp/mesh.h"
#include "System.h"
#include "Resources.h"


std::shared_ptr<btBvhTriangleMeshShape> MeshColliderStorageSystem::GetStored(std::shared_ptr<Mesh> mesh) {
	totalGets++;
	auto it = meshes.find(mesh);
	if (it == meshes.end()) {
		return AddNew(mesh);
	}
	else {
		return it->second.shape;
	}
}

bool MeshColliderStorageSystem::Init() {
	unloadHandle = AssetDatabase::Get()->onBeforeUnloaded.Subscribe([this]() {OnUnloaded(); });
	return true;
}

void MeshColliderStorageSystem::Term() {
	AssetDatabase::Get()->onBeforeUnloaded.Unsubscribe(unloadHandle);
	meshes.clear();
}

void MeshColliderStorageSystem::OnUnloaded() {
	meshes.clear();
}
std::shared_ptr<btBvhTriangleMeshShape> MeshColliderStorageSystem::AddNew(std::shared_ptr<Mesh> mesh) {
	if (mesh && mesh->physicsData && mesh->physicsData->triangleShape) {
		auto shape = StoredMesh{ mesh->physicsData->triangleShape, mesh->physicsData->triangles };
		meshes[mesh] = shape;
		return shape.shape;
	}
	else {
		auto shape = Create(mesh, true);
		meshes[mesh] = shape;
		return shape.shape;
	}

}

MeshColliderStorageSystem::StoredMesh MeshColliderStorageSystem::Create(std::shared_ptr<Mesh> mesh, bool buildBvh) {
	if (!mesh) {
		return StoredMesh{};
	}
	totalNew++;
	auto triangles = std::make_shared<btTriangleIndexVertexArray>();
	auto indexedMesh = btIndexedMesh();
	//TODO preload or use non assimp buffers;
	indexedMesh.m_numVertices = mesh->rawVertices.size();
	indexedMesh.m_vertexBase = (const unsigned char*)&(mesh->rawVertices[0].pos.x);
	indexedMesh.m_vertexStride = sizeof(RawVertexData);
	indexedMesh.m_vertexType = PHY_FLOAT;

	auto& indices = mesh->rawIndices;
	indexedMesh.m_numTriangles = mesh->rawIndices.size() / 3;
	indexedMesh.m_triangleIndexBase = (const uint8_t*)&indices[0];
	indexedMesh.m_triangleIndexStride = 3 * sizeof(uint16_t);
	indexedMesh.m_indexType = PHY_ScalarType::PHY_SHORT;
	triangles->addIndexedMesh(indexedMesh, PHY_ScalarType::PHY_SHORT);
	triangles->setPremadeAabb(btConvert(mesh->aabb.min), btConvert(mesh->aabb.max));

	auto shape = std::make_shared<btBvhTriangleMeshShape>(triangles.get(), true, buildBvh);

	return StoredMesh{ shape, triangles };
}
REGISTER_SYSTEM(MeshColliderStorageSystem);

std::shared_ptr<btCollisionShape> MeshCollider::CreateShape() {
	if (!mesh) {
		return nullptr;
	}
	auto nonScaledShape = MeshColliderStorageSystem::Get()->GetStored(mesh);
	auto scale = gameObject()->transform()->GetScale();
	std::shared_ptr<btCollisionShape> scaledShape;
	if (isConvex) {
		//TODO create convex shape on exporting and load on GetStored
		std::vector<btVector3> points;
		points.reserve(mesh->rawVertices.size());
		for (int i = 0; i < mesh->rawVertices.size(); i++) {
			points.push_back(btConvert(Vector3::HadamardProduct(mesh->rawVertices[i].pos, scale)));
		}
		auto shape = std::make_shared<btConvexHullShape>((btScalar*)points.data(), points.size());
		shape->initializePolyhedralFeatures(); // TODO this is for debug draw only now
		scaledShape = shape;
	}
	else {
		scaledShape = std::make_shared<btScaledBvhTriangleMeshShape>(nonScaledShape.get(), btConvert(scale));
	}
	return scaledShape;
}

DECLARE_TEXT_ASSET(MeshCollider);
