#pragma once

#include "Component.h"
#include "SMath.h"

class SE_CPP_API Transform : public Component {
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
	static void Des(const SerializationContext& so, Transform& t);
	static void Ser(SerializationContext& so, const Transform& t);

	REFLECT_CUSTOM(Transform, Transform::Ser, Transform::Des);
};
