#include "Serialization.h"
#include "ryml.hpp"
//TODO name to importer
//TODO overriding existing importer checks

void SerializationInfoStorage::Register(std::string typeName, std::shared_ptr<TextAssetImporter> importer) {
	textImporters[typeName] = importer;
}

void SerializationInfoStorage::Register(std::string typeName, std::shared_ptr<AssetImporter> importer) {
	binaryImporters[typeName] = importer;
}

void SerializationInfoStorage::UnregisterText(std::string typeName) {
	textImporters[typeName] = nullptr;
}

void SerializationInfoStorage::UnregisterBinary(std::string typeName) {
	binaryImporters[typeName] = nullptr;
}

void SerializationInfoStorage::Register(const SerializationInfoStorage& otherStorage) {
	for (auto& i : otherStorage.textImporters) {
		Register(i.first, i.second);
	}
	for (auto& i : otherStorage.binaryImporters) {
		Register(i.first, i.second);
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

std::shared_ptr<TextAssetImporter>& SerializationInfoStorage::GetTextImporter(const std::string typeName) {
	return textImporters[typeName];
}

std::shared_ptr<AssetImporter>& SerializationInfoStorage::GetBinaryImporter(const std::string typeName) {
	return binaryImporters[typeName];
}


SerializationContext SerializedObjectImporterBase::CreateContext(AssetDatabase_TextImporterHandle& handle) { 
	return SerializationContext(handle.yaml);
}