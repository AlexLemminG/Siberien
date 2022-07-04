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

std::shared_ptr<Scene> GameObject::GetScene() const { 
	//TODO actual scene of GameObject
	return Scene::Get();
}

std::string GameObject::GetDbgName() const
{
	return "GameObject '" + tag + "'";
}