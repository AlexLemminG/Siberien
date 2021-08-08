#pragma once

#include "Common.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include "Object.h"


class AssetLoadingContext {
public:
	std::istream* inputFile;
};


class TextAsset : public Object {
public:
	bool Load(AssetLoadingContext& context) {
		yaml = YAML::Load(*(context.inputFile));
		if (!yaml.IsDefined()) {
			return false;
		}
		else {
			return true;
		}
	}

	const YAML::Node& GetYamlNode() const { return yaml; }
private:
	YAML::Node yaml;
};

class TextureAsset : public Object {

};

class BinaryAsset : public Object {
public:
	std::vector<char> buffer;
	BinaryAsset(std::vector<char>&& buffer) :
		buffer(buffer) {}
};

class AssetDatabase;

class AssetImporter {
public:
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const std::string& path) = 0;
};

class TextAssetImporter {
public:
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const YAML::Node& node) = 0;
};

class MeshAsset : public Object {

};

//assets referenced by path from assets/ folder
//ex: prefabs/hero.asset
//    textures/floor.png
//each path can contain multiple assets even of the same type, they could be referenced by @__id
//__id is decalred explicitly for text assets such as *.asset
//__id is generated for meshes
//ex: meshes/hero.fbx@animations/run
//ex: meshes/hero.fbx@bones/left_leg
//ex: meshes/hero.fbx@meshes/hat
//ex: meshes/hero.fbx@materials/head
//
//TODO
//does this make bone asset ?
//maybe yes and we need to rename it to Object
class AssetDatabase {
	friend AssetImporter;
public:
	bool Init() {
		mainDatabase = this;
		return true;
	}

	void Term() {
		mainDatabase = nullptr;
	}


	//TODO make sure everyone knows that it does not cache
	std::shared_ptr<TextAsset> LoadTextAsset(std::string path);
	std::shared_ptr<TextAsset> LoadTextAssetFromLibrary(std::string path);
	std::shared_ptr<BinaryAsset> LoadBinaryAsset(std::string path);
	std::shared_ptr<BinaryAsset> LoadBinaryAssetFromLibrary(std::string path);

	template<typename AssetType>
	std::shared_ptr<AssetType> LoadByPath(std::string path) {
		auto& asset = LoadByPath(path);
		return std::dynamic_pointer_cast<AssetType>(asset);
	}

	std::shared_ptr<Object> LoadByPath(std::string path);

	static void RegisterBinaryAssetImporter(std::string assetType, std::unique_ptr<AssetImporter>&& importer);
	static void RegisterTextAssetImporter(std::string assetType, std::unique_ptr<TextAssetImporter>&& importer);

	template<typename T>
	void RequestObjectPtr(std::shared_ptr<T>& dest, std::string objectPath) {
		RequestObjectPtr((void*)&dest, objectPath);
	}
	static AssetDatabase* Get();

	std::shared_ptr<Object> GetLoaded(std::string path);

	void AddAsset(std::shared_ptr<Object> asset, std::string path, std::string id);

	static const std::string assetsFolderPrefix;
	static const std::string libraryAssetsFolderPrefix;

	std::string GetLibraryPath(std::string assetPath);

	std::string GetFileName(std::string path);

	//TODO this class is getting out of hand
	std::string GetToolsPath(std::string path) { return "tools\\" + path; }
	std::string GetAssetPath(std::string path) { return assetsFolderPrefix + "\\" + path; }
	long GetLastModificationTime(std::string path);
	void CreateFolders(std::string fullPath);

private:
	class PathDescriptor {
	public:
		PathDescriptor(std::string path);
		std::string assetPath;
		std::string assetId;
	};

	std::string currentAssetLoadingPath;

	void LoadAllAtPath(std::string path);
	void LoadNextWhileNotEmpty();

	std::string GetFileExtension(std::string path);


	void RequestObjectPtr(void* dest, std::string objectPath);

	//TODO main asset
	std::unordered_map<std::string, std::vector<std::pair<PathDescriptor, std::shared_ptr<Object>>>> assets;
	//std::unordered_map<std::string, std::shared_ptr<TextAsset>> textAssets;

	std::vector<std::string> requestedAssetsToLoad;
	//TODO typeinfo for safety
	std::unordered_map<void*, std::string> requestedObjectPtrs;

	static std::unordered_map<std::string, std::unique_ptr<AssetImporter>>& GetAssetImporters();
	static std::unordered_map<std::string, std::unique_ptr<TextAssetImporter>>& GetTextAssetImporters();

	static AssetDatabase* mainDatabase;

};
