#include "Component.h"
#include "Scene.h"


class Component_Type : public ReflectedType<Component> {
public:
	Component_Type() : ReflectedType<Component>("Component") {
	}
};
ReflectedTypeBase* Component::TypeOf() {
	static Component_Type t;
	return &t;
}

ReflectedTypeBase* Component::GetType() const {
	return TypeOf();
}

void Component::SetEnabled(bool isEnabled) {
	Scene::Get()->SetComponentEnabledInternal(this, isEnabled);
}

bool Component::IsEnabled() const {
	return GetFlags() & IS_ENABLED;
}