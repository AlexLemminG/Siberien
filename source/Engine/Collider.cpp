#include "Collider.h"
#include "btBulletCollisionCommon.h"

DECLARE_TEXT_ASSET(Collider);

void Collider::OnEnable() {
	shape = CreateShape();
}

void Collider::OnDisable() {

}
