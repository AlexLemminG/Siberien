#pragma once

#include "Math.h"
#include "Serialization.h"

class Camera {
public:
	Color GetClearColor() { return clearColor; }

	static Camera* GetMain() { return mainCamera; }
	static void SetMain(Camera* mainCamera) { Camera::mainCamera = mainCamera; }
private:
	Color clearColor;
	static Camera* mainCamera;

	REFLECT_BEGIN(Camera);
	REFLECT_VAR(clearColor, Colors::black);
	REFLECT_END();
};