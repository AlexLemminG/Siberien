#include "GameObject.h"
#include "Scene.h"
#include "SMath.h"
#include "Asserts.h"
#include "Component.h"

DECLARE_TEXT_ASSET(GameObject);

std::shared_ptr<Component> GameObject::GetComponent(const std::string& type) {
	for (int i = 0; i < components.size(); i++) {
		if (components[i].get()->GetType()->GetName() == type) {
			return std::dynamic_pointer_cast<Component>(components[i]);
		}
	}
	return nullptr;
}

std::shared_ptr<Component> GameObject::AddComponent(const std::string& type) {
	ryml::Tree t = ryml::parse(c4::to_csubstr((type + ": ~").c_str()));
	auto context = SerializationContext(t);
	auto obj = std::dynamic_pointer_cast<Component>(Object::Instantiate(context));
	if (obj) {
		GetScene()->AddComponent(this, obj);
		return obj;
	}
	else {
		//error + check if type was not a component type
		ASSERT(false);
		return nullptr;
	}

}

void GameObject::OnBeforeSerializeCallback(SerializationContext& context) const {
	for (auto c : components) {
		context.AddAllowedToSerializeObject(c);
	}
}

std::shared_ptr<GameObject> GameObject::FindWithTag(const std::string& tag) { return Scene::Get()->FindGameObjectByTag(tag); }

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