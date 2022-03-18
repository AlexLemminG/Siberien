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
private:
	Frustum frustum;
	Matrix4 viewProjectionMatrix;
};

class ManualCamera :public ICamera {
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
	float GetFarPlane() const { return farPlane; }
	float GetNearPlane() const { return nearPlane; }
	float GetFov() const { return fov; }
	void SetFov(float fov) { this->fov = fov; }
	virtual Color GetClearColor() const override { return clearColor; }
	void SetClearColor(const Color& color) { clearColor = color; }

	static Camera* GetMain();

	void OnEnable() override;
	void OnDisable() override;


	Ray ScreenPointToRay(Vector2 point);

	void OnBeforeRender();

	virtual const Matrix4& GetViewMatrix() const override { return viewMatrix; }
	virtual const Matrix4& GetProjectionMatrix() const override { return projectionMatrix; }

	static const std::vector<Camera*>& GetAllCameras();

	static std::vector<Camera*> cameras;

	static constexpr const char* editorCameraTag = "EditorCamera";

private:
	Color clearColor = Colors::black;
	float fov = 60.f;
	float nearPlane = 0.1f;
	float farPlane = 100.f;

	static Camera* mainCamera;
	Matrix4 viewMatrix;
	Matrix4 projectionMatrix;

	REFLECT_BEGIN(Camera);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_VAR(clearColor);
	REFLECT_VAR(fov);
	REFLECT_VAR(nearPlane);
	REFLECT_VAR(farPlane);
	REFLECT_END();
};