#include "Math.h"
#include "Serialization.h"

void Color::Deserialize(const SerializedObject& serializedObject) {
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

const Vector3 Vector3_zero = Vector3{ 0,0,0 };
const Vector3 Vector3_forward = Vector3{ 0,0,1 };
const Vector3 Vector3_up = Vector3{ 0,1,0 };
const Vector3 Vector3_right = Vector3{ 1,0,0 };