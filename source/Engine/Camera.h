#pragma once

#include "Math.h"
#include "Serialization.h"
#include "Resources.h"
#include "Component.h"

class Camera :public Component {
public:
	float GetFarPlane() { return farPlane; }
	float GetNearPlane() { return nearPlane; }
	float GetFov() { return fov; }
	Color GetClearColor() { return clearColor; }

	static Camera* GetMain() { return mainCamera; }
	static void SetMain(Camera* mainCamera) { Camera::mainCamera = mainCamera; }

	void OnEnable() override { SetMain(this); }
	void OnDisable() override { SetMain(nullptr); }

	Ray ScreenPointToRay(Vector2 point);
private:
	Color clearColor = Colors::black;
	float fov = 60.f;
	float nearPlane = 0.1f;
	float farPlane = 100.f;

	static Camera* mainCamera;

	REFLECT_BEGIN(Camera);
	REFLECT_VAR(clearColor);
	REFLECT_VAR(fov);
	REFLECT_VAR(nearPlane);
	REFLECT_VAR(farPlane);
	REFLECT_END();
};