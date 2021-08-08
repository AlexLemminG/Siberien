#pragma once

#include "Object.h"
#include "Serialization.h"

class GameObject;
class Scene;

class Component : public Object {
	friend Scene;
public:
	virtual void OnEnable() {}
	virtual void Update() {}//TODO use scheduler instead
	virtual void FixedUpdate() {}//TODO use scheduler instead
	virtual void OnDisable() {}

	std::shared_ptr<GameObject> gameObject() { return m_gameObject; }


private:
	std::shared_ptr<GameObject> m_gameObject;
};