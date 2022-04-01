#pragma once

#include "Component.h"
#include <memory>
#include "Transform.h"

class SE_CPP_API GameObject : public Object {
	friend class Scene;
	friend class InspectorWindow;//TODO just add flags setter?
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
	virtual std::string GetDbgName() const override;

	static std::shared_ptr<GameObject> FindWithTag(const std::string& tag);

	bool IsActive() const;
	void SetActive(bool isActive);

	REFLECT_BEGIN(GameObject, Object);
	REFLECT_VAR(tag);
	REFLECT_VAR(components);
	REFLECT_END();

	uint64_t flags = 0;//TODO back to private
	struct FLAGS {
		enum :uint64_t {
			IS_ACTIVE = 1 << 0,
			IS_HIDDEN_IN_INSPECTOR = 1 << 1
		};
	};
private:

	//TODO optimize
	std::shared_ptr<GameObject> prefab;
};