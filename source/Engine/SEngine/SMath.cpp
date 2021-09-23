

#include "SMath.h"
#include "bgfx_utils.h"

void Frustum::SetFromViewProjection(const Matrix4& viewProjection) {
	bgfx_examples::buildFrustumPlanes((bx::Plane*)frustumPlanes, &viewProjection(0, 0));
	matrix = viewProjection;
}

bool Frustum::IsOverlapingSphere(const Sphere& sphere) const
{
	for (int i = 0; i < 6; i++)
	{
		if (frustumPlanes[i].x * sphere.pos.x + frustumPlanes[i].y * sphere.pos.y +
			frustumPlanes[i].z * sphere.pos.z + frustumPlanes[i].w <= -sphere.radius) {
			return false;
		}
	}
	return true;
}

Matrix4 Frustum::GetMatrix() const { return matrix; }

bool AABB::Contains(const Vector3 pos) const {
	return min.x <= pos.x && min.y <= pos.y && min.z <= pos.z && max.x >= pos.x && max.y >= pos.y && max.z >= pos.z;
}

OBB AABB::ToOBB() const{
	auto center = GetCenter();
	Matrix4 matr = Matrix4::Identity();
	SetPos(matr, center);
	return OBB(matr, GetSize());
}

//TODO static func of namespace
bool IsOverlapping(const Sphere& sphere, Ray ray) {
	auto delta = sphere.pos - ray.origin;//TODO project on plane func
	float distance = (delta - Vector3::DotProduct(delta, ray.dir) * ray.dir).Length();
	return distance <= sphere.radius;
}

OBB::OBB(const Matrix4& center, const Vector3& size)
	:center(center), halfSize(size * 0.5f)
{

}
std::array<Vector3, 8> OBB::GetVertices()const {
	std::array<Vector3, 8> result;

	result[0] = center * Vector3{ halfSize.x, halfSize.y, halfSize.z };
	result[1] = center * Vector3{ -halfSize.x, halfSize.y, halfSize.z };
	result[2] = center * Vector3{ halfSize.x, -halfSize.y, halfSize.z };
	result[3] = center * Vector3{ halfSize.x, halfSize.y, -halfSize.z };
	result[4] = center * Vector3{ halfSize.x, -halfSize.y, -halfSize.z };
	result[5] = center * Vector3{ -halfSize.x, halfSize.y, -halfSize.z };
	result[6] = center * Vector3{ -halfSize.x, -halfSize.y, halfSize.z };
	result[7] = center * Vector3{ -halfSize.x, -halfSize.y, -halfSize.z };

	return result;
}

Matrix4 OBB::GetCenterMatrix() const { return center; }

Vector3 OBB::GetSize() const { return halfSize * 2.f; }

Vector3 OBB::GetCenter() const {
	return GetPos(GetCenterMatrix());
}

AABB OBB::ToAABB() const{
	AABB aabb;
	for (auto v : GetVertices()) {
		aabb.Expand(v);
	}
	return aabb;
}

OBB operator*(const Matrix4& matrix, const AABB& aabb) {
	return matrix * aabb.ToOBB();
}
OBB operator*(const Matrix4& matrix, const OBB& obb) {
	return OBB(matrix * obb.GetCenterMatrix(), obb.GetSize());
}

Sphere Sphere::Combine(const Sphere& a, const Sphere& b) {
	auto delta = a.pos - b.pos;
	if (delta.LengthSquared() == 0.f) {
		return Sphere(a.pos, Mathf::Max(a.radius, b.radius));
	}
	float distance = delta.Length();
	delta.Normalize();
	auto center = (a.pos + b.pos + delta * (a.radius - b.radius)) * 0.5f;
	float radius = (a.pos - center).Length() + a.radius;
	return Sphere(center, radius);
}

AABB Sphere::ToAABB() const {
	AABB aabb;
	aabb.min = pos - Vector3_one * radius;
	aabb.max = pos + Vector3_one * radius;
	return aabb;
}
