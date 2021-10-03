

#include "SMath.h"
#include "Serialization.h"

void Color::Deserialize(const SerializationContext& context, Color& color) {
	if (context.IsMap()) {
		::Deserialize(context.Child("r"), color.r);
		::Deserialize(context.Child("g"), color.g);
		::Deserialize(context.Child("b"), color.b);
		::Deserialize(context.Child("a"), color.a);
	}
	else if (context.IsSequence()) {
		int size = Mathf::Min(4, context.Size());
		for (int i = 0; i < size; i++) {
			::Deserialize(context.Child(i), *(&color.r + i));
		}
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