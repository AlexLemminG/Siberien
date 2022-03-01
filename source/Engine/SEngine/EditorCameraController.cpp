#include "Component.h"
#include "Input.h"
#include "GameObject.h"
#include "STime.h"
#include "SDL.h"
#include "Camera.h"
#include "Dbg.h"

//TODO not static so f5 works
static bool copyPos = true;
static Matrix4 editorTransformMatrix = Matrix4::Identity();
static float baseSpeed = 5.0f;
static float speedMultiplier = 1.f;
static float lastSpeedChangeTime = 0.f;

class EditorCameraController : public Component {
public:
	bool isMouseHidden = false;//TODO move to Engine class or something
	int mouseXBeforeHide = 0;
	int mouseYBeforeHide = 0;

	bool initedCameraTransform = false;

	virtual void Update() override {
		if (!Engine::Get()->IsEditorMode()) {
			return;//TODO dont create camera in first place
		}
		if (Time::getRealTime() - lastSpeedChangeTime < 2.f) {
			Dbg::Text("CameraSpeed x%.2f", speedMultiplier);
		}
		auto transform = gameObject()->transform();
		if (initedCameraTransform) {
			//camera moved outside (probably by inspector) so stop to copying
			//TODO less hacky
			if (transform->GetPosition() == GetPos(editorTransformMatrix)) {
				copyPos = false;
			}
		}
		{
			// updating fov
			Camera* camera = nullptr;
			for (auto c : Camera::cameras) {
				if (c != gameObject()->GetComponent<Camera>().get()) {
					camera = c;
				}
			}
			if (camera) {
				auto thisCamera = gameObject()->GetComponent<Camera>();
				thisCamera->SetFov(camera->GetFov());
				thisCamera->SetClearColor(camera->GetClearColor());
				if (copyPos) {
					editorTransformMatrix = camera->gameObject()->transform()->GetMatrix();
					gameObject()->transform()->SetMatrix(camera->gameObject()->transform()->GetMatrix());
				}
			}
		}
		if (!initedCameraTransform) {
			initedCameraTransform = true;
			transform->SetMatrix(editorTransformMatrix);
		}

		if (!Input::GetKey(SDL_SCANCODE_Z) && !Input::GetMouseButton(1)) {
			if (isMouseHidden) {
				isMouseHidden = false;
				SDL_SetRelativeMouseMode(SDL_FALSE);
				SDL_WarpMouseGlobal(mouseXBeforeHide, mouseYBeforeHide);
			}
			return;
		}
		if (Input::GetMouseScrollY() != 0.f) {
			float scroll = Input::GetMouseScrollY();
			speedMultiplier = Mathf::Clamp(speedMultiplier * Mathf::Pow(1.4f, scroll), 0.1f, 10.f);
			lastSpeedChangeTime = Time::getRealTime();
		}

		copyPos = false;// TODO fix camera is not planar after copying game camera
		if (!isMouseHidden) {
			isMouseHidden = true;
			SDL_GetGlobalMouseState(&mouseXBeforeHide, &mouseYBeforeHide);
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}


		float moveSpeed = speedMultiplier * baseSpeed;
		float rotateSpeed = 10.f;

		if (Input::GetKey(SDL_SCANCODE_LCTRL)) {
			moveSpeed *= 3.f;
		}


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
		editorTransformMatrix = transform->GetMatrix();
	}

	REFLECT_BEGIN(EditorCameraController, Component);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_END();
};
DECLARE_TEXT_ASSET(EditorCameraController);