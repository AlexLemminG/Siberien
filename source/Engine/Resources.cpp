#include "Common.h"

#include "Resources.h"
#include <memory>
#include <windows.h>

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
		//LogError("Asset with path '%s' not loaded", path.c_str());
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

std::string AssetDatabase::GetLibraryPath(std::string assetPath) {
	return libraryAssetsFolderPrefix + assetPath;
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

//TODO remove doublecoding shit
std::shared_ptr<TextAsset> AssetDatabase::LoadTextAsset(std::string path) {
	auto fullPath = assetsFolderPrefix + path;
	std::ifstream input(fullPath);
	if (!input) {
		LogError("Failed to load '%s': file not found", path.c_str());
		return nullptr;
	}

	AssetLoadingContext context;
	context.inputFile = &input;

	auto ext = GetFileExtension(fullPath);
	auto textAsset = std::make_shared<TextAsset>();
	if (textAsset->Load(context)) {
		return textAsset;
	}
	LogError("Failed to load '%s': failed to import", path.c_str());
	return nullptr;
}


std::shared_ptr<TextAsset> AssetDatabase::LoadTextAssetFromLibrary(std::string path) {
	auto fullPath = libraryAssetsFolderPrefix + path;
	std::ifstream input(fullPath);
	if (!input) {
		LogError("Failed to load '%s' from library: file not found", path.c_str());
		return nullptr;
	}

	AssetLoadingContext context;
	context.inputFile = &input;

	auto ext = GetFileExtension(fullPath);
	auto textAsset = std::make_shared<TextAsset>();
	if (textAsset->Load(context)) {
		return textAsset;
	}
	LogError("Failed to load '%s' from library: import error", path.c_str());
	return nullptr;
}

std::shared_ptr<BinaryAsset> AssetDatabase::LoadBinaryAsset(std::string path) {
	auto fullPath = assetsFolderPrefix + path;

	std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

	if (!file) {
		auto fullPath = libraryAssetsFolderPrefix + path;
		file.open(fullPath, std::ios::binary | std::ios::ate);
	}
	if (!file) {
		LogError("Failed to load binary asset '%s': file not found", path.c_str());
		return nullptr;
	}
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		return std::make_shared<BinaryAsset>(std::move(buffer));
	}
	else {
		LogError("Failed to load binary asset '%s': failed to read file", path.c_str());
		ASSERT(false);
		return nullptr;
	}
}
//TODO remove doublecoding shit
std::shared_ptr<BinaryAsset> AssetDatabase::LoadBinaryAssetFromLibrary(std::string path) {
	auto fullPath = libraryAssetsFolderPrefix + path;

	std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

	if (!file) {
		auto fullPath = libraryAssetsFolderPrefix + path;
		file.open(fullPath, std::ios::binary | std::ios::ate);
	}
	if (!file) {
		LogError("Failed to load binary asset '%s' from library: file not found", path.c_str());
		return nullptr;
	}
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		return std::make_shared<BinaryAsset>(std::move(buffer));
	}
	else {
		LogError("Failed to load binary asset '%s' from library: failed to read file", path.c_str());
		ASSERT(false);
		return nullptr;
	}
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
			PathDescriptor descriptor(kv.first.as<std::string>());
			std::string type = descriptor.assetPath;
			std::string id = descriptor.assetId.size() > 0 ? descriptor.assetId : descriptor.assetPath;

			auto node = kv.second;

			auto& importer = GetTextAssetImporters()[type];
			if (!importer) {
				LogError("Unknown asset type '%s' in '%s'", type.c_str(), currentAssetLoadingPath.c_str());
				continue;
			}

			auto asset = importer->Import(*this, node);

			if (asset != nullptr) {
				AddAsset(asset, path, id);
			}
			else {
				LogError("failed to import '%s' from '%s'", type.c_str(), currentAssetLoadingPath.c_str());

			}
		}
	}
	else if (ext == "fbx" || ext == "glb" || ext == "blend") {
		std::string type = "Mesh";
		auto& importer = GetAssetImporters()[type];
		if (importer) {
			importer->Import(*this, path);
		}
		else {
			ASSERT(false);
		}
	}
	else {
		LogError("unknown extension '%s' at file '%s'", ext.c_str(), currentAssetLoadingPath.c_str());
	}
	currentAssetLoadingPath = "";
}
const std::string AssetDatabase::assetsFolderPrefix = "assets\\";
const std::string AssetDatabase::libraryAssetsFolderPrefix = "library\\assets\\";

void AssetDatabase::LoadNextWhileNotEmpty() {
	for (int i = 0; i < requestedAssetsToLoad.size(); i++) {
		if (assets.find(requestedAssetsToLoad[i]) == assets.end()) {
			LoadAllAtPath(requestedAssetsToLoad[i]);
		}
	}
	requestedAssetsToLoad.clear();
	for (auto it : requestedObjectPtrs) {
		auto object = GetLoaded(it.second);
		if (!object) {
			LogError("Asset not found '%s'", it.second.c_str());
			*(std::shared_ptr<Object>*)(it.first) = nullptr;
		}
		else {
			//TODO check type
			*(std::shared_ptr<Object>*)(it.first) = object;
		}
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

long AssetDatabase::GetLastModificationTime(std::string path) {
	struct stat result;
	auto fullPath = assetsFolderPrefix + path;

	if (stat(fullPath.c_str(), &result) == 0)
	{
		auto mod_time = result.st_mtime;
		return mod_time;
	}
	else {
		return 0;
	}
}

void AssetDatabase::CreateFolders(std::string fullPath) {
	auto firstFolder = fullPath.find_first_of("\\");
	for (int i = 0; i < fullPath.size(); i++) {
		if (fullPath[i] == '\\') {
			auto subpath = fullPath.substr(0, i);
			CreateDirectoryA(subpath.c_str(), NULL);
		}
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
