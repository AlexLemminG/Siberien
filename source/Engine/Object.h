#pragma once

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
};