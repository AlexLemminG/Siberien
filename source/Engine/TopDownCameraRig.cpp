#include "TopDownCameraRig.h"
#include "Transform.h"
#include "GameObject.h"
#include "Scene.h"
#include "Time.h"

DECLARE_TEXT_ASSET(TopDownCameraRig);

void TopDownCameraRig::OnEnable()
{

	auto target = Scene::FindGameObjectByTag(targetTag);
	if (target != nullptr) {
		auto trans = gameObject()->transform();
		auto targetPos = target->transform()->GetPosition() + offset;

		trans->SetPosition(targetPos);
	}
}

void TopDownCameraRig::Update() {
	auto target = Scene::FindGameObjectByTag(targetTag);
	if (target != nullptr) {
		auto trans = gameObject()->transform();
		auto targetPos = target->transform()->GetPosition() + offset;

		trans->SetPosition(Mathf::Lerp(trans->GetPosition(), targetPos, Time::deltaTime() * lerpT));
	}
}
