#include "Collider.h"
#include "btBulletCollisionCommon.h"
#include "RigidBody.h"
#include "GameObject.h"

void Collider::OnEnable() {
	shape = CreateShape();
}

void Collider::OnDisable() {
	shape = nullptr;
}

void Collider::OnValidate() {
	if (!IsEnabled()) {
		return;
	}
	shape = CreateShape();
	auto rb = gameObject()->GetComponent<RigidBody>();
	if (rb) {
		rb->OnValidate();
	}
}
