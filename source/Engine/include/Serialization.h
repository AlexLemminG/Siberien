#pragma once

#include <string>
#include <memory>
#include "Reflect.h"

class TextAssetImporter;
class AssetImporter;
class AssetImporter2;

class AssetDatabase;

class AssetImporter {
public:
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const std::string& path) = 0;
};

class AssetImporter2;
class AssetDatabase2_BinaryImporterHandle;

class TextAssetImporter {
public:
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const YAML::Node& node) = 0;
};
class AssetImporter2 {
public:
	virtual bool ImportAll(AssetDatabase2_BinaryImporterHandle& databaseHandle) = 0; // return false on error and true otherwise (even if no assets are imported)
};

class SerializationInfoStorage {
public:
	//TODO name to importer
	//TODO overriding existing importer checks
	void Register(std::string typeName, std::shared_ptr<TextAssetImporter> importer) {
		textImporters[typeName] = importer;
	}
	void Register(std::string typeName, std::shared_ptr<AssetImporter> importer) {
		binaryImporters[typeName] = importer;
	}
	void Register(std::string typeName, std::shared_ptr<AssetImporter2> importer) {
		binaryImporters2[typeName] = importer;
	}

	void UnregisterText(std::string typeName) {
		textImporters[typeName] = nullptr;
	}
	void UnregisterBinary(std::string typeName) {
		binaryImporters[typeName] = nullptr;
	}
	void UnregisterBinary2(std::string typeName) {
		binaryImporters2[typeName] = nullptr;
	}

	void Register(const SerializationInfoStorage& otherStorage) {
		for (auto& i : otherStorage.textImporters) {
			Register(i.first, i.second);
		}
		for (auto& i : otherStorage.binaryImporters) {
			Register(i.first, i.second);
		}
		for (auto& i : otherStorage.binaryImporters2) {
			Register(i.first, i.second);
		}
	}

	void Unregister(const SerializationInfoStorage& otherStorage) {
		for (auto& i : otherStorage.textImporters) {
			UnregisterText(i.first);
		}
		for (auto& i : otherStorage.binaryImporters) {
			UnregisterBinary(i.first);
		}
		for (auto& i : otherStorage.binaryImporters2) {
			UnregisterBinary2(i.first);
		}
	}

	std::shared_ptr<TextAssetImporter>& GetTextImporter(const std::string typeName) {
		return textImporters[typeName];
	}
	std::shared_ptr<AssetImporter>& GetBinaryImporter(const std::string typeName) {
		return binaryImporters[typeName];
	}
	std::shared_ptr<AssetImporter2>& GetBinaryImporter2(const std::string typeName) {
		return binaryImporters2[typeName];
	}

private:
	std::unordered_map<std::string, std::shared_ptr<TextAssetImporter>> textImporters;
	std::unordered_map<std::string, std::shared_ptr<AssetImporter>> binaryImporters;
	std::unordered_map<std::string, std::shared_ptr<AssetImporter2>> binaryImporters2;
};

SerializationInfoStorage& GetSerialiationInfoStorage();

#define DECLARE_SERIALATION_INFO_STORAGE() \
SerializationInfoStorage& GetSerialiationInfoStorage(){\
	static SerializationInfoStorage storage;\
	return storage;\
}

template <typename ImporterType>
class TextAssetImporterRegistrator {
public:
	TextAssetImporterRegistrator(std::string typeName) {
		GetSerialiationInfoStorage().Register(typeName, std::make_shared<ImporterType>());
	}
};

template <typename ImporterType>
class BinaryAssetImporterRegistrator {
public:
	BinaryAssetImporterRegistrator(std::string typeName) {
		GetSerialiationInfoStorage().Register(typeName, std::make_shared<ImporterType>());
	}
};

template <typename ImporterType>
class BinaryAssetImporterRegistrator2 {
public:
	BinaryAssetImporterRegistrator2(std::string typeName) {
		GetSerialiationInfoStorage().Register(typeName, std::make_shared<ImporterType>());
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