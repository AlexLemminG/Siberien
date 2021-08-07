#pragma once

#include <string>
#include <memory>
#include "yaml-cpp/yaml.h"
#include "Resources.h"
#include "Object.h"
#include <type_traits>
#include "Math.h"

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

public:
	YAML::Node yamlNode;
};

template<typename Type,
	typename Type2,
	typename = std::enable_if_t<!(
		std::is_base_of<Object, Type>::value
		&& std::is_assignable<std::shared_ptr<Type>, Type2>::value)>
>
void Deserialize(const SerializedObject& src, Type& dst, const Type2& defaultValue) {
	dst.Deserialize(src);
}

template<typename Type,
	typename Type2,
	typename = std::enable_if_t<
	std::is_base_of<Object, Type>::value
	&& std::is_assignable<std::shared_ptr<Type>, Type2>::value>
>
inline void Deserialize(const SerializedObject& src, std::shared_ptr<Type>& dst, const Type2& defaultValue) {
	auto path = src.yamlNode.as<std::string>();
	AssetDatabase::Get()->RequestObjectPtr(dst, path);
}

template<typename Type>
inline void Deserialize(const SerializedObject& src, std::vector<Type>& dst, const std::vector<Type>& defaultValue) {
	//dst = nullptr;
	if (src.yamlNode.IsSequence()) {
		for (const auto& v : src.yamlNode) {
			dst.push_back(Type{});
		}
		int i = 0;
		for (const auto& v : src.yamlNode) {
			::Deserialize(v, dst[i], Type{});
			i++;
		}
	}
}


inline void Deserialize(const SerializedObject& src, int& dst, const int& defaultValue) {
	dst = src.AsInt();
}

inline void Deserialize(const SerializedObject& src, float& dst, const float& defaultValue) {
	dst = src.AsFloat();
}


inline void Deserialize(const SerializedObject& src, Vector3& dst, const Vector3& defaultValue) {
	dst.x = src.yamlNode[0].as<float>();
	dst.y = src.yamlNode[1].as<float>();
	dst.z = src.yamlNode[2].as<float>();
}
//
//inline void Deserialize(const SerializedObject& src, Object*& dst, const Object*& defaultValue) {
//	dst = nullptr;
//}
//
//inline void Deserialize(const SerializedObject& src, std::shared_ptr<Object>& dst, const std::shared_ptr<Object>& defaultValue) {
//	dst = nullptr;
//}

//TOOD check that Type is Object



class ObjectType {

};

#define CLASS_DECLARATION(className) \
public:\
virtual ObjectType* GetType()const;\
private:\
static ObjectType type;

//TODO remove default and just use original varName value
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
class TextAssetImporterRegistrator {
public:
	TextAssetImporterRegistrator(std::string typeName) {
		AssetDatabase::RegisterTextAssetImporter(typeName, std::make_unique<ImporterType>());
	}
};

template <typename ImporterType>
class BinaryAssetImporterRegistrator {
public:
	BinaryAssetImporterRegistrator(std::string typeName) {
		AssetDatabase::RegisterBinaryAssetImporter(typeName, std::make_unique<ImporterType>());
	}
};

template <typename Type>
class SerializedObjectImporter : public TextAssetImporter {
	std::shared_ptr<Object> Import(AssetDatabase& database, const YAML::Node& node) override {
		std::unique_ptr<Type> asset = std::make_unique<Type>();
		auto serializedObject = SerializedObject(node);
		asset->Deserialize(serializedObject);
		return std::shared_ptr<Object>(std::move(asset));
	}
};

#define DECLARE_BINARY_ASSET(className, importerClassName) \
static BinaryAssetImporterRegistrator<##importerClassName> AssetImporterRegistrator_##className{#className};

#define DECLARE_TEXT_ASSET(className) \
static TextAssetImporterRegistrator<SerializedObjectImporter<##className>> AssetImporterRegistrator_##className{#className};

#define DECLARE_CUSTOM_TEXT_ASSET(className, importerClassName) \
static TextAssetImporterRegistrator<##importerClassName> AssetImporterRegistrator_##className{#className};

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