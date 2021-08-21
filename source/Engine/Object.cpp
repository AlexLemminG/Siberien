#include "Object.h"
#include "Reflect.h"
#include "Resources.h"

#include <fstream>

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

std::shared_ptr<Object> Object::Instantiate(SerializationContext& serializedOriginal) {
	return AssetDatabase::Get()->LoadFromYaml<Object>(serializedOriginal.yamlNode);
}

SerializationContext Object::Serialize(std::shared_ptr<Object> original) {
	YAML::Node node;
	std::vector<std::shared_ptr<Object>> serializedObjects;
	serializedObjects.push_back(original);
	SerializationContext context{ node, serializedObjects };
	context.database = AssetDatabase::Get();
	::Serialize(context, original);
	context.FlushRequiestedToSerialize();

	return context;
}

std::shared_ptr<Object> Object::Instantiate(std::shared_ptr<Object> original) {
	return Instantiate(Serialize(original));
}
