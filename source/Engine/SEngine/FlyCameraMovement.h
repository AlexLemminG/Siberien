#pragma once

#include "Component.h"
#include "GameObject.h"
#include "Transform.h"
#include "Input.h"
#include "STime.h"

class FlyCameraMovement : public Component {
public:
	float speed = 5.f;
	float rotationSpeed = 0.1f;

	void Update() override;

	REFLECT_BEGIN(FlyCameraMovement);
	REFLECT_VAR(speed);
	REFLECT_VAR(rotationSpeed);
	REFLECT_END();
};