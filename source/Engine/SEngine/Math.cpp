

#include "SMath.h"
#include "Serialization.h"

void Color::Deserialize(const SerializationContext& context, Color& color) {
	if (context.IsMap()) {
		::Deserialize(context.Child("r"), color.r);
		::Deserialize(context.Child("g"), color.g);
		::Deserialize(context.Child("b"), color.b);
		::Deserialize(context.Child("a"), color.a);
	}
	else {
		unsigned int i;
		context >> i;
		color = Color::FromIntRGBA(i);
	}
}
void Color::Serialize(SerializationContext& context, const Color& color) {
	::Serialize(context.Child("r"), color.r);
	::Serialize(context.Child("g"), color.g);
	::Serialize(context.Child("b"), color.b);
	::Serialize(context.Child("a"), color.a);
}


const Vector3 Vector3_zero = Vector3{ 0,0,0 };
const Vector3 Vector3_one = Vector3{ 1,1,1 };
const Vector3 Vector3_forward = Vector3{ 0,0,1 };
const Vector3 Vector3_up = Vector3{ 0,1,0 };
const Vector3 Vector3_right = Vector3{ 1,0,0 };
const Vector3 Vector3_max = Vector3{ FLT_MAX,FLT_MAX,FLT_MAX };