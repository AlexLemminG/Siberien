#pragma once

#include "Component.h"

class PlayerController : public Component {
public:
	void Update() override;
private:

	void UpdateMovement();
	void UpdateShooting();

	float speed = 1.f;
	REFLECT_BEGIN(PlayerController);
	REFLECT_VAR(speed);
	REFLECT_END();
};