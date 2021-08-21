#pragma once

#include "System.h"
#include "Math.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btTransform.h"

inline Vector3 btConvert(btVector3 vec) {
	return Vector3(vec.x(), vec.y(), vec.z());
}
inline btVector3 btConvert(Vector3 vec) {
	return btVector3(vec.x, vec.y, vec.z);
}
inline Matrix3 btConvert(btMatrix3x3 rot) {
	Matrix3 r;
	r.data_[0] = btConvert(rot[0]);
	r.data_[1] = btConvert(rot[1]);
	r.data_[2] = btConvert(rot[2]);
	return r;
}
inline btTransform btConvert(Matrix4 transformMatrix) {
	btTransform transform;
	transform.setFromOpenGLMatrix(&transformMatrix(0, 0));
	return transform;
}
inline Matrix4 btConvert(btTransform transformMatrix) {
	Matrix4 mat;
	transformMatrix.getOpenGLMatrix(&mat(0, 0));
	return mat;
}

class btBvhTriangleMeshShape;
class btTriangleIndexVertexArray;
class Mesh;

class MeshPhysicsData {
public:
	std::shared_ptr<btBvhTriangleMeshShape> triangleShape;
	std::shared_ptr<btTriangleIndexVertexArray> triangles;
	std::vector<uint8_t> triangleShapeBuffer;
};

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btDynamicsWorld;
class GameObject;
class BinaryBuffer;

class PhysicsSystem : public System< PhysicsSystem> {
public:
	virtual bool Init() override;
	virtual void Update() override;
	virtual void Term() override;

	Vector3 GetGravity()const;
	std::vector<GameObject*> GetOverlaping(Vector3 pos, float radius);

	btDefaultCollisionConfiguration* collisionConfiguration = nullptr;

	btCollisionDispatcher* dispatcher = nullptr;

	btBroadphaseInterface* overlappingPairCache = nullptr;

	btSequentialImpulseConstraintSolver* solver = nullptr;

	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;


	static constexpr int defaultGroup = 64;
	static constexpr int playerGroup = 128;
	static constexpr int enemyGroup = 256;
	static constexpr int playerBulletGroup = 512;
	static constexpr int enemyBulletGroup = 1024;
	static constexpr int enemyCorpseGroup = 2048;
	static constexpr int grenadeGroup = 4096;
	static constexpr int staticGeomGroup = 8192;


	static constexpr int defaultMask = -1 ^ (playerBulletGroup | enemyBulletGroup);
	static constexpr int playerMask = staticGeomGroup | defaultGroup | playerGroup | enemyGroup | enemyBulletGroup;
	static constexpr int enemyMask = staticGeomGroup | defaultGroup | enemyGroup | playerGroup | playerBulletGroup | enemyCorpseGroup;
	static constexpr int playerBulletMask = staticGeomGroup | enemyMask | enemyCorpseGroup;
	static constexpr int enemyBulletMask = staticGeomGroup | playerMask;
	static constexpr int enemyCorpseMask = defaultGroup | enemyGroup | playerBulletGroup | enemyCorpseGroup | staticGeomGroup;
	static constexpr int grenadeMask = -1 ^ (playerGroup | enemyGroup);
	static constexpr int staticGeomMask = enemyBulletMask | playerBulletMask | playerMask | enemyMask | enemyCorpseGroup;

	static void GetGroupAndMask(const std::string& groupName, int& group, int& mask);

	void SerializeMeshPhysicsDataToBuffer(std::vector<std::shared_ptr<Mesh>>& meshes, BinaryBuffer& buffer);
	void DeserializeMeshPhysicsDataFromBuffer(std::vector<std::shared_ptr<Mesh>>& meshes, BinaryBuffer& buffer);
	void CalcMeshPhysicsDataFromBuffer(std::shared_ptr<Mesh> mesh);

private:
	static void OnPhysicsTick(btDynamicsWorld* world, btScalar timeStep);

	float prevSimulationTime = 0.f;
};