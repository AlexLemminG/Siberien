#pragma once

#include "Component.h"
#include "GameObject.h"

class TopDownCameraRig : public Component {
public:
	float lerpT = 1.f;
	Vector3 offset = Vector3_zero;
	std::string targetTag;

	void OnEnable() override;
	void Update() override;

	REFLECT_BEGIN(TopDownCameraRig);
	REFLECT_VAR(targetTag);
	REFLECT_VAR(lerpT);
	REFLECT_VAR(offset);
	REFLECT_END();

private:
};