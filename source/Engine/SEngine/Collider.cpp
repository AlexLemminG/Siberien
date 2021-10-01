#include "Collider.h"
#include "btBulletCollisionCommon.h"

void Collider::OnEnable() {
	shape = CreateShape();
}

void Collider::OnDisable() {

}
