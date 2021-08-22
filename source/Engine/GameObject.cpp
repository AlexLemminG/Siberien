#include "GameObject.h"
#include "Scene.h"

DECLARE_TEXT_ASSET(GameObject);

void GameObject::OnBeforeSerializeCallback(SerializationContext& context) const {
	for (auto c : components) {
		context.AddAllowedToSerializeObject(c);
	}
}

std::shared_ptr<GameObject> GameObject::FindWithTag(const std::string& tag) { return Scene::FindGameObjectByTag(tag); }
