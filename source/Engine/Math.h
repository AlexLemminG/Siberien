#pragma once

#include "algebra3.h"
#include "Serialization.h"

typedef vec3 Vector3;
typedef mat4 Matrix;

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

	void Deserialize(const SerializedObject& serializedObject) {
		if (serializedObject.IsObject()) {
			::Deserialize(serializedObject.Child("r"), r, 0.f);
			::Deserialize(serializedObject.Child("g"), g, 0.f);
			::Deserialize(serializedObject.Child("b"), b, 0.f);
			::Deserialize(serializedObject.Child("a"), a, 1.f);
		}
		else {
			*this = FromIntRGBA(serializedObject.AsInt());
		}
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