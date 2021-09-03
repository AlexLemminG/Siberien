#pragma once

#include "Component.h"
#include <memory>
#include "Transform.h"

class SE_CPP_API GameObject : public Object {
	friend class Scene;
public:
	std::string tag;
	std::vector<std::shared_ptr<Component>> components;

	template<typename T>
	std::shared_ptr<T> GetComponent() {
		for (int i = 0; i < components.size(); i++) {
			T* casted = dynamic_cast<T*>(components[i].get());
			if (casted != nullptr) {
				return std::dynamic_pointer_cast<T>(components[i]);
			}
		}
		return nullptr;
	}

	std::shared_ptr<Transform> transform() {
		return GetComponent<Transform>();
	}

	virtual void OnBeforeSerializeCallback(SerializationContext& context) const override;

	static std::shared_ptr<GameObject> FindWithTag(const std::string& tag);

	bool IsActive() const;
	void SetActive(bool isActive);

	static void Des(const SerializationContext& so, GameObject& t);
	static void Ser(SerializationContext& so, const GameObject& t);

	REFLECT_CUSTOM(GameObject, GameObject::Ser, GameObject::Des);
private:
	uint64_t flags = 0;
	struct FLAGS {
		enum :uint64_t {
			IS_ACTIVE = 1 << 0
		};
	};

	//TODO optimize
	std::shared_ptr<GameObject> prefab;
};