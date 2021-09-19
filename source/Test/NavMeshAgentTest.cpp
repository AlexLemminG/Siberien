#include "Component.h"
#include <Input.h>
#include <Camera.h>
#include <Physics.h>
#include <NavMeshAgent.h>
#include <GameObject.h>

class NavMeshAgentTest_MoveToMouse : public Component {
public:
	virtual void Update() override {
		if (Input::GetMouseButton(0)) {
			auto ray = Camera::GetMain()->ScreenPointToRay(Input::GetMousePosition());
			Physics::RaycastHit hit;
			if (Physics::Raycast(hit, ray, 1000.f)) {
				//SetVelocity(hit.GetPoint() - gameObject()->transform()->GetPosition());
				agent->SetDestination(hit.GetPoint());
			}
		}
	}

	virtual void OnEnable() override {
		agent = gameObject()->GetComponent<NavMeshAgent>();
	}

	std::shared_ptr<NavMeshAgent> agent = nullptr;

	REFLECT_BEGIN(NavMeshAgentTest_MoveToMouse, Component);
	REFLECT_END();
};

DECLARE_TEXT_ASSET(NavMeshAgentTest_MoveToMouse);

class NavMeshAgentTest_WASD : public Component {
public:
	virtual void Update() override {
		Vector3 dir = Vector3_zero;
		if (Input::GetKey(SDL_SCANCODE_W)) {
			dir += Vector3_forward;
		}
		if (Input::GetKey(SDL_SCANCODE_S)) {
			dir -= Vector3_forward;
		}
		if (Input::GetKey(SDL_SCANCODE_D)) {
			dir += Vector3_right;
		}
		if (Input::GetKey(SDL_SCANCODE_A)) {
			dir -= Vector3_right;
		}

		agent->SetVelocity(dir * agent->GetMaxSpeed());
	}

	virtual void OnEnable() override {
		agent = gameObject()->GetComponent<NavMeshAgent>();
	}

	std::shared_ptr<NavMeshAgent> agent = nullptr;

	REFLECT_BEGIN(NavMeshAgentTest_WASD, Component);
	REFLECT_END();
};

DECLARE_TEXT_ASSET(NavMeshAgentTest_WASD);