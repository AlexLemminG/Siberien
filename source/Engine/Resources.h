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
		return yaml.IsDefined();
	}

	const YAML::Node& GetYamlNode() const { return yaml; }
private:
	YAML::Node yaml;
};

class TextureAsset : public Object {

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


	std::shared_ptr<TextAsset> LoadTextAsset(std::string path);

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

	std::string GetFileName(std::string path);
	std::string GetFileExtension(std::string path);


	void RequestObjectPtr(void* dest, std::string objectPath);

	//TODO main asset
	std::unordered_map<std::string, std::vector<std::pair<PathDescriptor, std::shared_ptr<Object>>>> assets;
	std::unordered_map<std::string, std::shared_ptr<TextAsset>> textAssets;

	std::vector<std::string> requestedAssetsToLoad;
	//TODO typeinfo for safety
	std::unordered_map<void*, std::string> requestedObjectPtrs;

	static std::unordered_map<std::string, std::unique_ptr<AssetImporter>>& GetAssetImporters();
	static std::unordered_map<std::string, std::unique_ptr<TextAssetImporter>>& GetTextAssetImporters();

	static AssetDatabase* mainDatabase;

};
