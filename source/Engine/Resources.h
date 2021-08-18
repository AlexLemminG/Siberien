#pragma once

#include "Common.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include "Reflect.h"
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

	REFLECT_BEGIN(TextAsset);
	REFLECT_END();
};

class TextureAsset : public Object {

	REFLECT_BEGIN(TextureAsset);
	REFLECT_END();
};

class BinaryAsset : public Object {
public:
	std::vector<char> buffer;
	BinaryAsset(std::vector<char>&& buffer) :
		buffer(buffer) {}

	~BinaryAsset() {
	}

	REFLECT_BEGIN(BinaryAsset);
	REFLECT_END();
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

class AssetDatabase2 {
	bool Init() {
	}

	void Term() {
	}

	template<typename T>
	std::shared_ptr<T> Load(const std::string& path) {

	}

	std::vector<std::shared_ptr<Object>> LoadAll(const std::string& path);

	class ImporterHandle {
	public:
		void AddAssetToLoaded(std::string id, std::shared_ptr<Object> object);

		bool ReadAssetAsBinary(std::vector<char>& buffer);
		bool ReadMeta(YAML::Node& node);

		void WriteToLibraryFile(const std::string& id, const YAML::Node& node); 
		void WriteToLibraryFile(const std::string& id, std::vector<char>& buffer);
		bool ReadFromLibraryFile(const std::string& id, YAML::Node& node);
		bool ReadFromLibraryFile(const std::string& id, std::vector<char>& buffer);
	private:
		bool ReadBinary(const std::string& fullPath, std::vector<char>& buffer);
		bool ReadYAML(const std::string& fullPath, YAML::Node& node);
		std::string GetLibraryPath(const std::string id);
		std::string assetPath;
		AssetDatabase2* database = nullptr;
	};

private:
	class Asset {
	public:
		void Add(const std::string& id, std::shared_ptr<Object> object) {
			objects.push_back(SingleObject{ object, id });
		}
		std::shared_ptr<Object> GetMain() {
			return objects.size() > 0 ? objects[0].obj : std::shared_ptr<Object>();
		}
		std::shared_ptr<Object> Get(const std::string& id) {
			for (const auto& so : objects) {
				if (so.id == id) {
					return so.obj;
				}
			}
			return nullptr;
		}
		std::shared_ptr<Object> Get(const ReflectedTypeBase* type) {
			for (const auto& so : objects) {
				if (so.obj->GetType() == type) {
					return so.obj;
				}
			}
			return nullptr;
		}
		std::shared_ptr<Object> Get(const ReflectedTypeBase* type, const std::string& id) {
			for (const auto& so : objects) {
				if (so.obj->GetType() == type && so.id == id) {
					return so.obj;
				}
			}
			return nullptr;
		}
	private:
		class SingleObject {
		public:
			std::shared_ptr<Object> obj;
			std::string id;
		};

		std::vector<SingleObject> objects;
	};
	class PathDescriptor {
	public:
		PathDescriptor(std::string path) {
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
		std::string assetPath;
		std::string assetId;

		std::string ToFullPath() {
			return assetId.size() > 0 ? FormatString("%s$%s", assetPath.c_str(), assetId.c_str()) : assetPath;
		}
	};

	std::shared_ptr<Object> Load(const PathDescriptor& path, ReflectedTypeBase* type);

	std::unordered_map<std::string, Asset> assets;

	std::string assetsRootFolder;
	std::string libraryRootFolder;
};

class AssetDatabase {
	friend AssetImporter;
public:


	bool Init() {
		mainDatabase = this;
		return true;
	}

	void Term() {
		UnloadAll();
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

	//TODO does it belong here ?
	template<typename AssetType>
	std::shared_ptr<AssetType> LoadFromYaml(const YAML::Node& node) {
		auto& asset = LoadFromYaml(node);
		return std::dynamic_pointer_cast<AssetType>(asset);
	}
	std::shared_ptr<Object> LoadFromYaml(const YAML::Node& node);


	static void RegisterBinaryAssetImporter(std::string assetType, std::unique_ptr<AssetImporter>&& importer);
	static void RegisterTextAssetImporter(std::string assetType, std::unique_ptr<TextAssetImporter>&& importer);

	template<typename T>
	void RequestObjPtr(std::shared_ptr<T>& dest, std::string objectPath) {
		RequestObjPtr((void*)&dest, objectPath);
	}
	void RequestObjPtr(void* dest, std::string objectPath);

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
	template<typename T>
	std::string GetAssetPath(std::shared_ptr<T> obj) {
		return GetAssetPath(std::dynamic_pointer_cast<Object>(obj));
	}
	std::string GetAssetPath(std::shared_ptr<Object> obj);

	void UnloadAll();
	class PathDescriptor {
	public:
		PathDescriptor(std::string path);
		std::string assetPath;
		std::string assetId;

		std::string ToFullPath() {
			return assetId.size() > 0 ? FormatString("%s$%s", assetPath.c_str(), assetId.c_str()) : assetPath;
		}
	};
private:

	std::string currentAssetLoadingPath;

	void LoadAllAtPath(std::string path);
	void LoadNextWhileNotEmpty();
	void LoadAllAtYaml(const YAML::Node& node, const std::string& path);

	std::string GetFileExtension(std::string path);



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
