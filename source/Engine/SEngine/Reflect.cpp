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
				objectPaths[obj] = database->GetAssetUID(obj);
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

void SerializationContext::FinishDeserialization() {
	for (auto obj : deserializedObjects) {
		//TODO not this!!
		obj->OnAfterDeserializeCallback(*this);
	}
}

void SerializationContext::RequestDeserialization(void* ptr, const std::string& assetPath) const {
	if (databaseHandle) {
		databaseHandle->RequestObjectPtr(ptr, assetPath);
	}
	else {
		ASSERT(false);
	}
}


class Test {
	REFLECT_BEGIN(Test);
	REFLECT_END();
};

class TestInh {
	REFLECT_BEGIN(TestInh, Test);
	REFLECT_END();
};