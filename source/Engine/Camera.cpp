#include "Camera.h"
#include "GameObject.h"
#include "Transform.h"
#include <SDL.h>
#include "Render.h"

Camera* Camera::mainCamera = nullptr;

DECLARE_TEXT_ASSET(Camera);

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

	auto rot = Matrix4::ToRotationMatrix(gameObject()->transform()->matrix);// *extraRot.ToMatrix();

	Ray ray{ gameObject()->transform()->GetPosition(), rot * dir };

	return ray;

}
