#include "SerializedObject.h"
#include <unordered_map>
#include <vector>
#include "Object.h"

struct SerializedObjectWithName {
	std::string name;
	SerializedObject serializedObject;
};

struct Data {
	//TODO union
	// {
		float f;
		int i;
		std::string str;
		std::shared_ptr<Object> obj;
		std::vector<SerializedObjectWithName> vec;
	//};
};

int i = sizeof(Data);
static_assert(sizeof(SerializedObject::RawData) >= sizeof(Data));

Data& GetData(SerializedObject::RawData& obj) {
	return *(Data*)&(obj);
}
const Data& GetData(const SerializedObject::RawData& obj) {
	return *(Data*)&(obj);
}

//<= sizeof(std::vector<SerializedObject>));

float SerializedObject::GetFloat() const {
	return GetData(rawData).f;//TODO check type
}

void SerializedObject::SetFloat(const float& f) {
	type = Type::FLOAT;
	GetData(rawData).f = f;
}

float SerializedObject::GetInt() const {
	return GetData(rawData).i;//TODO check type
}

void SerializedObject::SetInt(const int& i) {
	type = Type::INT;
	GetData(rawData).i = i;
}

const char* SerializedObject::GetString() const {
	return GetData(rawData).str.c_str();//TODO check type}
}
void SerializedObject::SetString(const char* str) {
	type = Type::STRING;
	GetData(rawData).str = std::string(str);
}

std::shared_ptr<Object> SerializedObject::GetObject() const {
	return GetData(rawData).obj;//TODO check type
}

void SerializedObject::SetObject(const std::shared_ptr<Object>& object) {
	type = Type::OBJECT;
	GetData(rawData).obj = object;
}

SerializedObject SerializedObject::GetChild(const char* name) const {
	const auto& vec = GetData(rawData).vec;//TODO check type
	for (int i = 0; i < vec.size(); i++) {
		if (vec[i].name == name) {
			return vec[i].serializedObject;
		}
	}
	return SerializedObject();
}

//TODO return ref somehow

void SerializedObject::SetChild(const char* name, const SerializedObject& child) {
	type = Type::MAP;//TODO init vec
	auto& vec = GetData(rawData).vec;
	for (int i = 0; i < vec.size(); i++) {
		if (vec[i].name == name) {
			vec[i].serializedObject = child;
			return;
		}
	}
	vec.push_back(SerializedObjectWithName{ std::string(name), child });
}

SerializedObject SerializedObject::GetChild(int idx) const {
	const auto& vec = GetData(rawData).vec;//TODO check type
	if (idx < vec.size()) {
		return vec[idx].serializedObject;
	}
	return SerializedObject();
}

void SerializedObject::SetChild(int idx, const SerializedObject& child) {
	type = Type::ARRAY;//TODO init vec
	auto& vec = GetData(rawData).vec;
	//TODO inc size by 1 at a time
	if (idx >= vec.size()) {
		vec.resize(idx + 1);
	}
	vec[idx].serializedObject = child;
}

SerializedObject::Type SerializedObject::GetType() const {
	return type;
}
