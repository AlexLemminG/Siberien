#pragma once

#include <string>
#include <memory>
#include "yaml-cpp/yaml.h"
#include "Resources.h"

class ReflectedObjectVar {
	std::string name;
};

class SerializedObject {
public:
	SerializedObject(YAML::Node yamlNode) :yamlNode(yamlNode) {}
	SerializedObject(const TextAsset& textAsset) :yamlNode(textAsset.GetYamlNode()) {}

	bool IsValid()const {
		return yamlNode.IsDefined();
	}
	bool IsObject()const {
		return yamlNode.IsMap();
	}

	int AsInt() const { return yamlNode.as<unsigned int>(); }
	int AsFloat() const { return yamlNode.as<float>();  }

	const SerializedObject Child(const std::string& name) const {
		return SerializedObject(yamlNode[name]);
	}

private:
	YAML::Node yamlNode;
};

template<typename Type>
void Deserialize(const SerializedObject& src, Type& dst, const Type& defaultValue) {
	dst.Deserialize(src);
}

inline void Deserialize(const SerializedObject& src, int& dst, const int& defaultValue) {
	dst = src.AsInt();
}

inline void Deserialize(const SerializedObject& src, float& dst, const float& defaultValue) {
	dst = src.AsFloat();
}


class ObjectType {

};

#define CLASS_DECLARATION(className) \
public:\
virtual ObjectType* GetType()const;\
private:\
static ObjectType type;

#define REFLECT_VAR(varName, defaultValue)\
{\
	const auto& child = serializedObject.Child(std::string(#varName)); \
	if(child.IsValid())\
	{\
		::Deserialize(child, varName, defaultValue);\
	}else{\
		varName = defaultValue; \
	}\
	\
}\

#define REFLECT_BEGIN(className) \
public:\
void Deserialize(const SerializedObject& serializedObject){

#define REFLECT_END() \
};

class TestMiniClass {
	float f;
	bool b;
};
class TestSerializationClass {
	int i;
	TestMiniClass c;

	REFLECT_BEGIN(TestSerializationClass);
	REFLECT_VAR(i, 666);
	REFLECT_END();
};