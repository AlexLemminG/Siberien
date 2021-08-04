#pragma once

#include "Math.h"
#include "Serialization.h"
#include "Resources.h"

class Camera :public Asset {
public:
	Color GetClearColor() { return clearColor; }

	static std::shared_ptr<Camera> GetMain() { return mainCamera; }
	static void SetMain(std::shared_ptr<Camera> mainCamera) { Camera::mainCamera = mainCamera; }
private:
	Color clearColor;
	static std::shared_ptr<Camera> mainCamera;

	REFLECT_BEGIN(Camera);
	REFLECT_VAR(clearColor, Colors::black);
	REFLECT_END();
};