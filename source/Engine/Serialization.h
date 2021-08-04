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
	int AsFloat() const { return yamlNode.as<float>(); }

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

template <typename ImporterType>
class AssetImporterRegistrator {
public:
	AssetImporterRegistrator(std::string typeName) {
		AssetDatabase::RegisterImporter(typeName, std::make_unique<ImporterType>());
	}
};

template <typename Type>
class SerializedObjectImporter : public AssetImporter {
	std::shared_ptr<Asset> Import(AssetDatabase& database, const std::string& path) override {
		auto textAsset = database.LoadTextAsset(path);
		if (textAsset == nullptr) {
			return nullptr;
		}
		std::unique_ptr<Type> asset = std::make_unique<Type>();
		auto serializedObject = SerializedObject(*textAsset);
		asset->Deserialize(serializedObject);
		return std::shared_ptr<Asset>(std::move(asset));
	}
};

#define DECLARE_ASSET(className) \
static AssetImporterRegistrator<SerializedObjectImporter<##className>> AssetImporterRegistrator_##className{#className};

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