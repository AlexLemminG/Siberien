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
	virtual std::shared_ptr<Object> Import(AssetDatabase_TextImporterHandle& databaseHandle) = 0; // return false on error and true otherwise (even if no assets are imported)
};

class AssetImporter {
public:
	virtual bool ImportAll(AssetDatabase_BinaryImporterHandle& databaseHandle) = 0; // return false on error and true otherwise (even if no assets are imported)
};

class SerializationInfoStorage {
public:
	//TODO name to importer
	//TODO overriding existing importer checks
	inline void Register(std::string typeName, std::shared_ptr<TextAssetImporter> importer) {
		textImporters[typeName] = importer;
	}
	inline void Register(std::string typeName, std::shared_ptr<AssetImporter> importer) {
		binaryImporters[typeName] = importer;
	}

	inline void UnregisterText(std::string typeName) {
		textImporters[typeName] = nullptr;
	}
	inline void UnregisterBinary(std::string typeName) {
		binaryImporters[typeName] = nullptr;
	}

	inline void Register(const SerializationInfoStorage& otherStorage) {
		for (auto& i : otherStorage.textImporters) {
			Register(i.first, i.second);
		}
		for (auto& i : otherStorage.binaryImporters) {
			Register(i.first, i.second);
		}
	}

	inline void Unregister(const SerializationInfoStorage& otherStorage) {
		for (auto& i : otherStorage.textImporters) {
			UnregisterText(i.first);
		}
		for (auto& i : otherStorage.binaryImporters) {
			UnregisterBinary(i.first);
		}
	}

	inline std::shared_ptr<TextAssetImporter>& GetTextImporter(const std::string typeName) {
		return textImporters[typeName];
	}
	inline std::shared_ptr<AssetImporter>& GetBinaryImporter(const std::string typeName) {
		return binaryImporters[typeName];
	}

private:
	std::unordered_map<std::string, std::shared_ptr<TextAssetImporter>> textImporters;
	std::unordered_map<std::string, std::shared_ptr<AssetImporter>> binaryImporters;
};

class ImporterRegistratorBase {
public:
	virtual void Register() = 0;
};
class SystemRegistratorBase;
class GameLibraryStaticStorage {
public:
	SerializationInfoStorage serializationInfoStorage;
	std::vector<SystemRegistratorBase*> systemRegistrators;//TODO shared ptrs
	std::vector<ImporterRegistratorBase*> importerRegistrators;//TODO shared ptrs
	static GameLibraryStaticStorage& Get();
};

inline SerializationInfoStorage& GetSerialiationInfoStorage() {
	return GameLibraryStaticStorage::Get().serializationInfoStorage;
}


template <typename ImporterType>
class TextAssetImporterRegistrator : public ImporterRegistratorBase {
public:
	TextAssetImporterRegistrator(std::string typeName) {
		this->typeName = typeName;
		GameLibraryStaticStorage::Get().importerRegistrators.push_back(this);//TODO remove ?
	}

	virtual void Register() override {
		GetSerialiationInfoStorage().Register(typeName, std::make_shared<ImporterType>());
	}

	std::string typeName;
};


template <typename ImporterType>
class BinaryAssetImporterRegistrator : public ImporterRegistratorBase {
public:
	BinaryAssetImporterRegistrator(std::string typeName) {
		this->typeName = typeName;
		GameLibraryStaticStorage::Get().importerRegistrators.push_back(this);//TODO remove ?
	}

	virtual void Register() override {
		GetSerialiationInfoStorage().Register(typeName, std::make_shared<ImporterType>());
	}

	std::string typeName;
};

class SE_CPP_API SerializedObjectImporterBase : public TextAssetImporter {
protected:
	SerializationContext CreateContext(AssetDatabase_TextImporterHandle& handle);
};

template <typename Type>
class SerializedObjectImporter : public SerializedObjectImporterBase {
	std::shared_ptr<Object> Import(AssetDatabase_TextImporterHandle& handle) override {
		std::unique_ptr<Type> asset = std::make_unique<Type>();
		auto context = CreateContext(handle);
		context.databaseHandle = &handle;
		Deserialize(context, *(asset.get()));
		return std::shared_ptr<Object>(std::move(asset));
	}
private:
};

#define DECLARE_BINARY_ASSET(className, importerClassName) \
static BinaryAssetImporterRegistrator<##importerClassName> BinaryAssetImporterRegistrator_##className{#className};

//TODO maybe DEFINE_... or REGISTER_...?
#define DECLARE_TEXT_ASSET(className) \
static TextAssetImporterRegistrator<SerializedObjectImporter<##className>> TextAssetImporterRegistrator_##className{#className};