

#include "Common.h"

#include "SMath.h"
#include "yaml-cpp/yaml.h"
#include "ryml.hpp"
#include "Resources.h"
#include <memory>
#include <windows.h>
#include "Serialization.h"
#include "SDL_filesystem.h"
#include <filesystem>
#include <locale>
#include <codecvt>


static std::unique_ptr<ryml::Tree> StreamToYAML(std::ifstream& input) {
	//TODO error checks

	std::vector<char> buffer;
	std::streamsize size = input.tellg();
	input.seekg(0, std::ios::beg);

	ResizeVectorNoInit(buffer, size);
	input.read((char*)buffer.data(), size);
	return std::make_unique<ryml::Tree>(ryml::parse(c4::csubstr(&buffer[0], buffer.size())));
}

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
	//TODO little bit hacky
	onAfterUnloaded.UnsubscribeAll();
	UnloadAll();
	onBeforeUnloaded.UnsubscribeAll();
}


//TODO currently only required for scene editing hack. consider removing

void AssetDatabase::Unload(const std::string& path) {
	auto it = assets.find(path);
	if (it == assets.end()) {
		return;
	}
	for (const auto& obj : it->second.objects) {
		objectPaths.erase(obj.obj);
	}
	assets.erase(path);
}

void AssetDatabase::UnloadAll() {
	OPTICK_EVENT();
	onBeforeUnloaded.Invoke();
	assets.clear();
	fileModificationDates.clear();
	objectPaths.clear();
	onAfterUnloaded.Invoke();
}

std::vector<std::string> AssetDatabase::GetAllAssetNames() {
	std::string path = assetsRootFolder;
	std::vector<std::string> result;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
		auto str = entry.path().string();
		result.push_back(str.substr(path.length(), str.length() - path.length()));
	}
	return result;
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

const std::shared_ptr<ryml::Tree> AssetDatabase::GetOriginalSerializedAsset(const std::string& assetPath) {
	return assets[assetPath].originalTree;
}

const ryml::NodeRef AssetDatabase::GetOriginalSerializedAsset(const std::shared_ptr<Object>& obj) {
	auto descriptor = AssetDatabase::PathDescriptor(GetAssetUID(obj));
	auto tree = assets[descriptor.assetPath].originalTree;
	if (tree == nullptr) {
		return ryml::NodeRef();
	}
	auto rootRef = tree->rootref();
	auto key = obj->GetType()->GetName();
	if (key != descriptor.assetId) {
		key += "$" + descriptor.assetId;
	}
	auto child = rootRef.find_child(ryml::csubstr(key.c_str(), key.size()));
	if (child.valid()) {
		return child;
	}
	if (key != descriptor.assetId) {
		return ryml::NodeRef();
		//TODO error
	}
	key += "$" + descriptor.assetId;//in case same id as type is explicitly specified
	child = rootRef.find_child(ryml::csubstr(key.c_str(), key.size()));
	if (child.valid()) {
		return child;
	}
	return ryml::NodeRef();//TODO error
}

std::string AssetDatabase::AddObjectToAsset(const std::string& path, const std::shared_ptr<Object>& object) {
	//TODO nullcheck
	auto& asset = assets[path];
	const std::string& typeName = object->GetType()->GetName();
	bool useTypeNameAsId = true;
	for (auto a : asset.objects) {
		if (a.id == typeName) {
			useTypeNameAsId = false;
			break;
		}
	}
	if (useTypeNameAsId) {
		AddObjectToAsset(path, typeName, object);
		return typeName;
	}

	int intId = 0;

	for (auto a : asset.objects) {
		char* pEnd = nullptr;
		int i = strtol(a.id.c_str(), &pEnd, 10);
		if (!*pEnd) {
			intId = Mathf::Max(intId, i + 1);
		}
	}
	std::string id = std::to_string(intId);
	AddObjectToAsset(path, id, object);
	return id;
}

std::string AssetDatabase::RemoveObjectFromAsset(const std::shared_ptr<Object>& object) {
	auto desc = AssetDatabase::PathDescriptor(GetAssetUID(object));
	if (!desc.assetId.empty()) {
		auto& asset = assets[desc.assetPath];
		asset.Remove(object);
	}
	objectPaths.erase(object);
	return desc.assetId;
}

void AssetDatabase::AddObjectToAsset(const std::string& path, const std::string& id, const std::shared_ptr<Object>& object) {
	//TODO nullcheck
	//TODO auto mark asset dirty in public method
	auto& asset = assets[path];
	asset.Add(id, object);//TODO assert does not already exist
	objectPaths[object] = AssetDatabase::PathDescriptor(path, id).ToFullPath();
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
		return it->second.Get(type, descriptor.assetId);
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
		if (loaded) {//TODO check if was trying to load before and failed as well
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
				importer->ImportAll(AssetDatabase_BinaryImporterHandle(this, path));//TODO mark path as failed on error
			}
		}
		else if (extention == "fbx" || extention == "glb" || extention == "blend") {
			auto& importer = GetAssetImporter("Mesh");
			if (importer) {
				importer->ImportAll(AssetDatabase_BinaryImporterHandle(this, path));//TODO mark path as failed on error
			}
		}
		else if (extention == "fs" || extention == "vs") {
			auto& importer = GetAssetImporter("Shader");
			if (importer) {
				importer->ImportAll(AssetDatabase_BinaryImporterHandle(this, path));//TODO mark path as failed on error
			}
		}
		else if (extention == "asset") {
			if (assets.find(path) == assets.end()) {
				auto fileName = assetsRootFolder + path;
				std::ifstream input(fileName, std::ios::binary | std::ios::ate);
				if (input) {
					//TODO
					//LogError("Failed to load '%s': file not found", fullPath.c_str());

					auto treePtr = std::shared_ptr(std::move(StreamToYAML(input)));
					ryml::Tree& tree = *treePtr;
					ryml::NodeRef node = tree;
					assets[path].originalTree = treePtr;
					if (!node.empty()) {
						for (const auto& kv : node) {
							auto key = kv.key();
							PathDescriptor descriptor{ std::string(key.str, key.len) };
							std::string type = descriptor.assetPath;
							std::string id = descriptor.assetId.size() > 0 ? descriptor.assetId : descriptor.assetPath;

							auto subnode = kv;

							auto& importer = GetSerialiationInfoStorage().GetTextImporter(type);
							if (importer) {
								AssetDatabase_TextImporterHandle handle{ this, subnode };
								auto object = importer->Import(handle);//TODO rename to deserializer->Deserialize or something

								if (object != nullptr) {
									loadedObjects.push_back(object);
									AddObjectToAsset(path, id, object);
								}
								else {
									//TODO LogError("failed to import '%s' from '%s'", type.c_str(), currentAssetLoadingPath.c_str());
								}
							}
							else {
								ASSERT(false);
								//TODO error
							}
						}
					}
					else {
						//TODO error
					}
				}
				else {
					//TORO error
				}
			}
			else {
				//TODO error
			}
		}
		else {
			//TODO error
		}


		loaded = GetLoaded(request.descriptor, nullptr);//PERF not the best way to check if asset was loaded
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
		SerializationContext c{ ryml::NodeRef() };//TODO HAAAKING
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


std::shared_ptr<Object> AssetDatabase::DeserializeFromYAMLInternal(const ryml::NodeRef& node) {
	//TODO optimize
	std::string path = "***temp_asset***";
	std::vector<std::shared_ptr<Object>> loadedObjects;
	currentAssetLoadingPath = path;
	//TODO less code duplication
	//TODO less hacking
	if (!node.empty()) {
		auto process = [&](const ryml::NodeRef& kv) {
			auto key = kv.key();
			PathDescriptor descriptor{ std::string(key.str, key.len) };
			std::string type = descriptor.assetPath;
			std::string id = descriptor.assetId.size() > 0 ? descriptor.assetId : descriptor.assetPath;//TODO why descriptor.assetPath as id ?

			auto node = kv;

			auto& importer = GetSerialiationInfoStorage().GetTextImporter(type);
			if (importer) {
				AssetDatabase_TextImporterHandle handle{ this, node };
				auto object = importer->Import(handle);//TODO rename to deserializer->Deserialize or something

				if (object != nullptr) {
					loadedObjects.push_back(object);
					AddObjectToAsset(path, id, object);
				}
				else {
					//TODO LogError("failed to import '%s' from '%s'", type.c_str(), currentAssetLoadingPath.c_str());
				}
			}
			else {
				//TODO error
			}
		};
		process(node);//WARN dangaras
		for (const auto& kv : node) {
			process(kv);
		}
	}

	ProcessLoadingQueue();

	for (auto o : loadedObjects) {
		SerializationContext c{ ryml::NodeRef() };//TODO HAAAKING
		o->OnAfterDeserializeCallback(c);
		objectPaths.erase(o);
	}
	currentAssetLoadingPath = "";
	auto main = assets[path].GetMain();
	assets[path] = Asset();
	return main;
}

uint64_t AssetDatabase::GetLastModificationTime(const std::string& assetPath) {
	auto it = fileModificationDates.find(assetPath);
	if (it != fileModificationDates.end()) {
		return it->second;
	}
	//PERF this is slow, since we need to open files multiple times (first to get modification date and then to actually open)
	struct stat s;
	uint64_t result;
	if (stat(assetPath.c_str(), &s) == 0)
	{
		result = s.st_mtime;
	}
	else {
		result = 0;
	}
	fileModificationDates[assetPath] = result;
	return result;
}

uint64_t AssetDatabase_BinaryImporterHandle::GetLastModificationTime(const std::string& assetPath) const {
	return database->GetLastModificationTime(assetPath);
}

void AssetDatabase_BinaryImporterHandle::GetLastModificationTime(const std::string& assetPath, uint64_t& assetModificationTime, uint64_t& metaModificationTime) const {
	assetModificationTime = database->GetLastModificationTime(assetPath);
	metaModificationTime = database->GetLastModificationTime(assetPath + ".meta");
}


void AssetDatabase_BinaryImporterHandle::GetLastModificationTime(uint64_t& assetModificationTime, uint64_t& metaModificationTime) const {
	//TODO naming conventions what is assetPath
	return GetLastModificationTime(database->assetsRootFolder + assetPath, assetModificationTime, metaModificationTime);
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
	//TODO make always lower case
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

bool AssetDatabase_BinaryImporterHandle::ReadAssetAsYAML(std::unique_ptr<ryml::Tree>& node) {
	return ReadYAML(GetAssetPath(), node);
}

bool AssetDatabase_BinaryImporterHandle::ReadMeta(std::unique_ptr<ryml::Tree>& node) {
	const auto fullPath = database->assetsRootFolder + assetPath + ".meta";
	return ReadYAML(fullPath, node);
}

std::string AssetDatabase_BinaryImporterHandle::GetAssetPath() const { return database->assetsRootFolder + assetPath; }
std::string AssetDatabase_BinaryImporterHandle::GetAssetFileName() const { return database->GetFileName(assetPath); }
std::string AssetDatabase_BinaryImporterHandle::GetFileExtension() const { return database->GetFileExtension(assetPath); }

void AssetDatabase_BinaryImporterHandle::EnsureForderForTempFileExists(std::string filePath) {
	auto fullPath = GetTempPathFromFileName(filePath);
	auto firstFolder = fullPath.find_first_of("\\");
	for (int i = 0; i < fullPath.size(); i++) {
		if (fullPath[i] == '\\') {
			auto subpath = fullPath.substr(0, i);
			CreateDirectoryA(subpath.c_str(), NULL);
		}
	}
}

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

std::string AssetDatabase_BinaryImporterHandle::GetTempPathFromFileName(const std::string& fileName) const {
	return database->tempFolder + fileName;
}

std::string AssetDatabase_BinaryImporterHandle::GetLibraryPathFromId(const std::string& id) const {
	return database->libraryRootFolder + assetPath + "\\" + id;
}

void AssetDatabase_BinaryImporterHandle::WriteToLibraryFile(const std::string& id, const ryml::NodeRef& node) {
	//TODO checks and make sure folder exists
	const auto fullPath = GetLibraryPathFromId(id);
	std::ofstream fout(fullPath);
	fout << node;
}

void AssetDatabase_BinaryImporterHandle::WriteToLibraryFile(const std::string& id, const std::vector<uint8_t>& buffer) {
	//TODO checks and make sure folder exists
	const auto fullPath = GetLibraryPathFromId(id);
	std::ofstream fout(fullPath, std::ios::binary);
	if (buffer.size() > 0) {
		fout.write((char*)&buffer[0], buffer.size());
	}
}

bool AssetDatabase_BinaryImporterHandle::ReadFromLibraryFile(const std::string& id, std::unique_ptr<ryml::Tree>& node)
{
	const auto fullPath = GetLibraryPathFromId(id);

	std::ifstream input(fullPath, std::ios::ate | std::ios::binary);
	if (!input) {
		LogError("Failed to load '%s': file not found", assetPath.c_str());
		node = nullptr;
		return false;
	}
	node = StreamToYAML(input);
	return node != nullptr;
}

bool AssetDatabase_BinaryImporterHandle::ReadFromLibraryFile(const std::string& id, std::vector<uint8_t>& buffer)
{
	return ReadBinary(GetLibraryPathFromId(id), buffer);
}

std::string AssetDatabase_BinaryImporterHandle::GetToolPath(std::string toolName) const {
	//TODO less hardcoded
	return "engine\\tools\\" + toolName;
}

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

	ResizeVectorNoInit(buffer, size);
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


bool AssetDatabase_BinaryImporterHandle::ReadYAML(const std::string& fullPath, std::unique_ptr<ryml::Tree>& node)
{
	std::ifstream input(fullPath, std::ios::ate | std::ios::binary);
	if (!input) {
		//TODO
		//LogError("Failed to load '%s': file not found", fullPath.c_str());
		node = nullptr;
		return false;
	}
	node = StreamToYAML(input);
	return node != nullptr;
}

std::string AssetDatabase::PathDescriptor::ToFullPath() {
	return assetId.size() > 0 ? FormatString("%s$%s", assetPath.c_str(), assetId.c_str()) : assetPath;
}
