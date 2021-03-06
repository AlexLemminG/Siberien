

#include "Object.h"
#include "Reflect.h"
#include "Resources.h"

#include <fstream>

class Object_Type : public ReflectedType<Object> {
public:
	Object_Type() : ReflectedType<Object>("Object") {
	}
};

Object::Object() {}

Object::~Object() {}

void Object::OnBeforeSerializeCallback(SerializationContext& context) const {}

void Object::OnAfterDeserializeCallback(const SerializationContext& context) {}

ReflectedTypeBase* Object::TypeOf() {
	static Object_Type t;
	return &t;
}

ReflectedTypeBase* Object::GetType() const {
	return TypeOf();
}

std::shared_ptr<Object> Object::Instantiate(SerializationContext& serializedOriginal) {
	return AssetDatabase::Get()->DeserializeFromYAML<Object>(serializedOriginal.GetYamlNode());
}

SerializationContext Object::Serialize(std::shared_ptr<Object> original) {
	std::vector<std::shared_ptr<Object>> serializedObjects;
	serializedObjects.push_back(original);
	SerializationContext context{ serializedObjects };
	context.database = AssetDatabase::Get();
	::Serialize(context, original);
	context.FlushRequiestedToSerialize();

	return context;
}

std::string Object::GetDbgName() const
{
	return GetType()->GetName();
}

std::shared_ptr<Object> Object::Instantiate(std::shared_ptr<Object> original) {
	auto so = Serialize(original);
	return Instantiate(so);
}
