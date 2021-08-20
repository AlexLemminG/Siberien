#include "MeshCollider.h"
#include "Mesh.h"
#include "btBulletDynamicsCommon.h"
#include "PhysicsSystem.h"
#include "GameObject.h"
#include "Transform.h"
#include "assimp/mesh.h"
#include "System.h"
#include "Resources.h"


class MeshColliderStorageSystem : public System<MeshColliderStorageSystem> {
public:
	int totalGets = 0;
	int totalNew = 0;
	std::shared_ptr<btBvhTriangleMeshShape> GetStored(std::shared_ptr<Mesh> mesh, Vector3 scale) {
		totalGets++;
		auto it = meshes.find(mesh);
		if (it == meshes.end()) {
			return AddNew(mesh, scale);
		}
		else {
			for (const auto& sm : it->second) {
				if (Vector3::DistanceSquared(sm.scale, scale) < 0.001) {
					return sm.shape;
				}
			}
			return AddNew(mesh, scale);
		}
	}

	bool Init() override {
		unloadHandle = AssetDatabase::Get()->onUnloaded.Subscribe([this]() {OnUnloaded(); });
		return true;
	}

	void Term() override {
		AssetDatabase::Get()->onUnloaded.Unsubscribe(unloadHandle);
		meshes.clear();
		meshIndices.clear();
	}

	void OnUnloaded() {
		meshes.clear();
		meshIndices.clear();
	}
	int unloadHandle;
private:
	std::shared_ptr<btBvhTriangleMeshShape> AddNew(std::shared_ptr<Mesh> mesh, Vector3 scale) {
		if (!mesh) {
			return nullptr;
		}
		totalNew++;
		auto triangles = std::make_shared<btTriangleIndexVertexArray>();
		auto indexedMesh = btIndexedMesh();
		//TODO preload or use non assimp buffers;
		indexedMesh.m_numVertices = mesh->originalMeshPtr->mNumVertices;
		indexedMesh.m_vertexBase = (const unsigned char*)&(mesh->originalMeshPtr->mVertices[0].x);
		indexedMesh.m_vertexStride = sizeof(float) * 3;
		indexedMesh.m_vertexType = PHY_FLOAT;

		auto itIndices = meshIndices.find(mesh);
		if (itIndices == meshIndices.end()) {
			meshIndices[mesh] = std::move(MeshVertexLayout::CreateIndices(mesh->originalMeshPtr));
		}
		auto& indices = meshIndices[mesh];
		indexedMesh.m_numTriangles = mesh->originalMeshPtr->mNumFaces;
		indexedMesh.m_triangleIndexBase = (const uint8_t*)&indices[0];
		indexedMesh.m_triangleIndexStride = 3 * sizeof(uint16_t);
		indexedMesh.m_indexType = PHY_ScalarType::PHY_SHORT;
		triangles->addIndexedMesh(indexedMesh, PHY_ScalarType::PHY_SHORT);
		triangles->setScaling(btConvert(scale));
		auto shape = std::make_shared<btBvhTriangleMeshShape>(triangles.get(), true);

		meshes[mesh].push_back(StoredMesh{ scale, shape, triangles });

		return shape;
	}
	struct StoredMesh {
		Vector3 scale;
		std::shared_ptr<btBvhTriangleMeshShape> shape;
		std::shared_ptr<btTriangleIndexVertexArray> triangles;
	};

	std::unordered_map<std::shared_ptr<Mesh>, std::vector<StoredMesh>> meshes;
	std::unordered_map<std::shared_ptr<Mesh>, std::vector<uint16_t>> meshIndices;
};
REGISTER_SYSTEM(MeshColliderStorageSystem);

std::shared_ptr<btCollisionShape> MeshCollider::CreateShape() {
	if (!mesh) {
		return nullptr;
	}
	auto scale = gameObject()->transform()->GetScale();
	auto shape = MeshColliderStorageSystem::Get()->GetStored(mesh, scale);
	return shape;
}

DECLARE_TEXT_ASSET(MeshCollider);
