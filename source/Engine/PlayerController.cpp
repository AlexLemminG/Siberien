#include "PlayerController.h"

#include "GameObject.h"
#include "Transform.h"
#include "Input.h"
#include "Time.h"
#include "Camera.h"
#include "Dbg.h"

void PlayerController::Update() {
	UpdateMovement();
	UpdateShooting();
}

void PlayerController::UpdateMovement() {
	Matrix4 matrix = gameObject()->transform()->matrix;
	auto rotation = GetRot(matrix);

	Vector3 deltaPos = Vector3_zero;
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_W)) {
		deltaPos += Vector3_forward;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_S)) {
		deltaPos -= Vector3_forward;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_A)) {
		deltaPos -= Vector3_right;
	}
	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_D)) {
		deltaPos += Vector3_right;
	}

	deltaPos *= speed * Time::deltaTime();

	SetPos(matrix, GetPos(matrix) + deltaPos);

	gameObject()->transform()->matrix = matrix;
}

void PlayerController::UpdateShooting() {
	Matrix4 matrix = gameObject()->transform()->matrix;

	Vector3 playerPos = gameObject()->transform()->GetPosition();

	if (!Camera::GetMain()) {
		return;
	}

	auto ray = Camera::GetMain()->ScreenPointToRay(Input::GetMousePosition());
	auto plane = Plane{ playerPos , Vector3_up };

	float dist;
	if (!plane.Raycast(ray, dist)) {
		return;
	}

	Dbg::Draw(ray.GetPoint(dist));

	auto deltaPos = ray.GetPoint(dist) - playerPos;

	if (deltaPos.Length() < 0.1f) {
		return;
	}

	SetRot(matrix, Quaternion::LookAt(deltaPos, Vector3_up));
	gameObject()->transform()->matrix = matrix;
}

DECLARE_TEXT_ASSET(PlayerController);