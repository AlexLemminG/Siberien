#include "BoxCollider.h"
#include "btBulletCollisionCommon.h"
#include "PhysicsSystem.h"
#include "Math.h"
#include "Transform.h"
#include "GameObject.h"

DECLARE_TEXT_ASSET(BoxCollider);
DECLARE_TEXT_ASSET(SphereCollider);
DECLARE_TEXT_ASSET(MeshCollider);

btCollisionShape* BoxCollider::CreateShape() {
	auto* compound = new btCompoundShape();

	auto trans = gameObject()->transform();
	Vector3 scale = GetScale(trans->matrix);
	Vector3 realSize = size;
	realSize = realSize * scale;

	Vector3 realCenter = scale * center;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btConvert(realCenter));

	boxShape.reset(new btBoxShape(btConvert(realSize / 2.f)));
	compound->addChildShape(transform, boxShape.get());
	return compound;
}


btCollisionShape* SphereCollider::CreateShape() {
	auto* compound = new btCompoundShape();

	auto trans = gameObject()->transform();
	Vector3 scale = GetScale(trans->matrix);
	float realRadius = radius;
	realRadius = realRadius * Mathf::Max(scale.x, Mathf::Max(scale.y, scale.z));

	Vector3 realCenter = scale * center;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btConvert(realCenter));

	//TODO move random somewhere else
	sphereShape.reset(new btSphereShape(realRadius));
	compound->addChildShape(transform, sphereShape.get());
	return compound;
}

btCollisionShape* MeshCollider::CreateShape() {
	if (!mesh) {
		return nullptr;
	}

	indexVertexArray = std::make_shared<btTriangleIndexVertexArray>();
	auto indexedMesh = btIndexedMesh();
	indexedMesh.m_numVertices = mesh->originalMeshPtr->mNumVertices;
	indexedMesh.m_vertexBase = (const unsigned char*)&(mesh->originalMeshPtr->mVertices[0].x);
	indexedMesh.m_vertexStride = sizeof(float) * 3;
	indexedMesh.m_vertexType = PHY_FLOAT;

	indexedMesh.m_numTriangles = mesh->originalMeshPtr->mNumFaces;
	indexedMesh.m_triangleIndexBase = (const unsigned char*)&mesh->indices[0];
	indexedMesh.m_triangleIndexStride = 3 * sizeof(uint16_t);
	indexedMesh.m_indexType = PHY_ScalarType::PHY_SHORT;

	indexVertexArray->addIndexedMesh(indexedMesh, PHY_ScalarType::PHY_SHORT);
	auto shape = new btBvhTriangleMeshShape(indexVertexArray.get(), true);
	return shape;
}