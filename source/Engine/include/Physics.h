#pragma once
#include "SMath.h"

class SE_CPP_API Physics {
public:
	class RaycastHit {
	public:
		RaycastHit() {
		}
		RaycastHit(Vector3 point, Vector3 normal) :point(point), normal(normal) {

		}
		Vector3 GetPoint() { return point; }
		Vector3 GetNormal() { return normal; }
	private:
		Vector3 point;
		Vector3 normal;
	};

	static bool SphereCast(RaycastHit& hit, Ray ray, float radius, float maxDistance);
	static bool Raycast(RaycastHit& hit, Ray ray, float maxDistance);
};