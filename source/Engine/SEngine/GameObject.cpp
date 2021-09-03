#include "GameObject.h"
#include "Scene.h"
#include "SMath.h"

DECLARE_TEXT_ASSET(GameObject);

void GameObject::OnBeforeSerializeCallback(SerializationContext& context) const {
	for (auto c : components) {
		context.AddAllowedToSerializeObject(c);
	}
}

std::shared_ptr<GameObject> GameObject::FindWithTag(const std::string& tag) { return Scene::FindGameObjectByTag(tag); }

bool GameObject::IsActive() const { return Bits::IsMaskTrue(flags, FLAGS::IS_ACTIVE); }
void GameObject::SetActive(bool isActive) {
	if (isActive == IsActive()) { return; }


	Bits::SetMask(flags, FLAGS::IS_ACTIVE, isActive);
}

void GameObject::Des(const SerializationContext& so, GameObject& t) {
	std::shared_ptr<GameObject> prefab;//TODO loading optmization?
	::Deserialize(so.Child("prefab"), prefab);


	::Deserialize(so.Child("tag"), t.tag);
	::Deserialize(so.Child("components"), t.components);
}

void GameObject::Ser(SerializationContext& so, const GameObject& t) {
	//TODO dont manually call
	t.OnBeforeSerializeCallback(so);
	::Serialize(so.Child("tag"), t.tag);
	::Serialize(so.Child("components"), t.components);
}
