#pragma once

#include "SMath.h"
#include "Serialization.h"
#include "Resources.h"
#include "Component.h"

class Sphere;
class MeshRenderer;

class ICamera {
public:
	virtual Color GetClearColor()const = 0;

	virtual const Matrix4& GetViewMatrix() const = 0;
	virtual const Matrix4& GetProjectionMatrix() const = 0;

	bool IsVisible(const Sphere& sphere) const;
	bool IsVisible(const AABB& aabb) const;
	bool IsVisible(const MeshRenderer& renderer) const;


	virtual void OnBeforeRender();
	const Matrix4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const Frustum& GetFrustum() const { return frustum; }
	const Vector3& GetPosition() const { return position; }
private:
	Frustum frustum;
	Matrix4 viewProjectionMatrix;
	Vector3 position;
};

class ManualCamera :public ICamera{
public:
	virtual Color GetClearColor() const override { return color; }

	virtual const Matrix4& GetViewMatrix() const override { return viewMatrix; }
	virtual const Matrix4& GetProjectionMatrix() const override { return projectionMatrix; }

	Color color;
	Matrix4 viewMatrix;
	Matrix4 projectionMatrix;
};

class SE_CPP_API Camera :public Component, public ICamera {
public:
	float GetFarPlane() { return farPlane; }
	float GetNearPlane() { return nearPlane; }
	float GetFov() { return fov; }
	void SetFov(float fov) { this->fov = fov; }
	virtual Color GetClearColor() const override { return clearColor; }

	static Camera* GetMain() { return mainCamera; }
	static void SetMain(Camera* mainCamera) { Camera::mainCamera = mainCamera; }


	void OnEnable() override { SetMain(this); }
	void OnDisable() override { SetMain(nullptr); }

	Ray ScreenPointToRay(Vector2 point);

	void OnBeforeRender();

	virtual const Matrix4& GetViewMatrix() const override { return viewMatrix; }
	virtual const Matrix4& GetProjectionMatrix() const override { return projectionMatrix; }

private:
	Color clearColor = Colors::black;
	float fov = 60.f;
	float nearPlane = 0.1f;
	float farPlane = 100.f;

	static Camera* mainCamera;
	Matrix4 viewMatrix;
	Matrix4 projectionMatrix;

	REFLECT_BEGIN(Camera);
	REFLECT_VAR(clearColor);
	REFLECT_VAR(fov);
	REFLECT_VAR(nearPlane);
	REFLECT_VAR(farPlane);
	REFLECT_END();
};