#include "Common.h"

#include "Resources.h"
#include <memory>

std::unordered_map<std::string, std::unique_ptr<AssetImporter>>& AssetDatabase::GetAssetImporters() {
	static std::unordered_map<std::string, std::unique_ptr<AssetImporter>> assetImporters{};
	return assetImporters;
}

std::shared_ptr<Asset> AssetDatabase::LoadByPath(std::string path) {
	auto it = assets.find(path);
	if (it != assets.end()) {
		return it->second;
	}

	if (GetFileExtension(path) == "asset") {
		auto textAsset = LoadTextAsset(path);
		if (!textAsset) {
			return nullptr;
		}
		//TODO return invalid asset ?
		if (!textAsset->GetYamlNode().IsDefined()) {
			return nullptr;
		}
		auto assetTypeNode = textAsset->GetYamlNode()["__type"];
		if (!assetTypeNode.IsDefined()) {
			return nullptr;
		}
		std::string type = assetTypeNode.as<std::string>();
		auto& importer = GetAssetImporters()[type];
		if (!importer) {
			return nullptr;
		}
		//TODO deduce typename
		auto asset = importer->Import(*this, path);

		if (asset != nullptr) {
			assets[path] = asset;
			return asset;
		}
		return nullptr;
	}
	return nullptr;
}

std::shared_ptr<TextAsset> AssetDatabase::LoadTextAsset(std::string path) {
	auto it = textAssets.find(path);

	if (it != textAssets.end()) {
		return std::static_pointer_cast<TextAsset>(it->second);
	}

	auto fullPath = assetsFolderPrefix + path;
	std::ifstream input(fullPath);
	if (!input) {
		//TODO
		//ASSERT(false);
		return nullptr;
	}

	AssetLoadingContext context;
	context.inputFile = &input;

	auto ext = GetFileExtension(fullPath);
	auto textAsset = std::make_shared<TextAsset>();
	if (textAsset->Load(context)) {
		textAssets[path] = textAsset;
		return textAsset;
	}
	return nullptr;
}

void AssetDatabase::RegisterImporter(std::string assetType, std::unique_ptr<AssetImporter>&& importer) {
	GetAssetImporters()[assetType] = std::move(importer);
}

std::string AssetDatabase::GetFileName(std::string path) {
	auto lastSlash = path.find_last_of('\\');
	if (lastSlash == -1) {
		return path;
	}
	else {
		return path.substr(lastSlash + 1, path.length() - lastSlash - 1);
	}
}

std::string AssetDatabase::GetFileExtension(std::string path) {
	auto name = GetFileName(path);
	if (name == "") {
		return "";
	}
	auto lastDot = path.find_last_of('.');
	if (lastDot == -1) {
		return "";
	}
	else {
		return path.substr(lastDot + 1, path.length() - lastDot - 1);
	}
}
