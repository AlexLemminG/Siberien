

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

void AABB::Expand(AABB box) {
	Expand(box.min);
	Expand(box.max);
}


void AABB::Expand(Vector3 pos) {
	min.x = Mathf::Min(min.x, pos.x);
	min.y = Mathf::Min(min.y, pos.y);
	min.z = Mathf::Min(min.z, pos.z);

	max.x = Mathf::Max(max.x, pos.x);
	max.y = Mathf::Max(max.y, pos.y);
	max.z = Mathf::Max(max.z, pos.z);
}

bool AABB::Contains(const Vector3 pos) const {
	return min.x <= pos.x && min.y <= pos.y && min.z <= pos.z && max.x >= pos.x && max.y >= pos.y && max.z >= pos.z;
}

OBB AABB::ToOBB() const {
	auto center = GetCenter();
	Matrix4 matr = Matrix4::Identity();
	SetPos(matr, center);
	return OBB(matr, GetSize());
}

void DeserializeMatrix4(const SerializationContext& context, Matrix4& dst) {
	if (context.IsSequence()) {
		dst = Matrix4(0.f);
		for (int i = 0; i < 16; i++) {
			auto child = context.Child(i);
			if (!child.IsDefined()) {
				break;
			}
			child >> dst(i);
		}
	}
	else {
		Vector3 pos = Vector3_zero;
		Vector3 euler = Vector3_zero;
		Vector3 scale = Vector3_one;
		::Deserialize(context.Child("pos"), pos);
		::Deserialize(context.Child("euler"), euler);
		::Deserialize(context.Child("scale"), scale);
		dst = Matrix4::Transform(pos, Quaternion::FromEulerAngles(Mathf::DegToRad(euler)).ToMatrix(), scale);
	}
}

void SerializeMatrix4(SerializationContext& context, const Matrix4& src) {

	Vector3 pos = GetPos(src);
	Vector3 euler = Mathf::RadToDeg(GetRot(src).ToEulerAngles());
	Vector3 scale = ::GetScale(src);

	Matrix4 transform = Matrix4::Transform(pos, Quaternion::FromEulerAngles(Mathf::DegToRad(euler)).ToMatrix(), scale);
	bool isTransform = true;

	for (int i = 0; i < src.kElements; i++) {
		if (Mathf::Abs(transform[i] - src[i]) > 0.0001f) {
			isTransform = false;
			break;
		}
	}

	if (isTransform) {
		::Serialize(context.Child("pos"), pos);
		::Serialize(context.Child("euler"), euler);
		::Serialize(context.Child("scale"), scale);
	}
	else {
		for (int i = 0; i < src.kElements; i++) {
			::Serialize(context.Child(i), src[i]);
		}
	}
}

bool GeomUtils::IsOverlapping(const Sphere& sphere, const Ray& ray) {
	auto delta = sphere.pos - ray.origin;//TODO project on plane func
	float distance = (delta - Vector3::DotProduct(delta, ray.dir) * ray.dir).Length();
	return distance <= sphere.radius;
}

OBB::OBB(const Matrix4& center, const Vector3& size)
	:center(center), halfSize(size * 0.5f)
{

}

std::array<int, 48> AABB::GetTriangleIndices() const {
	return OBB::GetTriangleIndices();
}

std::array<int, 24> AABB::GetQuadIndices() const {
	return OBB::GetQuadIndices();
}

std::array<Vector3, 8> AABB::GetVertices()const {
	std::array<Vector3, 8> result{
		Vector3{ min.x, max.y, max.z },
		Vector3{ min.x, min.y, max.z },
		Vector3{ max.x, min.y, max.z },
		Vector3{ max.x, max.y, max.z },
		Vector3{ max.x, max.y, min.z },
		Vector3{ max.x, min.y, min.z },
		Vector3{ min.x, min.y, min.z },
		Vector3{ min.x, max.y, min.z }
	};

	return result;
}
std::array<Vector3, 8> OBB::GetVertices()const {
	std::array<Vector3, 8> result{
		center * Vector3{ -halfSize.x, halfSize.y, halfSize.z },
		center * Vector3{ -halfSize.x, -halfSize.y, halfSize.z },
		center * Vector3{ halfSize.x, -halfSize.y, halfSize.z },
		center * Vector3{ halfSize.x, halfSize.y, halfSize.z },
		center * Vector3{ halfSize.x, halfSize.y, -halfSize.z },
		center * Vector3{ halfSize.x, -halfSize.y, -halfSize.z },
		center * Vector3{ -halfSize.x, -halfSize.y, -halfSize.z },
		center * Vector3{ -halfSize.x, halfSize.y, -halfSize.z }
	};
	return result;
}

std::array<int, 48> OBB::GetTriangleIndices() {
	std::array<int, 48> result{
		0, 1, 2,//front
		0, 2, 3,
		4, 5, 6,//back
		4, 6, 7,
		3, 2, 5,//right
		3, 5, 4,
		6, 1, 0,//left
		6, 0, 7,
		0, 3, 4,//top
		0, 4, 7,
		2, 1, 5,//bottom
		5, 1, 6
	};
	return result;
}

std::array<int, 24> OBB::GetQuadIndices() {
	std::array<int, 24> result{
		0, 1, 2, 3, //front
		4, 5, 6, 7, //back
		3, 2, 5, 4,//right
		6, 1, 0, 7,//left
		0, 3, 4, 7,//top
		2, 1, 6, 5,//bottom
	};
	return result;
}

Matrix4 OBB::GetCenterMatrix() const { return center; }

Vector3 OBB::GetSize() const { return halfSize * 2.f; }

Vector3 OBB::GetCenter() const {
	return GetPos(GetCenterMatrix());
}

AABB OBB::ToAABB() const {
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
