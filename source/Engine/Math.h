#pragma once

#include "mathfu/vector.h"
#include "mathfu/matrix.h"
#include "mathfu/quaternion.h"

typedef mathfu::Vector<float, 2> Vector2;
typedef mathfu::Vector<float, 3> Vector3;
typedef mathfu::Vector<float, 4> Vector4;
typedef mathfu::Matrix<float, 4, 4> Matrix4;
typedef mathfu::Matrix<float, 3, 3> Matrix3;
typedef mathfu::Quaternion<float> Quaternion;

extern const Vector3 Vector3_zero;
extern const Vector3 Vector3_one;
extern const Vector3 Vector3_forward;
extern const Vector3 Vector3_up;
extern const Vector3 Vector3_right;

class SerializedObject;


inline void SetPos(Matrix4& matrix, const Vector3& pos) {
	matrix.GetColumn(3) = Vector4(pos, 1);
}
inline Vector3 GetPos(const Matrix4& matrix) {
	return matrix.GetColumn(3).xyz();
}
inline Vector3 GetScale(const Matrix4& matrix) {
	return matrix.ScaleVector3D();
}
inline void SetScale(Matrix4& matrix, const Vector3& scale) {
	matrix = Matrix4::Transform(GetPos(matrix), Matrix4::ToRotationMatrix(matrix), scale);
}
inline Quaternion GetRot(const Matrix4& matrix) {
	auto scale = GetScale(matrix);
	auto rot = Matrix4::ToRotationMatrix(matrix);
	rot.GetColumn(0) *= 1.f / scale.x;
	rot.GetColumn(1) *= 1.f / scale.y;
	rot.GetColumn(2) *= 1.f / scale.z;
	return Quaternion::FromMatrix(rot);
}
inline void SetRot(Matrix4& matrix, const Quaternion& rot) {
	matrix = Matrix4::Transform(GetPos(matrix), rot.ToMatrix(), GetScale(matrix));
}
inline Quaternion LookRotation(const Vector3& forward, const Vector3& up) {
	auto lookAtMatr = Matrix3::LookAt(forward, Vector3_zero, up);
	return Quaternion::FromMatrix(lookAtMatr.Transpose());
}

class Mathf {
public:
	static int Round(float f) {
		return (int)(f + 0.5f - (f < 0));
	}
	static float Clamp(float f, float a, float b) {
		return f > b ? b : (f < a ? a : f);
	}
	static int Clamp(int i, int a, int b) {
		return i > b ? b : (i < a ? a : i);
	}
	static float Clamp01(float f) {
		return Clamp(f, 0.f, 1.f);
	}
	static int NormalizedFloatToByte(float f) {
		return Clamp(Round(f * 255.f), 0, 255);
	}
	static float ByteToNormalizedFloat(int byte) {
		return Clamp(byte, 0, 255) / 255.f;
	}
	static float RadToDeg(float rad) {
		return rad * 180.f / pi;
	}
	static float DegToRad(float deg) {
		return deg * pi / 180.f;
	}
	static float Sin(float f) {
		return sinf(f);
	}
	static float Cos(float f) {
		return cosf(f);
	}
	static float Tan(float f) {
		return tanf(f);
	}
	static Vector3 RadToDeg(Vector3 rad) {
		return rad * (180.f / pi);
	}
	static Vector3 DegToRad(Vector3 deg) {
		return deg * (pi / 180.f);
	}
	static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
		return a + (b - a) * Clamp01(t);
	}
	static Vector3 ClampLength(const Vector3& vec, float maxLength) {
		float length = vec.Length();
		if (length > maxLength) {
			return vec * (maxLength / length);
		}
		return vec;
	}
	static float Abs(float f) {
		return f < 0 ? -f : f;
	}

	static constexpr float pi = 3.14f;//TODO lol
	static constexpr float pi2 = pi * 2;
	static constexpr float epsilon = 0.00001f;
};


class Color {
public:
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;
	float a = 0.f;

	constexpr Color() {}
	constexpr Color(float r, float g, float b, float a = 1.f)
		:r(r), g(g), b(b), a(a) {}

	static Color FromIntRGBA(int rgbaColor) {
		Color c;
		c.r = ((rgbaColor >> 24) & 0xff) / 255.f;
		c.b = ((rgbaColor >> 16) & 0xff) / 255.f;
		c.g = ((rgbaColor >> 8) & 0xff) / 255.f;
		c.a = ((rgbaColor >> 0) & 0xff) / 255.f;
		return c;
	}

	int ToIntRGBA() const {
		return Mathf::NormalizedFloatToByte(r) << 24 |
			Mathf::NormalizedFloatToByte(g) << 16 |
			Mathf::NormalizedFloatToByte(b) << 8 |
			Mathf::NormalizedFloatToByte(a) << 0;
	}

	void Deserialize(const SerializedObject& serializedObject);
};

namespace Colors {
	constexpr Color white{ 1.f,1.f,1.f,1.f };
	constexpr Color black{ 0.f,0.f,0.f,1.f };
	constexpr Color red{ 1.f,0.f,0.f,1.f };
	constexpr Color green{ 0.f,1.f,0.f,1.f };
	constexpr Color blue{ 0.f,0.f,1.f,1.f };
	constexpr Color clear{ 0.f,0.f,0.f,0.f };
};


class Ray {
public:
	Ray(Vector3 origin, Vector3 dir) :origin(origin), dir(dir.Normalized()) {}
	const Vector3 origin;
	const Vector3 dir;

	Vector3 GetPoint(float distance) const{
		return origin + distance * dir;
	}
};

class Plane {
public:
	Plane(Vector3 origin, Vector3 normal) :origin(origin), normal(normal.Normalized()) {}
	const Vector3 origin;
	const Vector3 normal;

	bool Raycast(const Ray& ray, float& distance) {
		float projDst = Vector3::DotProduct(ray.origin - origin, normal);
		float projRayDir = -Vector3::DotProduct(ray.dir, normal);

		distance = projDst / projRayDir;
		return distance > 0.f;
	}
};