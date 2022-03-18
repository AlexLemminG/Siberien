#pragma once
#include "SMath.h"

class RigidBody;
class btCollisionObject;
class PhysicsBody;
class GameObject;

class SE_CPP_API Physics {
public:
	class RaycastHit {
	public:
		RaycastHit() {
		}
		RaycastHit(Vector3 point, Vector3 normal, RigidBody* rigidBody) :point(point), normal(normal), rigidBody(rigidBody) {

		}
		const Vector3& GetPoint() { return point; }
		const Vector3& GetNormal() { return normal; }
		RigidBody* GetRigidBody() { return rigidBody; }
	private:
		Vector3 point;
		Vector3 normal;
		RigidBody* rigidBody; //TODO may not exist
	};

	static bool SphereCast(RaycastHit& hit, Ray ray, float radius, float maxDistance, int layerMask = -1);
	static bool Raycast(RaycastHit& hit, Ray ray, float maxDistance, int layerMask = -1);

	static std::vector<RigidBody*> OverlapSphere(const Vector3& pos, float radius, int layerMask = -1);

	static int GetLayerCollisionMask(const std::string& layer);
	static int GetLayerAsMask(const std::string& layer);

	static Vector3 GetGravity();
	static void SetGravity(const Vector3& gravity);

	static std::shared_ptr<GameObject> GetGameObject(const btCollisionObject* collisionObject);
private:
	static RigidBody* GetRigidBody(const btCollisionObject* collisionObject);
};