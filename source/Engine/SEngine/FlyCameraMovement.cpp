#include "FlyCameraMovement.h"

void FlyCameraMovement::Update() {

	Matrix4 matrix = gameObject()->transform()->GetMatrix();
	auto rotation = GetRot(matrix);

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
		rotation = Quaternion::FromEulerAngles(0, -rotationSpeed * Time::deltaTime(), 0) * rotation;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_RIGHT)) {
		rotation = Quaternion::FromEulerAngles(0, rotationSpeed * Time::deltaTime(), 0) * rotation;
	}
	rotation.Normalize();

	deltaPos *= speed * Time::deltaTime();

	SetRot(matrix, rotation);
	SetPos(matrix, GetPos(matrix) + deltaPos);

	gameObject()->transform()->SetMatrix(matrix);
}

DECLARE_TEXT_ASSET(FlyCameraMovement);