

#include "Transform.h"

DECLARE_TEXT_ASSET(Transform);

void Transform::Des(const SerializationContext& so, Transform& t) {
	Vector3 pos = Vector3_zero;
	Vector3 euler = Vector3_zero;
	Vector3 scale = Vector3_one;

	Deserialize(so.Child("pos"), pos);
	Deserialize(so.Child("euler"), euler);
	Deserialize(so.Child("scale"), scale);

	t.matrix = Matrix4::Transform(pos, Quaternion::FromEulerAngles(Mathf::DegToRad(euler)).ToMatrix(), scale);
}

void Transform::Ser(SerializationContext& so, const Transform& t) {
	Vector3 pos = GetPos(t.matrix);
	Vector3 euler = Mathf::RadToDeg(GetRot(t.matrix).ToEulerAngles());
	Vector3 scale = ::GetScale(t.matrix);

	::Serialize(so.Child("pos"), pos);
	::Serialize(so.Child("euler"), euler);
	::Serialize(so.Child("scale"), scale);
}
