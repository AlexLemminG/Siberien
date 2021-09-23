#include "Camera.h"
#include "GameObject.h"
#include "Transform.h"
#include <SDL.h>
#include "Render.h"
#include <bgfx_utils.h>
#include "Graphics.h"
#include "MeshRenderer.h"
#include "Mesh.h"

DECLARE_TEXT_ASSET(Camera);

std::vector<Camera*> Camera::cameras;

Camera* Camera::GetMain() {
	if (Engine::Get()->IsEditorMode()) {
		for (auto camera : cameras) {
			if (camera->gameObject()->tag == "EditorCamera") {
				return camera;
			}
		}
	}
	else {
		for (auto camera : cameras) {
			if (camera->gameObject()->tag != "EditorCamera") {
				return camera;
			}
		}
	}
	return cameras.size() > 0 ? cameras[0] : nullptr;

}

void Camera::OnEnable() { cameras.push_back(this); }

void Camera::OnDisable() { cameras.erase(std::find(cameras.begin(), cameras.end(), this)); }

Ray Camera::ScreenPointToRay(Vector2 screenPoint) {

	int width;
	int height;
	SDL_GetWindowSize(Render::window, &width, &height);

	Vector2 viewPoint{
		(screenPoint.x - width / 2.f) / (width / 2.f),
		(screenPoint.y - height / 2.f) / (height / 2.f)
	};

	float aspect = width / (float)height;

	auto height2 = Mathf::Tan(Mathf::DegToRad(fov / 2.f));
	auto width2 = height2 * aspect;

	height2 *= -viewPoint.y;
	width2 *= viewPoint.x;

	Vector3 dir{ width2, height2, 1 };

	auto extraRot = Quaternion::FromEulerAngles(
		0,//Mathf::DegToRad(fov * viewPoint.y / 2.f),
		Mathf::DegToRad(fov * viewPoint.x / 2.f * aspect),
		0.f);

	auto rot = gameObject()->transform()->GetRotation();// *extraRot.ToMatrix();

	Ray ray{ gameObject()->transform()->GetPosition(), rot * dir };

	return ray;

}

void Camera::OnBeforeRender() {
	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, GetClearColor().ToIntRGBA(), 1.0f, 0);
	viewMatrix = gameObject()->transform()->GetMatrix();
	SetScale(viewMatrix, Vector3_one);
	viewMatrix = viewMatrix.Inverse();

	float proj[16];
	int width = Graphics::Get()->GetScreenWidth();
	int height = Graphics::Get()->GetScreenHeight();
	bx::mtxProj(proj, fov, float(width) / float(height), nearPlane, farPlane, bgfx::getCaps()->homogeneousDepth);
	projectionMatrix = Matrix4(proj);

	ICamera::OnBeforeRender();
}

void ICamera::OnBeforeRender() {
	viewProjectionMatrix = GetProjectionMatrix() * GetViewMatrix();

	frustum.SetFromViewProjection(viewProjectionMatrix);

	position = GetPos(GetViewMatrix().Inverse());//not good not terrible
}

bool ICamera::IsVisible(const Sphere& sphere) const {
	return frustum.IsOverlapingSphere(sphere);
}

bool ICamera::IsVisible(const AABB& aabb) const {
	Sphere sphere;
	sphere.pos = aabb.GetCenter();
	sphere.radius = aabb.GetSize().Length() / 2;
	return frustum.IsOverlapingSphere(sphere);
}

bool ICamera::IsVisible(const MeshRenderer& renderer) const {
	//TODO optimize
	auto sphere = renderer.mesh->boundingSphere;
	const auto scale = renderer.m_transform->GetScale();
	float maxScale = Mathf::Max(Mathf::Max(scale.x, scale.y), scale.z);
	sphere.radius *= maxScale;
	sphere.pos = renderer.m_transform->GetMatrix() * sphere.pos;
	return IsVisible(sphere);
}