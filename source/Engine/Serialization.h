#pragma once

#include <string>
#include <memory>
#include "yaml-cpp/yaml.h"
#include "Resources.h"
#include "Object.h"
#include <type_traits>
#include "Math.h"
#include "Reflect.h"

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

template <typename ImporterType>
class BinaryAssetImporterRegistrator2 {
public:
	BinaryAssetImporterRegistrator2(std::string typeName) {
		AssetDatabase2::RegisterBinaryAssetImporter(typeName, std::make_unique<ImporterType>());
	}
};

template <typename Type>
class SerializedObjectImporter : public TextAssetImporter {
	std::shared_ptr<Object> Import(AssetDatabase& database, const YAML::Node& node) override {
		std::unique_ptr<Type> asset = std::make_unique<Type>();
		auto context = SerializationContext(node);
		Deserialize(context, *(asset.get()));
		return std::shared_ptr<Object>(std::move(asset));
	}
};

#define DECLARE_BINARY_ASSET(className, importerClassName) \
static BinaryAssetImporterRegistrator<##importerClassName> AssetImporterRegistrator_##className{#className};

#define DECLARE_BINARY_ASSET2(className, importerClassName) \
static BinaryAssetImporterRegistrator2<##importerClassName> AssetImporterRegistrator2_##className{#className};

#define DECLARE_TEXT_ASSET(className) \
static TextAssetImporterRegistrator<SerializedObjectImporter<##className>> AssetImporterRegistrator_##className{#className};

#define DECLARE_CUSTOM_TEXT_ASSET(className, importerClassName) \
static TextAssetImporterRegistrator<##importerClassName> AssetImporterRegistrator_##className{#className};