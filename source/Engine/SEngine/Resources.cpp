#include "Common.h"

#include "Resources.h"
#include <memory>
#include <windows.h>
#include "Serialization.h"
#include "SDL_filesystem.h"
#include <filesystem>
#include <locale>
#include <codecvt>



AssetDatabase* AssetDatabase::mainDatabase = nullptr;
AssetDatabase* AssetDatabase::Get() {
	return mainDatabase;
}

bool AssetDatabase::Init() {
	OPTICK_EVENT();
	ASSERT(mainDatabase == nullptr);
	mainDatabase = this;
	return true;
}

void AssetDatabase::Term() {
	OPTICK_EVENT();
	ASSERT(mainDatabase == this);
	mainDatabase = nullptr;
	UnloadAll();
}

void AssetDatabase::UnloadAll() {
	OPTICK_EVENT();
	onBeforeUnloaded.Invoke();
	assets.clear();
	objectPaths.clear();
}

std::string AssetDatabase::GetAssetUID(std::shared_ptr<Object> obj) {
	auto it = objectPaths.find(obj);
	if (it != objectPaths.end()) {
		return it->second;
	}
	else {
		return "";
	}
}

std::string AssetDatabase::GetAssetPath(std::shared_ptr<Object> obj) {
	return AssetDatabase::PathDescriptor(GetAssetUID(obj)).assetPath;
}

void AssetDatabase_TextImporterHandle::RequestObjectPtr(void* dest, const std::string& uid) {
	auto descriptor = AssetDatabase::PathDescriptor(uid);
	if (descriptor.assetPath.size() == 0) {
		descriptor.assetPath = database->currentAssetLoadingPath;
	}
	database->loadingQueue.push_back(AssetDatabase::LoadingRequest{ dest, descriptor });
}

std::shared_ptr<AssetImporter>& AssetDatabase::GetAssetImporter(const std::string& type) {
	return GetSerialiationInfoStorage().GetBinaryImporter(type);
}


std::shared_ptr<Object> AssetDatabase::GetLoaded(const PathDescriptor& descriptor, ReflectedTypeBase* type) {
	auto it = assets.find(descriptor.assetPath);
	if (it != assets.end()) {
		//TODO
		//return assets[descriptor.assetPath].Get(GetReflectedType<T>(), descriptor.assetPath);
		return it->second.Get(descriptor.assetId);
	}
	else {
		return nullptr;
	}
}


void AssetDatabase::ProcessLoadingQueue() {
	std::vector<std::shared_ptr<Object>> loadedObjects;
	while (!loadingQueue.empty()) {
		const auto request = loadingQueue[0];
		currentAssetLoadingPath = request.descriptor.assetPath;
		//TODO optimize traversing
		loadingQueue.erase(loadingQueue.begin());

		auto loaded = GetLoaded(request.descriptor, nullptr);
		if (loaded) {
			if (request.target) {
				*(std::shared_ptr<Object>*)(request.target) = loaded;
			}
			continue;
		}

		const auto& path = request.descriptor.assetPath;
		auto extention = GetFileExtension(path);
		//TODO get extensions list from asset importer
		if (extention == "png") {
			auto& importer = GetAssetImporter("Texture");
			if (importer) {
				importer->ImportAll(AssetDatabase_BinaryImporterHandle(this, path));
			}
		}
		else if (extention == "fbx" || extention == "glb" || extention == "blend") {
			auto& importer = GetAssetImporter("Mesh");
			if (importer) {
				importer->ImportAll(AssetDatabase_BinaryImporterHandle(this, path));
			}
		}
		else if (extention == "fs" || extention == "vs") {
			auto& importer = GetAssetImporter("Shader");
			if (importer) {
				importer->ImportAll(AssetDatabase_BinaryImporterHandle(this, path));
			}
		}
		else if (extention == "asset") {
			YAML::Node node;
			std::ifstream input(assetsRootFolder + path);
			if (input) {
				//TODO
				//LogError("Failed to load '%s': file not found", fullPath.c_str());

				node = YAML::Load(input);
				if (node.IsDefined()) {
					for (const auto& kv : node) {
						PathDescriptor descriptor(kv.first.as<std::string>());
						std::string type = descriptor.assetPath;
						std::string id = descriptor.assetId.size() > 0 ? descriptor.assetId : descriptor.assetPath;

						auto node = kv.second;

						auto& importer = GetSerialiationInfoStorage().GetTextImporter(type);
						if (importer) {
							AssetDatabase_TextImporterHandle handle{ this, kv.second };
							auto object = importer->Import(handle);//TODO rename to deserializer->Deserialize or something

							if (object != nullptr) {
								loadedObjects.push_back(object);
								auto& asset = assets[path];
								asset.Add(id, object);
								objectPaths[object] = AssetDatabase::PathDescriptor(path, id).ToFullPath();
							}
							else {
								//TODO LogError("failed to import '%s' from '%s'", type.c_str(), currentAssetLoadingPath.c_str());
							}
						}
						else {
							//TODO error
						}
					}
				}
				else {
					node = YAML::Node();
					//TODO error
				}
			}
			else {
				//TORO error
				node = YAML::Node();
			}
		}
		else {
			//TODO error
		}


		loaded = GetLoaded(request.descriptor, nullptr);
		if (loaded) {
			if (request.target) {
				*(std::shared_ptr<Object>*)(request.target) = loaded;
			}
			continue;
		}
		else {
			if (request.target) {
				*(std::shared_ptr<Object>*)(request.target) = nullptr;
			}
		}
		currentAssetLoadingPath = "";
	}

	for (auto o : loadedObjects) {
		SerializationContext c{ YAML::Node() };//TODO HAAAKING
		o->OnAfterDeserializeCallback(c);
	}
}


void AssetDatabase::LoadAsset(const std::string& path) {
	if (assets.find(path) != assets.end()) {
		return;//TODO dont call LoadAsset in the first place
	}

	loadingQueue.push_back(AssetDatabase::LoadingRequest{ nullptr, PathDescriptor{path} });
	ProcessLoadingQueue();
}


std::shared_ptr<Object> AssetDatabase::DeserializeFromYAMLInternal(const YAML::Node& node) {
	std::string path = "***temp_asset***";
	std::vector<std::shared_ptr<Object>> loadedObjects;
	currentAssetLoadingPath = path;
	//TODO less code duplication
	//TODO less hacking
	if (node.IsDefined()) {
		for (const auto& kv : node) {
			PathDescriptor descriptor(kv.first.as<std::string>());
			std::string type = descriptor.assetPath;
			std::string id = descriptor.assetId.size() > 0 ? descriptor.assetId : descriptor.assetPath;

			auto node = kv.second;

			auto& importer = GetSerialiationInfoStorage().GetTextImporter(type);
			if (importer) {
				AssetDatabase_TextImporterHandle handle{ this, kv.second };
				auto object = importer->Import(handle);//TODO rename to deserializer->Deserialize or something

				if (object != nullptr) {
					loadedObjects.push_back(object);
					auto& asset = assets[path];
					asset.Add(id, object);
					objectPaths[object] = AssetDatabase::PathDescriptor(path, id).ToFullPath();//TODO delete
				}
				else {
					//TODO LogError("failed to import '%s' from '%s'", type.c_str(), currentAssetLoadingPath.c_str());
				}
			}
			else {
				//TODO error
			}
		}
	}

	ProcessLoadingQueue();

	for (auto o : loadedObjects) {
		SerializationContext c{ YAML::Node() };//TODO HAAAKING
		o->OnAfterDeserializeCallback(c);
		objectPaths.erase(o);
	}
	currentAssetLoadingPath = "";
	auto main = assets[path].GetMain();
	assets[path] = Asset();
	return main;
}

void AssetDatabase_BinaryImporterHandle::GetLastModificationTime(long& assetModificationTime, long& metaModificationTime) {

	struct stat result;
	auto fullPathAsset = this->assetPath;

	if (stat(fullPathAsset.c_str(), &result) == 0)
	{
		assetModificationTime = result.st_mtime;
	}
	else {
		assetModificationTime = 0;
	}

	auto fullPathMeta = this->assetPath + ".meta";

	if (stat(fullPathMeta.c_str(), &result) == 0)
	{
		metaModificationTime = result.st_mtime;
	}
	else {
		metaModificationTime = 0;
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


void AssetDatabase_BinaryImporterHandle::AddAssetToLoaded(std::string id, std::shared_ptr<Object> object)
{
	auto& asset = database->assets[assetPath];
	asset.Add(id, object);
	database->objectPaths[object] = AssetDatabase::PathDescriptor(assetPath, id).ToFullPath();
}

bool AssetDatabase_BinaryImporterHandle::ReadAssetAsBinary(std::vector<uint8_t>& buffer)
{
	return ReadBinary(GetAssetPath(), buffer);
}

bool AssetDatabase_BinaryImporterHandle::ReadAssetAsYAML(YAML::Node& node) {
	return ReadYAML(GetAssetPath(), node);
}

bool AssetDatabase_BinaryImporterHandle::ReadMeta(YAML::Node& node) {
	const auto fullPath = database->assetsRootFolder + assetPath + ".meta";
	return ReadYAML(fullPath, node);
}

std::string AssetDatabase_BinaryImporterHandle::GetAssetPath() { return database->assetsRootFolder + assetPath; }
std::string AssetDatabase_BinaryImporterHandle::GetFileExtension() { return database->GetFileExtension(assetPath); }
void AssetDatabase_BinaryImporterHandle::EnsureForderForLibraryFileExists(std::string id) {
	auto fullPath = GetLibraryPathFromId(id);
	auto firstFolder = fullPath.find_first_of("\\");
	for (int i = 0; i < fullPath.size(); i++) {
		if (fullPath[i] == '\\') {
			auto subpath = fullPath.substr(0, i);
			CreateDirectoryA(subpath.c_str(), NULL);
		}
	}
}

std::string AssetDatabase_BinaryImporterHandle::GetLibraryPathFromId(const std::string& id) {
	return database->libraryRootFolder + assetPath + "\\" + id;
}

void AssetDatabase_BinaryImporterHandle::WriteToLibraryFile(const std::string& id, const YAML::Node& node) {
	//TODO checks and make sure folder exists
	const auto fullPath = GetLibraryPathFromId(id);
	std::ofstream fout(fullPath);
	fout << node;
}

void AssetDatabase_BinaryImporterHandle::WriteToLibraryFile(const std::string& id, std::vector<uint8_t>& buffer) {
	//TODO checks and make sure folder exists
	const auto fullPath = GetLibraryPathFromId(id);
	std::ofstream fout(fullPath, std::ios::binary);
	if (buffer.size() > 0) {
		fout.write((char*)&buffer[0], buffer.size());
	}
}

bool AssetDatabase_BinaryImporterHandle::ReadFromLibraryFile(const std::string& id, YAML::Node& node)
{
	const auto fullPath = GetLibraryPathFromId(id);

	std::ifstream input(fullPath);
	if (!input) {
		LogError("Failed to load '%s': file not found", assetPath.c_str());
		node = YAML::Node(YAML::NodeType::Undefined);
		return false;
	}
	node = YAML::Load(input);
	if (!node.IsDefined()) {
		node = YAML::Node(YAML::NodeType::Undefined);
		return false;
	}
	else {
		return true;
	}
}

bool AssetDatabase_BinaryImporterHandle::ReadFromLibraryFile(const std::string& id, std::vector<uint8_t>& buffer)
{
	return ReadBinary(GetLibraryPathFromId(id), buffer);
}

std::string AssetDatabase_BinaryImporterHandle::GetToolPath(std::string toolName) { return "tools\\" + toolName; }

bool AssetDatabase_BinaryImporterHandle::ReadBinary(const std::string& fullPath, std::vector<uint8_t>& buffer)
{
	std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

	if (!file) {
		//TODO different message and Log level
		LogError("Failed to load binary asset '%s': file not found", fullPath.c_str());
		return false;
	}
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	buffer.resize(size);
	if (file.read((char*)buffer.data(), size))
	{
		return true;
	}
	else {
		buffer.clear();
		//TODO different message and Log level
		LogError("Failed to load binary asset '%s': failed to read file", fullPath.c_str());
		ASSERT(false);
		return false;
	}
}


bool AssetDatabase_BinaryImporterHandle::ReadYAML(const std::string& fullPath, YAML::Node& node)
{
	std::ifstream input(fullPath);
	if (!input) {
		//TODO
		//LogError("Failed to load '%s': file not found", fullPath.c_str());
		node = YAML::Node();
		return false;
	}
	node = YAML::Load(input);
	if (!node.IsDefined()) {
		node = YAML::Node();
		return false;
	}
	else {
		return true;
	}
}

std::string AssetDatabase::PathDescriptor::ToFullPath() {
	return assetId.size() > 0 ? FormatString("%s$%s", assetPath.c_str(), assetId.c_str()) : assetPath;
}
