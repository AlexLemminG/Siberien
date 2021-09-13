#pragma once
#include "SMath.h"

class RigidBody;
class btCollisionObject;

class SE_CPP_API Physics {
public:
	class RaycastHit {
	public:
		RaycastHit() {
		}
		RaycastHit(Vector3 point, Vector3 normal, std::shared_ptr<RigidBody> rigidBody) :point(point), normal(normal), rigidBody(rigidBody) {

		}
		const Vector3& GetPoint() { return point; }
		const Vector3& GetNormal() { return normal; }
		const std::shared_ptr<RigidBody>& GetRigidBody() { return rigidBody; }
	private:
		Vector3 point;
		Vector3 normal;
		std::shared_ptr<RigidBody> rigidBody; //TODO may not exist
	};

	static bool SphereCast(RaycastHit& hit, Ray ray, float radius, float maxDistance, int layerMask = -1);
	static bool Raycast(RaycastHit& hit, Ray ray, float maxDistance);

	static std::vector<std::shared_ptr<RigidBody>> OveplapSphere(const Vector3& pos, float radius);

	static int GetLayerCollisionMask(const std::string& layer);

	static Vector3 GetGravity();
	static void SetGravity(const Vector3& gravity);
private:
	static std::shared_ptr<RigidBody> GetRigidBody(const btCollisionObject* collisionObject);
};