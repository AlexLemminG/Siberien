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

class SE_CPP_API SerializationInfoStorage {
public:
	//TODO name to importer
	//TODO overriding existing importer checks
	void Register(ReflectedTypeBase* type, std::shared_ptr<TextAssetImporter> importer);
	void Register(ReflectedTypeBase* type, std::shared_ptr<AssetImporter> importer);

	void UnregisterText(std::string typeName);
	void UnregisterBinary(std::string typeName);

	void Register(const SerializationInfoStorage& otherStorage);

	void Unregister(const SerializationInfoStorage& otherStorage);
	const std::vector<ReflectedTypeBase*>& GetAllTypes()const { return allTypes; }
	std::shared_ptr<TextAssetImporter>& GetTextImporter(const std::string typeName) {
		return textImporters[typeName];
	}
	std::shared_ptr<AssetImporter>& GetBinaryImporter(const std::string typeName) {
		return binaryImporters[typeName];
	}

private:
	std::unordered_map<std::string, ReflectedTypeBase*> types;
	std::vector<ReflectedTypeBase*> allTypes;
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
	TextAssetImporterRegistrator(ReflectedTypeBase* type) {
		this->type = type;
		GameLibraryStaticStorage::Get().importerRegistrators.push_back(this);//TODO remove ?
	}

	virtual void Register() override {
		GetSerialiationInfoStorage().Register(type, std::make_shared<ImporterType>());
	}

	ReflectedTypeBase* type;
};


template <typename ImporterType>
class BinaryAssetImporterRegistrator : public ImporterRegistratorBase {
public:
	BinaryAssetImporterRegistrator(ReflectedTypeBase* type) {
		this->type = type;
		GameLibraryStaticStorage::Get().importerRegistrators.push_back(this);//TODO remove ?
	}

	virtual void Register() override {
		GetSerialiationInfoStorage().Register(type, std::make_shared<ImporterType>());
	}

	ReflectedTypeBase* type;
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
static BinaryAssetImporterRegistrator<##importerClassName> BinaryAssetImporterRegistrator_##className{GetReflectedType<className>()};

//TODO maybe DEFINE_... or REGISTER_...?
#define DECLARE_TEXT_ASSET(className) \
static TextAssetImporterRegistrator<SerializedObjectImporter<##className>> TextAssetImporterRegistrator_##className{GetReflectedType<className>()};