#pragma once

#include "System.h"
#include "SMath.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btTransform.h"
#include "AlignedAllocator.h"

inline Vector3 btConvert(btVector3 vec) {
	return Vector3(vec.x(), vec.y(), vec.z());
}

inline btVector3 btConvert(Vector3 vec) {
	return btVector3(vec.x, vec.y, vec.z);
}

inline btQuaternion btConvert(Quaternion vec) {
	return btQuaternion(vec[1], vec[2], vec[3], vec[0]);
}

inline Quaternion btConvert(btQuaternion vec) {
	return Quaternion(vec.getW(), vec.getX(), vec.getY(), vec.getZ());
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
class btConstraintSolverPoolMt;
class btITaskScheduler;
class btGhostPairCallback;

class MeshPhysicsData {
public:
	std::shared_ptr<btBvhTriangleMeshShape> triangleShape;
	std::shared_ptr<btTriangleIndexVertexArray> triangles;
	std::vector<uint8_t, aligned_allocator<uint8_t, 16>> triangleShapeBuffer;
};

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btDynamicsWorld;
class GameObject;
class BinaryBuffer;

class PhysicsSettings : public Object {
public:
	void GetGroupAndMask(const std::string& groupName, int& group, int& mask);
	Vector3 gravity{ 0.f, -9.8f, 0.f };
private:
	class Layer {
	public:
		std::string name;
		std::vector<std::string> collideWith;
		std::vector<std::string> doNotCollideWith;

		int group = 0;
		int mask = 0;

		REFLECT_BEGIN(Layer);
		REFLECT_VAR(name);
		REFLECT_VAR(collideWith);
		REFLECT_VAR(doNotCollideWith);
		REFLECT_END();
	};
	std::vector<Layer> layers;
	REFLECT_BEGIN(PhysicsSettings);
	REFLECT_VAR(layers);
	REFLECT_VAR(gravity);
	REFLECT_END();

	std::unordered_map<std::string, Layer> layersMap;

	virtual void OnAfterDeserializeCallback(const SerializationContext& context);;
};

class PhysicsSystem : public System<PhysicsSystem> {
public:
	virtual bool Init() override;
	virtual void Update() override;
	virtual void Term() override;

	Vector3 GetGravity()const;

	btDefaultCollisionConfiguration* collisionConfiguration = nullptr;

	btCollisionDispatcher* dispatcher = nullptr;

	btBroadphaseInterface* overlappingPairCache = nullptr;

	btSequentialImpulseConstraintSolver* solver = nullptr;

	btConstraintSolverPoolMt* solverPool = nullptr;

	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
	btITaskScheduler* taskScheduler = nullptr;
	btGhostPairCallback* ghostCall = nullptr;

	void GetGroupAndMask(const std::string& groupName, int& group, int& mask);

	void SerializeMeshPhysicsDataToBuffer(std::vector<std::shared_ptr<Mesh>>& meshes, BinaryBuffer& buffer);
	void DeserializeMeshPhysicsDataFromBuffer(std::vector<std::shared_ptr<Mesh>>& meshes, BinaryBuffer& buffer);
	void CalcMeshPhysicsDataFromBuffer(std::shared_ptr<Mesh> mesh);

	static std::string reservedLayerName;
private:
	static void OnPhysicsTick(btDynamicsWorld* world, btScalar timeStep);

	float prevSimulationTime = 0.f;

	std::shared_ptr<PhysicsSettings> settings;
};