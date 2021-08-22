#pragma once

#include "Component.h"
#include <memory>
#include "Transform.h"

class SE_CPP_API GameObject : public Object {
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

	REFLECT_BEGIN(GameObject);
	REFLECT_VAR(tag);
	REFLECT_VAR(components);
	REFLECT_END();
};