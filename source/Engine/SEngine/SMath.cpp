

#include "SMath.h"
#include "bgfx_utils.h"

void Frustum::SetFromViewProjection(const Matrix4& viewProjection) {
	bgfx_examples::buildFrustumPlanes((bx::Plane*)frustumPlanes, &viewProjection(0, 0));
}

bool Frustum::IsOverlapingSphere(const Sphere& sphere) const
{
	bool res = true;
	for (int i = 0; i < 6; i++)
	{
		if (frustumPlanes[i].x * sphere.pos.x + frustumPlanes[i].y * sphere.pos.y +
			frustumPlanes[i].z * sphere.pos.z + frustumPlanes[i].w <= -sphere.radius)
			res = false;
	}
	return res;
}

bool AABB::Contains(const Vector3 pos) const {
	return min.x <= pos.x && min.y <= pos.y && min.z <= pos.z && max.x >= pos.x && max.y >= pos.y && max.z >= pos.z;
}
