#include "Common.h"

#include "Resources.h"
#include <memory>
#include <windows.h>
#include "Serialization.h"
#include "SDL_filesystem.h"
#include <filesystem>
#include <locale>
#include <codecvt>


bool AssetDatabase2::Init() {
	OPTICK_EVENT();
	return true;
}

void AssetDatabase2::Term() {
	OPTICK_EVENT();
	UnloadAll();
}

void AssetDatabase2::UnloadAll() {
	OPTICK_EVENT();
	assets.clear();
	objectPaths.clear();
}

std::string AssetDatabase2::GetAssetPath(std::shared_ptr<Object> obj) {
	auto it = objectPaths.find(obj);
	if (it != objectPaths.end()) {
		return it->second;
	}
	else {
		return "";
	}
}


std::shared_ptr<AssetImporter2>& AssetDatabase2::GetAssetImporter(const std::string& type) {
	return GetSerialiationInfoStorage().GetBinaryImporter2(type);
}

std::shared_ptr<AssetImporter>& AssetDatabase::GetAssetImporter(const std::string& type) {
	return GetSerialiationInfoStorage().GetBinaryImporter(type);
}

std::shared_ptr<TextAssetImporter>& AssetDatabase::GetTextAssetImporter(const std::string& type) {
	return GetSerialiationInfoStorage().GetTextImporter(type);
}

AssetDatabase* AssetDatabase::mainDatabase = nullptr;
AssetDatabase* AssetDatabase::Get() {
	return AssetDatabase::mainDatabase;
}


std::shared_ptr<Object> AssetDatabase2::GetLoaded(const PathDescriptor& descriptor, ReflectedTypeBase* type) {
	if (assets.find(descriptor.assetPath) != assets.end()) {
		//TODO
		//return assets[descriptor.assetPath].Get(GetReflectedType<T>(), descriptor.assetPath);
		return assets[descriptor.assetPath].Get(descriptor.assetId);
	}
	else {
		return nullptr;
	}
}

void AssetDatabase2::LoadAsset(const std::string& path) {
	if (assets.find(path) != assets.end()) {
		return;//TODO dont call LoadAsset in the first place
	}
	auto extention = GetFileExtension(path);
	//TODO get extensions list from asset importer
	if (extention == "png") {
		auto& importer = GetAssetImporter("Texture");
		if (importer) {
			importer->ImportAll(AssetDatabase2_BinaryImporterHandle(this, path));
		}
	}
	else if (extention == "fbx" || extention == "glb" || extention == "blend") {
		auto& importer = GetAssetImporter("Mesh");
		if (importer) {
			importer->ImportAll(AssetDatabase2_BinaryImporterHandle(this, path));
		}
	}
	else if (extention == "asset") {

	}
	else {
		return;
	}
}

void AssetDatabase2_BinaryImporterHandle::GetLastModificationTime(long& assetModificationTime, long& metaModificationTime) {

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


std::string AssetDatabase2::GetFileName(std::string path) {
	auto lastSlash = path.find_last_of('\\');
	if (lastSlash == -1) {
		return path;
	}
	else {
		return path.substr(lastSlash + 1, path.length() - lastSlash - 1);
	}
}
std::string AssetDatabase2::GetFileExtension(std::string path) {
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
		return database2.Load<Object>(path);
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

bool AssetDatabase::Init() {
	OPTICK_EVENT();
	mainDatabase = this;
	database2.Init();
	return true;
}

void AssetDatabase::Term() {
	OPTICK_EVENT();
	UnloadAll();
	mainDatabase = nullptr;
	database2.Term();
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

std::shared_ptr<Object> AssetDatabase::LoadFromYaml(const YAML::Node& node) {
	std::string path = "***temp_storage***";
	currentAssetLoadingPath = path;
	LoadAllAtYaml(node, path);
	currentAssetLoadingPath = "";

	LoadNextWhileNotEmpty();

	auto asset = GetLoaded(path);

	assets[path].clear();

	return asset;
}

void AssetDatabase::LoadAllAtYaml(const YAML::Node& node, const std::string& path) {
	//TODO return invalid asset ?
	if (!node.IsDefined()) {
		currentAssetLoadingPath = "";//TODO scope exis shit
		return;
	}
	for (const auto& kv : node) {
		PathDescriptor descriptor(kv.first.as<std::string>());
		std::string type = descriptor.assetPath;
		std::string id = descriptor.assetId.size() > 0 ? descriptor.assetId : descriptor.assetPath;

		auto node = kv.second;

		auto& importer = GetTextAssetImporter(type);
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

std::vector<std::string> AssetDatabase::GetAllAssetNames() {
	auto result = std::vector<std::string>();
	std::string path = assetsFolderPrefix;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
		if (entry.is_regular_file()) {
			std::wstring string_to_convert = entry.path();
			//setup converter
			using convert_type = std::codecvt_utf8<wchar_t>;
			std::wstring_convert<convert_type, wchar_t> converter;

			//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
			std::string converted_str = converter.to_bytes(string_to_convert);
			converted_str = converted_str.substr(assetsFolderPrefix.size(), converted_str.size() - assetsFolderPrefix.size());

			result.push_back(converted_str);
		}
	}

	//SDL_FindFirst();
	return result;
}

void AssetDatabase::LoadAllAtPath(std::string path)
{
	if (path.empty()) {
		return;
	}
	currentAssetLoadingPath = path;
	std::string ext = GetFileExtension(path);
	if (ext == "asset") {
		auto textAsset = LoadTextAsset(path);
		if (!textAsset) {
			currentAssetLoadingPath = "";//TODO scope exis shit
			return;
		}

		LoadAllAtYaml(textAsset->GetYamlNode(), path);
	}
	else if (ext == "fbx" || ext == "glb" || ext == "blend") {
		database2.Load<Object>(path);
	}
	else if (ext == "png") {
		database2.Load<Object>(path);
	}
	else if (ext == "wav") {
		std::string type = "AudioClip";
		auto& importer = GetAssetImporter(type);
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
			if (!it.second.empty()) {
				LogError("Asset not found '%s'", it.second.c_str());
			}
			*(std::shared_ptr<Object>*)(it.first) = nullptr;
		}
		else {
			//TODO check type
			*(std::shared_ptr<Object>*)(it.first) = object;
		}
	}
	requestedObjectPtrs.clear();
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

void AssetDatabase::UnloadAll() {
	OPTICK_EVENT();
	assets.clear();
	requestedAssetsToLoad.clear();
	requestedObjectPtrs.clear();
	database2.UnloadAll();
	onUnloaded.Invoke();
}

std::string AssetDatabase::GetAssetPath(std::shared_ptr<Object> obj) {
	//WARN slow
	for (auto it : assets) {
		for (auto it2 : it.second) {
			if (it2.second == obj) {
				return it2.first.ToFullPath();
			}
		}
	}
	return database2.GetAssetPath(obj);
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

void AssetDatabase::RequestObjPtr(void* dest, std::string objectPath) {
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

std::string AssetDatabase::PathDescriptor::ToFullPath() {
	return assetId.size() > 0 ? FormatString("%s$%s", assetPath.c_str(), assetId.c_str()) : assetPath;
}

void AssetDatabase2_BinaryImporterHandle::AddAssetToLoaded(std::string id, std::shared_ptr<Object> object)
{
	auto& asset = database->assets[assetPath];
	asset.Add(id, object);
	database->objectPaths[object] = AssetDatabase2::PathDescriptor(assetPath, id).ToFullPath();
}

bool AssetDatabase2_BinaryImporterHandle::ReadAssetAsBinary(std::vector<uint8_t>& buffer)
{
	return ReadBinary(GetAssetPath(), buffer);
}

bool AssetDatabase2_BinaryImporterHandle::ReadMeta(YAML::Node& node) {
	const auto fullPath = database->assetsRootFolder + assetPath + ".meta";
	return ReadYAML(fullPath, node);
}

std::string AssetDatabase2_BinaryImporterHandle::GetAssetPath() { return database->assetsRootFolder + assetPath; }

void AssetDatabase2_BinaryImporterHandle::EnsureForderForLibraryFileExists(std::string id) {
	auto fullPath = GetLibraryPathFromId(id);
	auto firstFolder = fullPath.find_first_of("\\");
	for (int i = 0; i < fullPath.size(); i++) {
		if (fullPath[i] == '\\') {
			auto subpath = fullPath.substr(0, i);
			CreateDirectoryA(subpath.c_str(), NULL);
		}
	}
}

std::string AssetDatabase2_BinaryImporterHandle::GetLibraryPathFromId(const std::string& id) {
	return database->libraryRootFolder + assetPath + "\\" + id;
}

void AssetDatabase2_BinaryImporterHandle::WriteToLibraryFile(const std::string& id, const YAML::Node& node) {
	//TODO checks and make sure folder exists
	const auto fullPath = GetLibraryPathFromId(id);
	std::ofstream fout(fullPath);
	fout << node;
}

void AssetDatabase2_BinaryImporterHandle::WriteToLibraryFile(const std::string& id, std::vector<uint8_t>& buffer) {
	//TODO checks and make sure folder exists
	const auto fullPath = GetLibraryPathFromId(id);
	std::ofstream fout(fullPath, std::ios::binary);
	if (buffer.size() > 0) {
		fout.write((char*)&buffer[0], buffer.size());
	}
}

bool AssetDatabase2_BinaryImporterHandle::ReadFromLibraryFile(const std::string& id, YAML::Node& node)
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

bool AssetDatabase2_BinaryImporterHandle::ReadFromLibraryFile(const std::string& id, std::vector<uint8_t>& buffer)
{
	return ReadBinary(GetLibraryPathFromId(id), buffer);
}

std::string AssetDatabase2_BinaryImporterHandle::GetToolPath(std::string toolName) { return "tools\\" + toolName; }

bool AssetDatabase2_BinaryImporterHandle::ReadBinary(const std::string& fullPath, std::vector<uint8_t>& buffer)
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


bool AssetDatabase2_BinaryImporterHandle::ReadYAML(const std::string& fullPath, YAML::Node& node)
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

std::string AssetDatabase2::PathDescriptor::ToFullPath() {
	return assetId.size() > 0 ? FormatString("%s$%s", assetPath.c_str(), assetId.c_str()) : assetPath;
}
