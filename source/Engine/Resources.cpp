#include "Common.h"

#include "Resources.h"
#include <memory>

std::unordered_map<std::string, std::unique_ptr<AssetImporter>>& AssetDatabase::GetAssetImporters() {
	static std::unordered_map<std::string, std::unique_ptr<AssetImporter>> assetImporters{};
	return assetImporters;
}

std::unordered_map<std::string, std::unique_ptr<TextAssetImporter>>& AssetDatabase::GetTextAssetImporters() {
	static std::unordered_map<std::string, std::unique_ptr<TextAssetImporter>> textAssetImporters{};
	return textAssetImporters;
}
AssetDatabase* AssetDatabase::mainDatabase = nullptr;
AssetDatabase* AssetDatabase::Get() {
	return AssetDatabase::mainDatabase;
}

//TODO templated
std::shared_ptr<Object> AssetDatabase::GetLoaded(std::string path) {
	PathDescriptor descriptor{ path };
	auto& it = assets.find(descriptor.assetPath);
	if (it != assets.end()) {
		//TODO naming maaan
		//TODO check type actualy
		std::shared_ptr<Object> bestVariant = nullptr;
		for (auto itIt : it->second) {
			if (itIt.first.assetId == descriptor.assetId) {
				bestVariant = itIt.second;
				break;
			}
			if (descriptor.assetId == "" && bestVariant == nullptr) {
				bestVariant = itIt.second;
			}
		}
		return bestVariant;
	}
	else {
		return nullptr;
	}
}

void AssetDatabase::AddAsset(std::shared_ptr<Object> asset, std::string path, std::string id) {
	std::string fullPath = path;
	if (id.size() > 0) {
		//TODO constexpr char
		fullPath += "$" + id;
	}
	assets[path].push_back({ PathDescriptor(fullPath), asset });

}

std::shared_ptr<Object> AssetDatabase::LoadByPath(std::string path) {
	auto desc = PathDescriptor{ path };
	auto loaded = GetLoaded(path);

	if (std::find(requestedAssetsToLoad.begin(), requestedAssetsToLoad.end(), desc.assetPath) == requestedAssetsToLoad.end()) {
		requestedAssetsToLoad.push_back(desc.assetPath);
	}
	LoadNextWhileNotEmpty();

	return GetLoaded(path);
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

void AssetDatabase::RegisterBinaryAssetImporter(std::string assetType, std::unique_ptr<AssetImporter>&& importer) {
	GetAssetImporters()[assetType] = std::move(importer);
}

void AssetDatabase::RegisterTextAssetImporter(std::string assetType, std::unique_ptr<TextAssetImporter>&& importer) {
	GetTextAssetImporters()[assetType] = std::move(importer);
}

void AssetDatabase::LoadAllAtPath(std::string path)
{
	currentAssetLoadingPath = path;
	std::string ext = GetFileExtension(path);
	if (ext == "asset") {
		auto textAsset = LoadTextAsset(path);
		if (!textAsset) {
			currentAssetLoadingPath = "";//TODO scope exis shit
			return;
		}
		//TODO return invalid asset ?
		if (!textAsset->GetYamlNode().IsDefined()) {
			currentAssetLoadingPath = "";//TODO scope exis shit
			return;
		}

		for (const auto& kv : textAsset->GetYamlNode()) {
			std::string type = kv.first.as<std::string>();
			auto node = kv.second;

			auto& importer = GetTextAssetImporters()[type];
			if (!importer) {
				continue;
			}

			auto asset = importer->Import(*this, node);

			std::string id = type;
			
			if (asset != nullptr) {
				AddAsset(asset, path, id);
			}
		}
	}
	else if (ext == "fbx" || ext=="glb") {
		std::string type = "Mesh";
		auto& importer = GetAssetImporters()[type];
		if (importer) {
			importer->Import(*this, path);
		}
	}
	currentAssetLoadingPath = "";
}
const std::string AssetDatabase::assetsFolderPrefix = "assets\\";

void AssetDatabase::LoadNextWhileNotEmpty() {
	for (int i = 0; i < requestedAssetsToLoad.size(); i++) {
		if (assets.find(requestedAssetsToLoad[i]) == assets.end()) {
			LoadAllAtPath(requestedAssetsToLoad[i]);
		}
	}
	requestedAssetsToLoad.clear();
	for (auto it : requestedObjectPtrs) {
		auto object = GetLoaded(it.second);
		*(std::shared_ptr<Object>*)(it.first) = object;
	}
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

void AssetDatabase::RequestObjectPtr(void* dest, std::string objectPath) {
	auto path = objectPath;
	if (path.size() > 0 && path[0] == '$') {
		path = currentAssetLoadingPath + path;
	}
	auto desc = PathDescriptor(path);
	if (std::find(requestedAssetsToLoad.begin(), requestedAssetsToLoad.end(), desc.assetPath) == requestedAssetsToLoad.end()) {
		requestedAssetsToLoad.push_back(desc.assetPath);
	}

	requestedObjectPtrs[dest] = path;
}

AssetDatabase::PathDescriptor::PathDescriptor(std::string path) {
	auto lastIdChar = path.find_last_of('$');
	if (lastIdChar == -1) {
		this->assetId = "";
		this->assetPath = path;
	}
	else {
		this->assetId = path.substr(lastIdChar + 1, path.length() - lastIdChar - 1);
		this->assetPath = path.substr(0, lastIdChar);
	}
}
