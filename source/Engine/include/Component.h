#pragma once

#include "Object.h"
#include "Serialization.h"

class GameObject;
class Scene;

class Component : public Object {
	friend Scene;
public:
	virtual void Update() {}//TODO use scheduler instead
	virtual void FixedUpdate() {}//TODO use scheduler instead

	std::shared_ptr<GameObject> gameObject() { return m_gameObject.lock(); }

	void SetEnabled(bool isEnabled) {
		if (this->isEnabled == isEnabled) {
			return;
		}
		this->isEnabled = isEnabled;
		if (isEnabled) {
			OnEnable();
		}
		else {
			OnDisable();
		}
	}

private:
	virtual void OnEnable() {}
	virtual void OnDisable() {}

	std::weak_ptr<GameObject> m_gameObject;
	bool isEnabled = false;

	REFLECT_BEGIN(Component);
	REFLECT_END();
};