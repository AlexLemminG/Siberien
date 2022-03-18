

#include "Reflect.h"
#include <fstream>
#include "SMath.h"
#include "Resources.h"
#include "ryml.hpp"

static_assert(sizeof(ryml::NodeRef) == sizeof(RymlNodeRefDummy));

SerializationContext::SerializationContext(const ryml::NodeRef& yamlNode, std::vector<std::shared_ptr<Object>> objectsAllowedToSerialize)
	:yamlNode(*(RymlNodeRefDummy*)&(yamlNode))
	, objectsAllowedToSerialize(objectsAllowedToSerialize)
	, rootObjectsRequestedToSerialize(&objectsRequestedToSerialize)
	, rootObjectsRequestedToSerializeRequesters(&objectsRequestedToSerializeRequesters)
	, rootObjectsAllowedToSerialize(&(this->objectsAllowedToSerialize))
	, rootDeserializedObjects(&(this->deserializedObjects))
{}

SerializationContext::SerializationContext(std::vector<std::shared_ptr<Object>> objectsAllowedToSerialize)
{
	this->yamlTree = std::make_shared<ryml::Tree>();
	this->yamlNode = *(RymlNodeRefDummy*)&((ryml::NodeRef)*yamlTree);
	this->objectsAllowedToSerialize = objectsAllowedToSerialize;
	this->rootObjectsRequestedToSerialize = (&objectsRequestedToSerialize);
	this->rootObjectsRequestedToSerializeRequesters = (&objectsRequestedToSerializeRequesters);
	this->rootObjectsAllowedToSerialize = (&(this->objectsAllowedToSerialize));
	this->rootDeserializedObjects = (&(this->deserializedObjects));
}

void SerializationContext::SetType(SerializationContextType type) {
	switch (type)
	{
	case SerializationContextType::Sequence:
		GetYamlNode() |= ryml::SEQ;
		break;
	case SerializationContextType::Map:
		GetYamlNode() |= ryml::MAP;
		break;
	default:
		break;
	}
	//TODO
}

const bool SerializationContext::IsDefined() const {
	return GetYamlNode().valid();
}

const SerializationContext SerializationContext::Child(const std::string& name) const {
	if (!GetYamlNode().is_map()) {
		return SerializationContext(ryml::NodeRef(), *this);
	}
	else {
		return SerializationContext(GetYamlNode()[c4::csubstr(name.c_str(), name.length())], *this);
	}
}

SerializationContext SerializationContext::Child(const std::string& name) {
	//TODO create only when value set (not when non const child is acquired)
	ryml::NodeType_e type = GetYamlNode().type();
	type = (ryml::NodeType_e)(type & (-1 ^ ryml::VAL) | ryml::MAP);
	GetYamlNode().set_type(type);
	auto cname = c4::csubstr(name.c_str(), name.length());
	ryml::NodeRef child;
	if (GetYamlNode().has_child(cname)) {
		child = GetYamlNode()[cname];
	}
	else {
		child = GetYamlNode().append_child();
		child.set_key_serialized(cname);
	}
	return SerializationContext(child, *this);
}

const SerializationContext SerializationContext::Child(int iChild) const {
	if (GetYamlNode().num_children() > iChild) {
		return SerializationContext(GetYamlNode()[iChild], *this);
	}
	else {
		return SerializationContext(ryml::NodeRef(), *this);
	}
}

SerializationContext SerializationContext::Child(int iChild) {
	GetYamlNode() |= ryml::SEQ;
	ryml::NodeRef child;
	if (GetYamlNode().num_children() > iChild) {
		child = GetYamlNode()[iChild];
	}
	else {
		child = GetYamlNode().append_child();
		//TODO ASSERT(yamlNode.num_children() > iChild);
	}
	return SerializationContext(GetYamlNode()[iChild], *this);
}

bool SerializationContext::IsSequence() const {
	return GetYamlNode().is_seq();
}

bool SerializationContext::IsMap() const {
	return GetYamlNode().is_map();
}

int SerializationContext::Size() const {
	return GetYamlNode().num_children();
}

std::vector<std::string> SerializationContext::GetChildrenNames() const {
	std::vector<std::string> children;
	for (auto c : GetYamlNode()) {
		children.push_back(std::string(c.key().str, c.key().len));
	}
	return children;
}

void SerializationContext::Clear() {
	GetYamlNode().tree()->remove(GetYamlNode().id());
}

void SerializationContext::ClearValue() {
	GetYamlNode().clear_children();
	if (GetYamlNode().has_key()) {
		auto key = GetYamlNode().key();
		GetYamlNode().clear_val();
		GetYamlNode().set_key(key);
	}
	else {
		GetYamlNode().clear_val();
	}
	GetYamlNode().set_type(GetYamlNode().type() & (ryml::KEY) | ryml::VAL);//empty val
}

void SerializationContext::AddAllowedToSerializeObject(std::shared_ptr<Object> obj) {
	rootObjectsAllowedToSerialize->push_back(obj);
}

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
			(*(ryml::NodeRef*)&node) << c4::csubstr(path.c_str(), path.length());
		}
	}
}

void SerializationContext::FinishDeserialization() {
	for (auto obj : deserializedObjects) {
		//TODO not this!!
		obj->OnAfterDeserializeCallback(*this);
	}
}

ryml::NodeRef& SerializationContext::GetYamlNode() { return *(ryml::NodeRef*)&yamlNode; }
const ryml::NodeRef& SerializationContext::GetYamlNode() const { return *(ryml::NodeRef*)&yamlNode; }

SerializationContext::SerializationContext(ryml::NodeRef yamlNode, const SerializationContext& parent)
	:yamlNode(*(RymlNodeRefDummy*)&(yamlNode))
	, databaseHandle(parent.databaseHandle)
	, rootObjectsRequestedToSerialize(parent.rootObjectsRequestedToSerialize)
	, rootObjectsAllowedToSerialize(parent.rootObjectsAllowedToSerialize)
	, rootDeserializedObjects(parent.rootDeserializedObjects)
	, rootObjectsRequestedToSerializeRequesters(parent.rootObjectsRequestedToSerializeRequesters) {}

void SerializationContext::RequestDeserialization(void* ptr, const std::string& assetPath) const {
	if (databaseHandle) {
		databaseHandle->RequestObjectPtr(ptr, assetPath);
	}
	else {
		ASSERT(false);
	}
}
void SerializationContext::operator<<(const int& t) {
	GetYamlNode() << t;
}
void SerializationContext::operator>>(int& t) const {
	GetYamlNode() >> t;
}
void SerializationContext::operator<<(const long& t) {
	GetYamlNode() << t;
}
void SerializationContext::operator>>(long& t) const {
	GetYamlNode() >> t;
}
void SerializationContext::operator<<(const uint64_t& t) {
	GetYamlNode() << t;
}
void SerializationContext::operator>>(uint64_t& t) const {
	GetYamlNode() >> t;
}
void SerializationContext::operator<<(const unsigned int& t) {
	GetYamlNode() << t;
}
void SerializationContext::operator>>(unsigned int& t) const {
	GetYamlNode() >> t;
}
void SerializationContext::operator<<(const float& t) {
	GetYamlNode() << t;
}
void SerializationContext::operator>>(float& t) const {
	GetYamlNode() >> t;
}
void SerializationContext::operator<<(const bool& t) {
	GetYamlNode() << t;
}
void SerializationContext::operator>>(bool& t) const {
	GetYamlNode() >> t;
}
void SerializationContext::operator<<(const std::string& t) {
	GetYamlNode() << t.c_str();
}
void SerializationContext::operator>>(std::string& t) const {
	c4::csubstr csubstr;
	GetYamlNode() >> csubstr;
	t = std::string(csubstr.str, csubstr.len);
}


class Test {
	REFLECT_BEGIN(Test);
	REFLECT_END();
};

class TestInh {
	REFLECT_BEGIN(TestInh, Test);
	REFLECT_END();
};

void ReflectedTypeString::Serialize(SerializationContext& context, const void* object)
{
	context << *((std::string*)object);
}

void ReflectedTypeString::Deserialize(const SerializationContext& context, void* object)
{
	if (context.IsDefined()) {
		context >> *((std::string*)object);
	}
}
