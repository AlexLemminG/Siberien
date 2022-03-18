#pragma once

#include "Component.h"
#include "SMath.h"

class SE_CPP_API Transform : public Component {
	//TODO custom serializer from pos,eulers to matrix
	//TODO store euler angles or use yaml for inspector
public:
	void SetPosition(const Vector3& pos) {
		SetPos(matrix, pos);
	}
	Vector3 GetPosition() const {
		return GetPos(matrix);
	}
	void SetRotation(const Quaternion& rotation) {
		SetRot(matrix, rotation);
	}
	Quaternion GetRotation() const {
		return GetRot(matrix);
	}
	Vector3 GetEulerAngles() const {
		return  GetRotation().ToEulerAngles();
	}
	void SetEulerAngles(const Vector3& euler) {
		SetRotation(Quaternion::FromEulerAngles(euler));
	}
	const Vector3& GetScale() const {
		return scale;
	}
	void SetScale(const Vector3& scale);

	Vector3 GetRight() const {
		return matrix.GetColumn(0).xyz();
	}
	Vector3 GetUp() const {
		return matrix.GetColumn(1).xyz();
	}
	Vector3 GetForward() const {
		return matrix.GetColumn(2).xyz();
	}
	const Matrix4& GetMatrix()const { return matrix; }
	void SetMatrix(const Matrix4& matrix);

private:
	Vector3 scale = Vector3_one;

	Matrix4 matrix = Matrix4::Identity();//TODO remove direct access to matrix for optimization

public:
	static void Deserialize(const SerializationContext& so, Transform& t);
	static void Serialize(SerializationContext& so, const Transform& t);

	REFLECT_CUSTOM(Transform, Transform::Serialize, Transform::Deserialize);
};
