#include "Reflect.h"
#include <fstream>
#include "SMath.h"
#include "Resources.h"

void SerializationContext::FlushRequiestedToSerialize() {
	if (rootObjectsRequestedToSerialize != &objectsRequestedToSerialize) {
		ASSERT(false);
		return;
	}
	std::unordered_map<std::shared_ptr<Object>, std::string> objectPaths;
	for (int i = 0; i < objectsRequestedToSerialize.size(); i++) {
		auto obj = objectsRequestedToSerialize[i];
		if (obj == nullptr) {
			continue;
		}
		if (std::find(objectsAllowedToSerialize.begin(), objectsAllowedToSerialize.end(), obj) == objectsAllowedToSerialize.end()) {
			if (database) {
				objectPaths[obj] = database->GetAssetPath(obj);
			}
			continue;
		}
		int sameTypeIdx = i;
		std::string type = obj->GetType()->GetName();
		std::string path = FormatString("%s$%d", type.c_str(), i);
		std::string id = FormatString("$%d", i);
		::Serialize(Child(path), *(obj.get()));
		objectPaths[obj] = id;
	}
	for (auto it : objectsRequestedToSerializeRequesters) {
		std::string path = objectPaths[it.first];
		for (auto& node : it.second) {
			node = path;
		}
	}

}

void SerializationContext::RequestDeserialization(void* ptr, const std::string& assetPath) const {
	if (database) {
		//TODO request load + request ptr
		database->RequestObjPtr(ptr, assetPath);
	}
	else {
		//TODO assert false
		AssetDatabase::Get()->RequestObjPtr(ptr, assetPath);
	}
}
