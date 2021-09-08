#pragma once

#include <memory>

class Object;

class SerializedObject {
public:
	enum class Type {
		NO_VALUE,
		FLOAT,
		INT,
		STRING,
		OBJECT,
		MAP,
		ARRAY
	};

	float GetFloat() const;
	void SetFloat(const float& f);

	float GetInt() const;
	void SetInt(const int& i);

	const char* GetString() const;
	void SetString(const char* str);

	std::shared_ptr<Object> GetObject() const;
	void SetObject(const std::shared_ptr<Object>& object);

	SerializedObject GetChild(const char* name) const;//TODO return ref somehow
	void SetChild(const char* name, const SerializedObject& child);

	SerializedObject GetChild(int idx) const;//TODO return ref somehow
	void SetChild(int idx, const SerializedObject& child);

	Type GetType()const;

	class RawData {
		char data[96];
	};
private:

	Type type;
	RawData rawData;
};