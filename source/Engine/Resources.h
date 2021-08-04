#pragma once

#include "Common.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>


class AssetLoadingContext {
public:
	std::istream* inputFile;
};


class Asset {

};

class TextAsset : public Asset {
public:
	bool Load(AssetLoadingContext& context) {
		yaml = YAML::Load(*(context.inputFile));
		return yaml.IsDefined();
	}

	const YAML::Node& GetYamlNode() const { return yaml; }
private:
	YAML::Node yaml;
};

class TextureAsset : public Asset {

};

class AssetDatabase;

class AssetImporter {
public:
	virtual std::shared_ptr<Asset> Import(AssetDatabase& database, const std::string& path) = 0;
};

class MeshAsset : public Asset {

};

class AssetDatabase {
public:
	bool Init() {
		//assetsFolderPrefix = SDL_GetBasePath();

		return true;
	}

	void Term() {}

	std::shared_ptr<TextAsset> LoadTextAsset(std::string path);


	template<typename AssetType>
	std::shared_ptr<AssetType> LoadByPath(std::string path) {
		return std::static_pointer_cast<AssetType>(LoadByPath(path));
	}

	std::shared_ptr<Asset> LoadByPath(std::string path);

	static void RegisterImporter(std::string assetType, std::unique_ptr<AssetImporter>&& importer);

private:
	std::string assetsFolderPrefix = "assets\\";

	std::string GetFileName(std::string path);
	std::string GetFileExtension(std::string path);

	std::unordered_map<std::string, std::shared_ptr<Asset>> assets;
	std::unordered_map<std::string, std::shared_ptr<TextAsset>> textAssets;

	static std::unordered_map<std::string, std::unique_ptr<AssetImporter>>& GetAssetImporters();
};
