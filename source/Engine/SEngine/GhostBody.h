#pragma once

#include "Component.h"
#include "SMath.h"
#include "RigidBody.h"

struct btDefaultMotionState;
struct btCompoundShape;
class btPairCachingGhostObject;
class Transform;

class SE_CPP_API GhostBody : public PhysicsBody {
public:
	btPairCachingGhostObject* GetHandle() { return pBody; }
	virtual void OnEnable() override;
	virtual void FixedUpdate() override;
	virtual void OnDisable() override;

	std::vector<std::shared_ptr<GameObject>> GetOverlappedObjects() const;

	std::string layer;
private:
	std::shared_ptr<btCollisionShape> originalShape;
	btPairCachingGhostObject* pBody = nullptr;
	Transform* transform = nullptr;

	REFLECT_BEGIN(GhostBody, PhysicsBody);
	REFLECT_VAR(layer);
	REFLECT_END();
};