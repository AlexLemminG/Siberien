#pragma once

#include "Component.h"
#include "GameObject.h"
#include "Transform.h"
#include "Input.h"
#include "Time.h"

class FlyCameraMovement : public Component {
	float speed = 0.f;
	float rotationSpeed = 0.1f;

	void Update() override;

	REFLECT_BEGIN(FlyCameraMovement);
	REFLECT_VAR(speed, 5.f);
	REFLECT_VAR(rotationSpeed, 0.1f);
	REFLECT_END();
};