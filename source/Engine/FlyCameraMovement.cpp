#include "FlyCameraMovement.h"

void FlyCameraMovement::Update() {

	Matrix4 matrix = gameObject()->transform()->matrix;
	auto rotation = Quaternion::FromMatrix(matrix);

	Vector3 deltaPos = Vector3_zero;
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_W)) {
		deltaPos += rotation * Vector3_forward;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_S)) {
		deltaPos -= rotation * Vector3_forward;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_A)) {
		deltaPos -= rotation * Vector3_right;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_D)) {
		deltaPos += rotation * Vector3_right;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_E)) {
		deltaPos += rotation * Vector3_up;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_Q)) {
		deltaPos -= rotation * Vector3_up;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_LEFT)) {
		rotation = rotation * Quaternion::FromEulerAngles(0, rotationSpeed * Time::deltaTime(), 0);
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_RIGHT)) {
		rotation = rotation * Quaternion::FromEulerAngles(0, -rotationSpeed * Time::deltaTime(), 0);
	}
	rotation.Normalize();

	deltaPos *= speed * Time::deltaTime();

	matrix = Matrix4::Transform(matrix.TranslationVector3D() + deltaPos, rotation.ToMatrix(), matrix.ScaleVector3D());

	gameObject()->transform()->matrix = matrix;
}

DECLARE_TEXT_ASSET(FlyCameraMovement);