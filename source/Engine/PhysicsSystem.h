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

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

class PhysicsSystem : public System< PhysicsSystem> {
public:
	virtual bool Init() override;
	virtual void Update() override;
	virtual void Term() override;

	btDefaultCollisionConfiguration* collisionConfiguration = nullptr;

	btCollisionDispatcher* dispatcher = nullptr;

	btBroadphaseInterface* overlappingPairCache = nullptr;

	btSequentialImpulseConstraintSolver* solver = nullptr;

	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;

private:
	float prevSimulationTime = 0.f;
};