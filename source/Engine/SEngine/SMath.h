#pragma once

#include "Defines.h"

#include "mathfu/vector.h"
#include "mathfu/matrix.h"
#include "mathfu/quaternion.h"

#include "Reflect.h"

typedef mathfu::Vector<float, 2> Vector2;
typedef mathfu::Vector<float, 3> Vector3;
typedef mathfu::Vector<float, 4> Vector4;
typedef mathfu::Matrix<float, 4, 4> Matrix4;
typedef mathfu::Matrix<float, 3, 3> Matrix3;
typedef mathfu::Quaternion<float> Quaternion;

extern SE_CPP_API const Vector3 Vector3_zero;
extern SE_CPP_API const Vector3 Vector3_one;
extern SE_CPP_API const Vector3 Vector3_forward;
extern SE_CPP_API const Vector3 Vector3_up;
extern SE_CPP_API const Vector3 Vector3_right;
extern SE_CPP_API const Vector3 Vector3_max;

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
inline Quaternion GetRot(const Matrix4& matrix) {
	auto scale = GetScale(matrix);
	auto rot = Matrix4::ToRotationMatrix(matrix);
	rot.GetColumn(0) *= 1.f / scale.x;
	rot.GetColumn(1) *= 1.f / scale.y;
	rot.GetColumn(2) *= 1.f / scale.z;
	return Quaternion::FromMatrix(rot);
}
inline void SetScale(Matrix4& matrix, const Vector3& scale) {
	matrix = Matrix4::Transform(GetPos(matrix), GetRot(matrix).ToMatrix(), scale);
}
inline void SetRot(Matrix4& matrix, const Quaternion& rot) {
	matrix = Matrix4::Transform(GetPos(matrix), rot.ToMatrix(), GetScale(matrix));
}
inline Quaternion LookRotation(const Vector3& forward, const Vector3& up) {
	auto lookAtMatr = Matrix3::LookAt(forward, Vector3_zero, up);
	return Quaternion::FromMatrix(lookAtMatr.Transpose());
}

class SE_CPP_API Mathf {
public:
	static int Round(float f) {
		return (int)(f + 0.5f - (f < 0));
	}
	static float Max(float a, float b) {
		return a > b ? a : b;
	}
	static float Min(float a, float b) {
		return a < b ? a : b;
	}
	static int Max(int a, int b) {
		return a > b ? a : b;
	}
	static int Min(int a, int b) {
		return a < b ? a : b;
	}
	static float Clamp(float f, float a, float b) {
		return f > b ? b : (f < a ? a : f);
	}
	static float Repeat(float t, float period) {
		return t - ((int)(t / period)) * period;
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
	static float Pow(float value, float power) {
		return std::pow(value, power);
	}
	static float Sqrt(float value) {
		return std::sqrt(value);
	}
	static float RadToDeg(float rad) {
		return rad * 180.f / pi;
	}
	static float DegToRad(float deg) {
		return deg * pi / 180.f;
	}
	static float Sin(float f) {
		return std::sin(f);
	}
	static float Cos(float f) {
		return std::cos(f);
	}
	static float Tan(float f) {
		return std::tan(f);
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
	static Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t) {
		return Quaternion::Slerp(a, b, Clamp01(t));
	}
	static float Lerp(const float& a, const float& b, float t) {
		return a + (b - a) * Clamp01(t);
	}
	static float InverseLerp(const float& a, const float& b, float t) {
		return (t - a) / (b - a); //TODO if b-a > 0
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
	static constexpr float pi2 = pi * 2.f;
	static constexpr float epsilon = 0.00001f;
	static constexpr float phi = 1.61803398875f; // golden ratio
};


class Random {
public:
	static float Range(float minInclusive, float maxExclusive) {
		return (rand() / (float)RAND_MAX) * (maxExclusive - minInclusive) + minInclusive;
	}
	static int Range(int minInclusive, int maxExclusive) {
		return rand() % (maxExclusive - minInclusive) + minInclusive;
	}
	static Vector3 InsideUnitSphere() {
		float x = 1;
		float y = 1;
		float z = 1;
		while (x * x + y * y + z * z > 1.f) {
			x = Range(-1.f, 1.f);
			y = Range(-1.f, 1.f);
			z = Range(-1.f, 1.f);
		}
		//TODO can you do better ?
		return Vector3(x, y, z);
	}
};

class Bits {
public:
	template<typename T, typename MASK_T>
	static bool IsMaskTrue(T value, MASK_T mask) {
		return value & mask;
	}

	template<typename T, typename MASK_T>
	static T SetMaskTrue(T value, MASK_T mask) {
		return value | mask;
	}

	template<typename T, typename MASK_T>
	static T SetMaskFalse(T value, MASK_T mask) {
		return value & (~mask);
	}

	template<typename T, typename MASK_T>
	static T SetMask(T original, MASK_T mask, bool value) {
		return value ? SetMaskTrue(original, mask) : SetMaskFalse(original, mask);
	}
};


class SE_CPP_API Color {
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
		c.g = ((rgbaColor >> 16) & 0xff) / 255.f;
		c.b = ((rgbaColor >> 8) & 0xff) / 255.f;
		c.a = ((rgbaColor >> 0) & 0xff) / 255.f;
		return c;
	}

	int ToIntRGBA() const {
		return Mathf::NormalizedFloatToByte(r) << 24 |
			Mathf::NormalizedFloatToByte(g) << 16 |
			Mathf::NormalizedFloatToByte(b) << 8 |
			Mathf::NormalizedFloatToByte(a) << 0;
	}
	int ToIntARGB() const {
		return Mathf::NormalizedFloatToByte(r) << 16 |
			Mathf::NormalizedFloatToByte(g) << 8 |
			Mathf::NormalizedFloatToByte(b) << 0 |
			Mathf::NormalizedFloatToByte(a) << 24;
	}

	static void Deserialize(const SerializationContext& serializedObject, Color& color);
	static void Serialize(SerializationContext& serializedObject, const Color& color);

	static Color Lerp(const Color& a, const Color& b, float t) {
		Color color;
		color.r = Mathf::Lerp(a.r, b.r, t);
		color.g = Mathf::Lerp(a.g, b.g, t);
		color.b = Mathf::Lerp(a.b, b.b, t);
		color.a = Mathf::Lerp(a.a, b.a, t);
		return color;
	}
};

namespace Colors {
	constexpr Color white{ 1.f,1.f,1.f,1.f };
	constexpr Color black{ 0.f,0.f,0.f,1.f };
	constexpr Color red{ 1.f,0.f,0.f,1.f };
	constexpr Color green{ 0.f,1.f,0.f,1.f };
	constexpr Color blue{ 0.f,0.f,1.f,1.f };
	constexpr Color clear{ 0.f,0.f,0.f,0.f };
};

class AABB {
public:
	AABB() = default;
	AABB(const Vector3& min, const Vector3& max) :min(min), max(max) {}
	Vector3 min = Vector3_max;
	Vector3 max = -Vector3_max;

	void Expand(Vector3 pos) {
		min.x = Mathf::Min(min.x, pos.x);
		min.y = Mathf::Min(min.y, pos.y);
		min.z = Mathf::Min(min.z, pos.z);

		max.x = Mathf::Max(max.x, pos.x);
		max.y = Mathf::Max(max.y, pos.y);
		max.z = Mathf::Max(max.z, pos.z);
	}

	Vector3 GetSize() const {
		return max - min;
	}

	Vector3 GetCenter() const {
		return (max + min) * 0.5f;
	}

	bool Contains(const Vector3 pos) const;
};

class Sphere {
public:
	Vector3 pos;
	float radius;

	AABB ToAABB() const;
};

class Ray {
public:
	Ray(Vector3 origin, Vector3 dir) :origin(origin), dir(dir.Normalized()) {}
	const Vector3 origin;
	const Vector3 dir;

	Vector3 GetPoint(float distance) const {
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

inline void SerializeVector(SerializationContext& context, const Vector3& src) {
	context.yamlNode = YAML::Node(YAML::NodeType::Sequence);
	context.yamlNode[0] = src.x;
	context.yamlNode[1] = src.y;
	context.yamlNode[2] = src.z;
}
inline void DeserializeVector(const SerializationContext& context, Vector3& dst) {
	if (!context.IsDefined() || !context.yamlNode.IsSequence() || context.yamlNode.size() < 3) {
		//v = defaultValue;
		return;
	}
	dst.x = context.yamlNode[0].as<float>();
	dst.y = context.yamlNode[1].as<float>();
	dst.z = context.yamlNode[2].as<float>();
}
inline void SerializeVector4(SerializationContext& context, const Vector4& src) {
	context.yamlNode = YAML::Node(YAML::NodeType::Sequence);
	context.yamlNode[0] = src.x;
	context.yamlNode[1] = src.y;
	context.yamlNode[2] = src.z;
	context.yamlNode[3] = src.w;
}
inline void DeserializeVector4(const SerializationContext& context, Vector4& dst) {
	if (!context.IsDefined() || !context.yamlNode.IsSequence() || context.yamlNode.size() < 4) {
		//v = defaultValue;
		return;
	}
	dst.x = context.yamlNode[0].as<float>();
	dst.y = context.yamlNode[1].as<float>();
	dst.z = context.yamlNode[2].as<float>();
	dst.w = context.yamlNode[3].as<float>();
}

class Frustum {
public:
	void SetFromViewProjection(const Matrix4& viewProjection);

	bool IsOverlapingSphere(const Sphere& sphere)const;

private:
	Vector4 frustumPlanes[6];
};

REFLECT_CUSTOM_EXT(Vector3, SerializeVector, DeserializeVector);
REFLECT_CUSTOM_EXT(Vector4, SerializeVector4, DeserializeVector4);
REFLECT_CUSTOM_EXT(Color, Color::Serialize, Color::Deserialize);