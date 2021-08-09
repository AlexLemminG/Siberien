#include "Object.h"
#include "Reflect.h"

class Object_Type : public ReflectedType<Object> {
public:
	Object_Type() : ReflectedType<Object>("Object") {
	}
};

ReflectedTypeBase* Object::TypeOf() {
	static Object_Type t;
	return &t;
}

ReflectedTypeBase* Object::GetType() const {
	return TypeOf();
}
