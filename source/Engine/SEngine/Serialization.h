#pragma once

#include <string>
#include <memory>
#include "Reflect.h"

class TextAssetImporter;
class AssetImporter;
class AssetImporter;

class AssetDatabase;

class AssetDatabase_BinaryImporterHandle;
class AssetDatabase_TextImporterHandle;

class TextAssetImporter {
public:
	virtual std::shared_ptr<Object> Import (AssetDatabase_TextImporterHandle& databaseHandle) = 0; // return false on error and true otherwise (even if no assets are imported)
};

class AssetImporter {
public:
	virtual bool ImportAll(AssetDatabase_BinaryImporterHandle& databaseHandle) = 0; // return false on error and true otherwise (even if no assets are imported)
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

	void UnregisterText(std::string typeName) {
		textImporters[typeName] = nullptr;
	}
	void UnregisterBinary(std::string typeName) {
		binaryImporters[typeName] = nullptr;
	}

	void Register(const SerializationInfoStorage& otherStorage) {
		for (auto& i : otherStorage.textImporters) {
			Register(i.first, i.second);
		}
		for (auto& i : otherStorage.binaryImporters) {
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
	}

	std::shared_ptr<TextAssetImporter>& GetTextImporter(const std::string typeName) {
		return textImporters[typeName];
	}
	std::shared_ptr<AssetImporter>& GetBinaryImporter(const std::string typeName) {
		return binaryImporters[typeName];
	}

private:
	std::unordered_map<std::string, std::shared_ptr<TextAssetImporter>> textImporters;
	std::unordered_map<std::string, std::shared_ptr<AssetImporter>> binaryImporters;
};

class SystemRegistratorBase;
class GameLibraryStaticStorage {
public:
	SerializationInfoStorage serializationInfoStorage;
	std::vector<SystemRegistratorBase*> systemRegistrators;//TODO shared ptrs
	static GameLibraryStaticStorage& Get();
};

inline SerializationInfoStorage& GetSerialiationInfoStorage() {
	return GameLibraryStaticStorage::Get().serializationInfoStorage;
}

template <typename ImporterType>
class TextAssetImporterRegistrator {
public:
	TextAssetImporterRegistrator(std::string typeName) {
		GetSerialiationInfoStorage().Register(typeName, std::make_shared<ImporterType>());
	}
};

template <typename ImporterType>
class TextAssetImporterRegistrator2 {
public:
	TextAssetImporterRegistrator2(std::string typeName) {
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
	std::shared_ptr<Object> Import(AssetDatabase_TextImporterHandle& handle) override {
		std::unique_ptr<Type> asset = std::make_unique<Type>();
		auto context = SerializationContext(handle.yaml);
		context.databaseHandle = &handle;
		Deserialize(context, *(asset.get()));
		return std::shared_ptr<Object>(std::move(asset));
	}
};

#define DECLARE_BINARY_ASSET(className, importerClassName) \
static BinaryAssetImporterRegistrator<##importerClassName> BinaryAssetImporterRegistrator_##className{#className};

#define DECLARE_TEXT_ASSET(className) \
static TextAssetImporterRegistrator<SerializedObjectImporter<##className>> TextAssetImporterRegistrator_##className{#className};