#include "GameObject.h"

DECLARE_TEXT_ASSET(GameObject);

void GameObject::OnBeforeSerializeCallback(SerializationContext& context) const {
	for (auto c : components) {
		context.AddAllowedToSerializeObject(c);
	}
}
