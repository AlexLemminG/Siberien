#pragma once

#include <memory>
#include "Defines.h"

class ReflectedTypeBase;
class SerializationContext;

class SE_CPP_API Object {
public:
	Object();
	virtual ~Object();

	virtual void OnBeforeSerializeCallback(SerializationContext& context) const;;//TODO make non const
	virtual void OnAfterDeserializeCallback(const SerializationContext& context);;

	static ReflectedTypeBase* TypeOf();
	virtual	ReflectedTypeBase* GetType() const;

	static std::shared_ptr<Object> Instantiate(std::shared_ptr<Object> original);

	// todo move to different file
	static std::shared_ptr<Object> Instantiate(SerializationContext& serializedOriginal);
	static SerializationContext Serialize(std::shared_ptr<Object> original);


	template<typename T>
	static std::shared_ptr<T> Instantiate(std::shared_ptr<T> original) {
		return std::dynamic_pointer_cast<T>(Object::Instantiate(std::dynamic_pointer_cast<Object>(original)));
	}
};