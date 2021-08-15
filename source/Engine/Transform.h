#pragma once

#include "Component.h"
#include "Math.h"

class Transform : public Component {
	//TODO private
	//TODO custom serializer from pos,eulers to matrix

public:
	void SetPosition(const Vector3& pos) {
		SetPos(matrix, pos);
	}
	void SetRotation(const Quaternion& rotation) {
		SetRot(matrix, rotation);
	}
	Vector3 GetPosition() const {
		return GetPos(matrix);
	}
	Vector3 GetScale() const {
		return ::GetScale(matrix);
	}
	void SetScale(const Vector3& scale) {
		return ::SetScale(matrix, scale);
	}

	Vector3 GetRight() const {
		return matrix.GetColumn(0).xyz();
	}
	Vector3 GetUp() const {
		return matrix.GetColumn(1).xyz();
	}
	Vector3 GetForward() const {
		return matrix.GetColumn(2).xyz();
	}

public:
	Matrix4 matrix = Matrix4::Identity();


public:
	static void Des(const SerializationContext& so, Transform& t) {
		Vector3 pos = Vector3_zero;
		Vector3 euler = Vector3_zero;
		Vector3 scale = Vector3_one;

		Deserialize(so.Child("pos"), pos);
		Deserialize(so.Child("euler"), euler);
		Deserialize(so.Child("scale"), scale);

		t.matrix = Matrix4::Transform(pos, Quaternion::FromEulerAngles(Mathf::DegToRad(euler)).ToMatrix(), scale);
	}
	static void Ser(SerializationContext& so, const Transform& t) {
		Vector3 pos = GetPos(t.matrix);
		Vector3 euler = Mathf::RadToDeg(GetRot(t.matrix).ToEulerAngles());
		Vector3 scale = ::GetScale(t.matrix);

		Serialize(so.Child("pos"), pos);
		Serialize(so.Child("euler"), euler);
		Serialize(so.Child("scale"), scale);
	}

	REFLECT_CUSTOM(Transform, Transform::Ser, Transform::Des);
};
