#pragma once

#include "Component.h"
#include "SMath.h"

class btCollisionShape;

class Collider : public Component {
public:
	virtual std::shared_ptr<btCollisionShape> CreateShape() const = 0;

	Vector3 GetCenterOffset()const { return center; }
protected:
	Vector3 center = Vector3_zero;

	REFLECT_BEGIN(Collider);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_VAR(center);
	REFLECT_END();
};