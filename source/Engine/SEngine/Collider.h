#pragma once

#include "Component.h"

class btCollisionShape;

class Collider : public Component {
public:
	std::shared_ptr<btCollisionShape> shape = nullptr;

	virtual void OnEnable() override;
	virtual void OnDisable() override;
	virtual void OnValidate() override;

protected:
	virtual std::shared_ptr<btCollisionShape> CreateShape() = 0;

	REFLECT_BEGIN(Collider);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_END();
};