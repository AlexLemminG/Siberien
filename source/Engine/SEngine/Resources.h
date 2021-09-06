#pragma once

#include "Common.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include "Reflect.h"
#include "Object.h"
#include "GameEvents.h"
#include "Serialization.h"


//assets referenced by path from assets/ folder
//ex: prefabs/hero.asset
//    textures/floor.png
//each path can contain multiple assets even of the same type, they could be referenced by $__id
//__id is decalred explicitly for text assets such as *.asset
//__id is generated for meshes
//ex: meshes/hero.fbx$animations/run
//ex: meshes/hero.fbx$bones/left_leg
//ex: meshes/hero.fbx$meshes/hat
//ex: meshes/hero.fbx$materials/head
//
//TODO
//does this make bone asset ?
//maybe yes and we need to rename it to Object

namespace YAML {
	class Node;
}

class SE_CPP_API AssetDatabase_BinaryImporterHandle {
	friend class AssetDatabase;
	AssetDatabase_BinaryImporterHandle(AssetDatabase* database, std::string assetPath) :database(database), assetPath(assetPath) {}
public:
	void AddAssetToLoaded(std::string id, std::shared_ptr<Object> object);

	bool ReadAssetAsBinary(std::vector<uint8_t>& buffer);
	bool ReadAssetAsYAML(YAML::Node& node);
	bool ReadMeta(YAML::Node& node);

	void WriteToLibraryFile(const std::string& id, const YAML::Node& node);
	void WriteToLibraryFile(const std::string& id, std::vector<uint8_t>& buffer);
	bool ReadFromLibraryFile(const std::string& id, YAML::Node& node);
	bool ReadFromLibraryFile(const std::string& id, std::vector<uint8_t>& buffer);
	std::string GetToolPath(std::string toolName);

	void GetLastModificationTime(long& assetModificationTime, long& metaModificationTime);

	std::string GetAssetPath();
	std::string GetFileExtension();
	void EnsureForderForLibraryFileExists(std::string id);

	std::string GetLibraryPathFromId(const std::string& id);
	

private:
	bool ReadBinary(const std::string& fullPath, std::vector<uint8_t>& buffer);
	bool ReadYAML(const std::string& fullPath, YAML::Node& node);
	std::string assetPath;
	AssetDatabase* database = nullptr;
};

class SE_CPP_API AssetDatabase {
	friend AssetDatabase_BinaryImporterHandle;
	friend AssetDatabase_TextImporterHandle;
public:
	bool Init();

	void Term();

	void UnloadAll();

	template<typename T>
	std::shared_ptr<T> Load(const std::string& path) {
		auto descriptor = PathDescriptor(path);
		auto loaded = std::dynamic_pointer_cast<T>(GetLoaded(descriptor, GetReflectedType<T>()));
		if (loaded) {
			return loaded;
		}
		LoadAsset(descriptor.assetPath);
		return std::dynamic_pointer_cast<T>(GetLoaded(descriptor, GetReflectedType<T>()));
	}
	//TODO LoadAll

	//TODO where T is Object
	template<typename T>
	std::shared_ptr<T> DeserializeFromYAML(const ryml::NodeRef& node) {
		//TODO not only mainObj
		auto mainObj = DeserializeFromYAMLInternal(node);
		return std::dynamic_pointer_cast<T>(mainObj);
	}

	std::string GetAssetUID(std::shared_ptr<Object> obj);
	//returns 
	std::string GetAssetPath(std::shared_ptr<Object> obj);

	static AssetDatabase* Get();

	GameEvent<> onBeforeUnloaded;
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
			if (id.size() == 0) {
				return GetMain();
			}
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
		PathDescriptor(std::string assetPath, std::string id) {
			this->assetId = id;
			this->assetPath = assetPath;
		}
		std::string assetPath;
		std::string assetId;

		std::string ToFullPath();
	};
	class LoadingRequest {
	public:
		void* target;
		PathDescriptor descriptor;
	};

	std::shared_ptr<Object> GetLoaded(const PathDescriptor& path, ReflectedTypeBase* type);
	void LoadAsset(const std::string& path);
	std::string GetFileExtension(std::string path);
	std::string GetFileName(std::string path);
	std::shared_ptr<Object> DeserializeFromYAMLInternal(const ryml::NodeRef& node);
	void ProcessLoadingQueue();
	std::shared_ptr<AssetImporter>& GetAssetImporter(const std::string& type);

	std::unordered_map<std::string, Asset> assets;
	std::unordered_map<std::shared_ptr<Object>, std::string> objectPaths;

	std::string currentAssetLoadingPath;
	std::vector<LoadingRequest> loadingQueue;

	std::string assetsRootFolder = "assets\\";
	std::string libraryRootFolder = "library\\assets\\";

	static AssetDatabase* mainDatabase;
};