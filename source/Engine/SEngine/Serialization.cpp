#include "Serialization.h"
#include "ryml.hpp"
//TODO name to importer
//TODO overriding existing importer checks

SerializationContext SerializedObjectImporterBase::CreateContext(AssetDatabase_TextImporterHandle& handle) {
	return SerializationContext(handle.yaml);
}

//TODO name to importer
//TODO overriding existing importer checks

void SerializationInfoStorage::Register(ReflectedTypeBase* type, std::shared_ptr<TextAssetImporter> importer) {
	types[type->GetName()] = type;
	allTypes.push_back(type);
	textImporters[type->GetName()] = importer;
}

void SerializationInfoStorage::Register(ReflectedTypeBase* type, std::shared_ptr<AssetImporter> importer) {
	types[type->GetName()] = type;
	allTypes.push_back(type);
	binaryImporters[type->GetName()] = importer;
}

void SerializationInfoStorage::UnregisterText(std::string typeName) {
	types.erase(typeName);
	for (int i = 0; i < allTypes.size(); i++) {
		if (allTypes[i]->GetName() == typeName) {
			allTypes.erase(allTypes.begin() + i);
		}
	}
	textImporters.erase(typeName);
}

void SerializationInfoStorage::UnregisterBinary(std::string typeName) {
	types.erase(typeName);
	for (int i = 0; i < allTypes.size(); i++) {
		if (allTypes[i]->GetName() == typeName) {
			allTypes.erase(allTypes.begin() + i);
		}
	}
	binaryImporters.erase(typeName);
}

void SerializationInfoStorage::Register(const SerializationInfoStorage& otherStorage) {
	for (auto& i : otherStorage.textImporters) {
		Register(otherStorage.types.at(i.first), i.second);
	}
	for (auto& i : otherStorage.binaryImporters) {
		Register(otherStorage.types.at(i.first), i.second);
	}
}

void SerializationInfoStorage::Unregister(const SerializationInfoStorage& otherStorage) {
	for (auto& i : otherStorage.textImporters) {
		UnregisterText(i.first);
	}
	for (auto& i : otherStorage.binaryImporters) {
		UnregisterBinary(i.first);
	}
}
