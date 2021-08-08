#pragma once

#include "Component.h"

class btCollisionShape;

class Collider : public Component {
public:
	btCollisionShape* shape = nullptr;

	virtual void OnEnable() override;
	virtual void OnDisable() override;

protected:
	virtual btCollisionShape* CreateShape() { return nullptr; }; //TODO = 0;

	REFLECT_BEGIN(Collider);
	REFLECT_END();
};