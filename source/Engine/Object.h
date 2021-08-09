#pragma once

#include <memory>

class ReflectedTypeBase;
class SerializationContext;

class Object {
public:
	Object() {}
	virtual ~Object() {}

	virtual void OnBeforeSerializeCallback(SerializationContext& context) const {};//TODO make non const
	virtual void OnAfterDeserializeCallback(const SerializationContext& context) {};

	static ReflectedTypeBase* TypeOf();
	virtual	ReflectedTypeBase* GetType() const;

	template<typename T>
	static std::shared_ptr<T> Instantiate(std::shared_ptr<T> original) {
		return std::dynamic_pointer_cast<T>(Instantiate(std::dynamic_pointer_cast<Object>(original)));
	}

	static std::shared_ptr<Object> Instantiate(std::shared_ptr<Object> original);
};