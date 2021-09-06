#pragma once

#include "Object.h"

class GameObject;
class Scene;

class SE_CPP_API Component : public Object {
	friend class Scene;
public:
	virtual void Update() {}//TODO use scheduler instead
	virtual void FixedUpdate() {}//TODO use scheduler instead
	virtual void OnEnable() {}
	virtual void OnDisable() {}

	std::shared_ptr<GameObject> gameObject() { return m_gameObject.lock(); }

	void SetEnabled(bool isEnabled);
	bool IsEnabled()const;

	bool ignoreUpdate = false;//TODO flag
	bool ignoreFixedUpdate = false;//TODO flag
private:
	std::weak_ptr<GameObject> m_gameObject;
	bool isEnabled = false;
};

#include "Serialization.h"
