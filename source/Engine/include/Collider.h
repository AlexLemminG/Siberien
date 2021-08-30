#pragma once

#include "Component.h"

class btCollisionShape;

class Collider : public Component {
public:
	std::shared_ptr<btCollisionShape> shape = nullptr;

	virtual void OnEnable() override;
	virtual void OnDisable() override;

protected:
	virtual std::shared_ptr<btCollisionShape> CreateShape() { return nullptr; }; //TODO = 0;

	REFLECT_BEGIN(Collider);
	REFLECT_END();
};