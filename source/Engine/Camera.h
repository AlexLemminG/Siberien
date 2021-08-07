#pragma once

#include "Math.h"
#include "Serialization.h"
#include "Resources.h"
#include "Component.h"

class Camera :public Component {
public:
	Color GetClearColor() { return clearColor; }

	static Camera* GetMain() { return mainCamera; }
	static void SetMain(Camera* mainCamera) { Camera::mainCamera = mainCamera; }

	virtual void OnEnable() { SetMain(this); }
	virtual void OnDisable() { SetMain(nullptr); }
private:
	Color clearColor;
	Vector3 pos;
	Vector3 dir;

	static Camera* mainCamera;

	REFLECT_BEGIN(Camera);
	REFLECT_VAR(pos, Vector3());
	REFLECT_VAR(dir, Vector3());
	REFLECT_VAR(clearColor, Colors::black);
	REFLECT_END();
};