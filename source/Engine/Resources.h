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
public:
	//TODO AssetLoadingFactory or some shit
	virtual bool Load(AssetLoadingContext& context) = 0;
};

class TextAsset : public Asset {
public:
	bool Load(AssetLoadingContext& context) override {
		yaml = YAML::Load(*(context.inputFile));
		return yaml.IsDefined();
	}

	const YAML::Node& GetYamlNode() const { return yaml; }
private:
	YAML::Node yaml;
};

class TextureAsset : public Asset {

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

	template<typename AssetType>
	std::shared_ptr<AssetType> LoadByPath(std::string path) {
		auto it = assets.find(path);

		if (it != assets.end()) {
			return std::static_pointer_cast<AssetType>(it->second);
		}
		if (TryLoad(path)) {
			it = assets.find(path);
		}

		if (it != assets.end()) {
			return std::static_pointer_cast<AssetType>(it->second);
		}
		return nullptr;
	}

private:
	std::string assetsFolderPrefix = "assets\\";

	std::string GetFileName(std::string path) {
		auto lastSlash = path.find_last_of('\\');
		if (lastSlash == -1) {
			return "";
		}
		else {
			return path.substr(lastSlash + 1, path.length() - lastSlash - 1);
		}
	}
	std::string GetFileExtension(std::string path) {
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

	bool TryLoad(std::string path) {
		auto fullPath = assetsFolderPrefix + path;
		std::ifstream input(fullPath);
		if (!input) {
			//TODO
			//ASSERT(false);
			return false;
		}

		AssetLoadingContext context;
		context.inputFile = &input;

		auto ext = GetFileExtension(fullPath);
		if (ext == "asset") {
			auto textAsset = std::make_shared<TextAsset>();
			if (textAsset->Load(context)) {
				assets[path] = textAsset;
				return true;
			}
			return false;
		}
		//ASSERT(false);
		return false;
	}

	std::unordered_map<std::string, std::shared_ptr<Asset>> assets;
};
