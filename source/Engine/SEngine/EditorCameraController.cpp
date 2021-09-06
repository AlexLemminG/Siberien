#include "Component.h"
#include "Input.h"
#include "GameObject.h"
#include "STime.h"
#include "SDL.h"

class EditorCameraController : public Component {
public:
	bool isMouseHidden = false;//TODO move to Engine class or something
	int mouseXBeforeHide = 0;
	int mouseYBeforeHide = 0;

	virtual void Update() override {
		if (!Engine::Get()->IsEditorMode()) {
			return;//TODO dont create camera in first place
		}

		if (!Input::GetKey(SDL_SCANCODE_Z) && !Input::GetMouseButton(1)) {
			if (isMouseHidden) {
				isMouseHidden = false;
				SDL_SetRelativeMouseMode(SDL_FALSE);
				SDL_WarpMouseGlobal(mouseXBeforeHide, mouseYBeforeHide);
			}
			return;
		}
		if (!isMouseHidden) {
			isMouseHidden = true;
			SDL_GetGlobalMouseState(&mouseXBeforeHide, &mouseYBeforeHide);
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}


		float moveSpeed = 5.f;
		float moveSpeedFast = 15.f;
		float rotateSpeed = 10.f;

		if (Input::GetKey(SDL_SCANCODE_LCTRL)) {
			moveSpeed = moveSpeedFast;
		}


		auto transform = gameObject()->transform();
		Vector3 vel = Vector3_zero;
		if (Input::GetKey(SDL_SCANCODE_W)) {
			vel += transform->GetForward();
		}
		if (Input::GetKey(SDL_SCANCODE_S)) {
			vel += -transform->GetForward();
		}
		if (Input::GetKey(SDL_SCANCODE_D)) {
			vel += transform->GetRight();
		}
		if (Input::GetKey(SDL_SCANCODE_A)) {
			vel += -transform->GetRight();
		}
		transform->SetPosition(transform->GetPosition() + vel * Time::deltaTime() * moveSpeed);
	
	
		auto rotation = transform->GetRotation();
		auto r1 = Quaternion::FromAngleAxis(Mathf::DegToRad(Time::deltaTime() * rotateSpeed) * Input::GetMouseDeltaPosition().x, Vector3_up);
		auto r2 = Quaternion::FromAngleAxis(Mathf::DegToRad(Time::deltaTime() * rotateSpeed) * Input::GetMouseDeltaPosition().y, transform->GetRight());
		transform->SetRotation(r1 * r2 * rotation);
	
	
	}

	REFLECT_BEGIN(EditorCameraController, Component);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_END();
};
DECLARE_TEXT_ASSET(EditorCameraController);