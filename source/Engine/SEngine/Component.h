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
	std::shared_ptr<GameObject> gameObject() const{ return m_gameObject.lock(); }

	void SetEnabled(bool isEnabled);
	bool IsEnabled()const;

	enum FLAGS :uint32_t {
		IS_ENABLED = 1 << 0,
		IGNORE_UPDATE = 1 << 1,
		IGNORE_FIXED_UPDATE = 1 << 2
	};
	bool HasFlag(FLAGS flag) const { return flags & flag; }
	FLAGS GetFlags() const { return flags; }
	void SetFlags(const FLAGS flags) { this->flags = flags; }
private:
	FLAGS flags = (FLAGS)0;
	std::weak_ptr<GameObject> m_gameObject;

};

#include "Serialization.h"
