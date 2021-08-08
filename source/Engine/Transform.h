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
	Vector3 GetPosition() const {
		return GetPos(matrix);
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

	REFLECT_BEGIN_CUSTOM(Transform, Des);
	REFLECT_END();

private:
	void Des(const SerializedObject& so) {
		Vector3 pos = Vector3_zero;
		Vector3 euler = Vector3_zero;
		Vector3 scale = Vector3_one;

		Deserialize(so.yamlNode["pos"], pos);
		Deserialize(so.yamlNode["euler"], euler);
		Deserialize(so.yamlNode["scale"], scale);

		matrix = Matrix4::Transform(pos, Quaternion::FromEulerAngles(Mathf::DegToRad(euler)).ToMatrix(), scale);
	}
};