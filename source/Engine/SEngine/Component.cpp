#include "Component.h"
#include "Scene.h"

void Component::SetEnabled(bool isEnabled) {
	Scene::Get()->SetComponentEnabledInternal(this, isEnabled);
}

bool Component::IsEnabled() const {
	return GetFlags() & IS_ENABLED;
}
