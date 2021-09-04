#pragma once

#include <string>
#include <unordered_map>
#include "yaml-cpp/yaml.h" //TODO this is not the way
#include "Object.h"
#include "Component.h"
#include "ryml.hpp"

template<class T>
struct is_shared_ptr : std::false_type {};

template<class T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

template<class T>
struct is_std_vector : std::false_type {};

template<class T>
struct is_std_vector<std::vector<T>> : std::true_type {};

class AssetDatabase;

class Object;

class SE_CPP_API AssetDatabase_TextImporterHandle {
	friend class AssetDatabase;
public:
	const ryml::NodeRef& yaml;

	//TODO where T : Object
	template<typename T>
	void RequestObjectPtr(std::shared_ptr<T>& dest, const std::string& uid) {
		RequestObjectPtr((void*)&dest, uid);//TODO store type info as well(at least for debugging
	}
	void RequestObjectPtr(void* dest, const std::string& uid);

private:
	AssetDatabase_TextImporterHandle(AssetDatabase* database, const ryml::NodeRef yaml) :database(database), yaml(yaml) {}
	AssetDatabase* database = nullptr;
};

enum class SerializationContextType {
	Sequence,
	Map
};

class SE_CPP_API SerializationContext {
public:

	SerializationContext(ryml::NodeRef yamlNode, std::vector<std::shared_ptr<Object>> objectsAllowedToSerialize = std::vector<std::shared_ptr<Object>>{})
		:yamlNode(yamlNode)
		, objectsAllowedToSerialize(objectsAllowedToSerialize)
		, rootObjectsRequestedToSerialize(&objectsRequestedToSerialize)
		, rootObjectsRequestedToSerializeRequesters(&objectsRequestedToSerializeRequesters)
		, rootObjectsAllowedToSerialize(&(this->objectsAllowedToSerialize))
		, rootDeserializedObjects(&(this->deserializedObjects))
	{}

	SerializationContext(std::vector<std::shared_ptr<Object>> objectsAllowedToSerialize = std::vector<std::shared_ptr<Object>>{})
	{
		this->yamlTree = std::make_shared<ryml::Tree>();
		this->yamlNode = *yamlTree;
		this->objectsAllowedToSerialize = objectsAllowedToSerialize;
		this->rootObjectsRequestedToSerialize = (&objectsRequestedToSerialize);
		this->rootObjectsRequestedToSerializeRequesters = (&objectsRequestedToSerializeRequesters);
		this->rootObjectsAllowedToSerialize = (&(this->objectsAllowedToSerialize));
		this->rootDeserializedObjects = (&(this->deserializedObjects));
	}


	void SetType(SerializationContextType type) {
		switch (type)
		{
		case SerializationContextType::Sequence:
			yamlNode |= ryml::SEQ;
			break;
		case SerializationContextType::Map:
			yamlNode |= ryml::MAP;
			break;
		default:
			break;
		}
		//TODO
	}

	const bool IsDefined() const {
		return yamlNode.valid();
	}
	const SerializationContext Child(const std::string& name) const {
		if (!yamlNode.is_map()) {
			return SerializationContext(ryml::NodeRef(), *this);
		}
		else {
			return SerializationContext(yamlNode[c4::csubstr(name.c_str(), name.length())], *this);
		}
	}
	SerializationContext Child(const std::string& name) {
		yamlNode |= ryml::MAP;
		auto cname = c4::csubstr(name.c_str(), name.length());
		ryml::NodeRef child;
		if (yamlNode.has_child(cname)) {
			child = yamlNode[cname];
		}
		else {
			child = yamlNode.append_child();
			child.set_key_serialized(cname);
		}
		return SerializationContext(child, *this);
	}
	const SerializationContext Child(int iChild) const {
		if (yamlNode.num_children() > iChild) {
			return SerializationContext(yamlNode[iChild], *this);
		}
		else {
			return SerializationContext(ryml::NodeRef(), *this);
		}
	}
	SerializationContext Child(int iChild) {
		yamlNode |= ryml::SEQ;
		ryml::NodeRef child;
		if (yamlNode.num_children() > iChild) {
			child = yamlNode[iChild];
		}
		else {
			child = yamlNode.append_child();
			//TODO ASSERT(yamlNode.num_children() > iChild);
		}
		return SerializationContext(yamlNode[iChild], *this);
	}
	bool IsSequence()const {
		return yamlNode.is_seq();
	}
	bool IsMap()const {
		return yamlNode.is_map();
	}
	int Size()const {
		return yamlNode.num_children();
	}

	template<typename T>
	void operator<<(const T& t) {
		yamlNode << t;
	}
	template<typename T>
	void operator>>(T& t) const {
		yamlNode >> t;
	}


	//TODO only if T is Object
	template<typename T>
	void RequestDeserialization(std::shared_ptr<T>& ptr, const std::string& assetPath) const {
		RequestDeserialization((void*)&ptr, assetPath);//TODO typecheck
	}

	template<typename T>
	void RequestSerialization(std::shared_ptr<T>& ptr) const {
		auto casted = std::reinterpret_pointer_cast<Object>(ptr);
		if (std::find(rootObjectsRequestedToSerialize->begin(), rootObjectsRequestedToSerialize->end(), casted) == rootObjectsRequestedToSerialize->end()) {
			rootObjectsRequestedToSerialize->push_back(casted);
		}
		(*rootObjectsRequestedToSerializeRequesters)[casted].push_back(yamlNode);
	}

	void AddAllowedToSerializeObject(std::shared_ptr<Object> obj) {
		rootObjectsAllowedToSerialize->push_back(obj);
	}

	AssetDatabase* database = nullptr;
	AssetDatabase_TextImporterHandle* databaseHandle = nullptr;

	void FlushRequiestedToSerialize();
	void FinishDeserialization();

	std::vector<Object*>* rootDeserializedObjects = nullptr;//TODO make private

	ryml::NodeRef GetYamlNode() { return yamlNode; }

private:
	ryml::NodeRef yamlNode{};
	std::shared_ptr<ryml::Tree> yamlTree{};

	SerializationContext(ryml::NodeRef yamlNode, const SerializationContext& parent)
		:yamlNode(yamlNode)
		, databaseHandle(parent.databaseHandle)
		, rootObjectsRequestedToSerialize(parent.rootObjectsRequestedToSerialize)
		, rootObjectsAllowedToSerialize(parent.rootObjectsAllowedToSerialize)
		, rootDeserializedObjects(parent.rootDeserializedObjects)
		, rootObjectsRequestedToSerializeRequesters(parent.rootObjectsRequestedToSerializeRequesters) {}

	void RequestDeserialization(void* ptr, const std::string& assetPath) const;

	std::vector<std::shared_ptr<Object>>* rootObjectsRequestedToSerialize = nullptr;
	std::vector<std::shared_ptr<Object>> objectsRequestedToSerialize;
	std::vector<std::shared_ptr<Object>>* rootObjectsAllowedToSerialize = nullptr;
	std::vector<std::shared_ptr<Object>> objectsAllowedToSerialize;
	std::unordered_map<std::shared_ptr<Object>, std::vector<ryml::NodeRef>>* rootObjectsRequestedToSerializeRequesters = nullptr;
	std::unordered_map<std::shared_ptr<Object>, std::vector<ryml::NodeRef>> objectsRequestedToSerializeRequesters;
	std::vector<Object*> deserializedObjects;

};

class ReflectedTypeBase {
public:
	virtual void Serialize(SerializationContext& context, const void* object) = 0;
	virtual void Deserialize(const SerializationContext& context, void* object) = 0;
	const std::string& GetName() {
		return __name__;
	}
	bool HasTag(const std::string tag) {
		return std::find(tags.begin(), tags.end(), tag) != tags.end();
	}


	std::vector<std::string> tags;//TODO make protected

protected:
	ReflectedTypeBase(const std::string& name) : __name__(name) {}

	ReflectedTypeBase* __parentType__ = nullptr;
private:
	std::string __name__;
};


template<typename T>
class ReflectedTypeSimple : public ReflectedTypeBase {
public:
	ReflectedTypeSimple(std::string name) : ReflectedTypeBase(name) {

	}
	virtual void Serialize(SerializationContext& context, const void* object) override
	{
		context << *((T*)object);
	}
	virtual void Deserialize(const SerializationContext& context, void* object) override
	{
		if (context.IsDefined()) {
			context >> (*(T*)object);
		}
	}
};

class ReflectedTypeString : public ReflectedTypeBase {
public:
	ReflectedTypeString(std::string name) : ReflectedTypeBase(name) {

	}
	virtual void Serialize(SerializationContext& context, const void* object) override
	{
		auto& str = *((std::string*)object);
		auto csubstr = c4::csubstr(str.c_str(), str.length());
		context << csubstr;
	}
	virtual void Deserialize(const SerializationContext& context, void* object) override
	{
		if (context.IsDefined()) {

			c4::csubstr csubstr;
			context >> csubstr;
			auto& str = *((std::string*)object);
			str = std::string(csubstr.str, csubstr.len);
		}
	}
};
//TODO export to dll to make type objects single instanced
template<typename T>
inline std::enable_if_t<!is_shared_ptr<T>::value && !is_std_vector<T>::value, ReflectedTypeBase*>
GetReflectedType() {
	return T::TypeOf();
}

template<>
inline ReflectedTypeBase* GetReflectedType<int>() {
	static ReflectedTypeSimple<int> type("int");
	return &type;
}

template<>
inline ReflectedTypeBase* GetReflectedType<float>() {
	static ReflectedTypeSimple<float> type("float");
	return &type;
}

template<>
inline ReflectedTypeBase* GetReflectedType<bool>() {
	static ReflectedTypeSimple<bool> type("bool");
	return &type;
}

template<>
inline ReflectedTypeBase* GetReflectedType<std::string>() {
	static ReflectedTypeString type("string");
	return &type;
}


class ReflectedTypeSharedPtrBase : public ReflectedTypeBase {
public:
	ReflectedTypeSharedPtrBase(const std::string& name) : ReflectedTypeBase(name) {}
};

//TODO where T is Object
template<typename T>
class ReflectedTypeSharedPtr : public ReflectedTypeSharedPtrBase {
public:
	ReflectedTypeSharedPtr(const std::string& name) : ReflectedTypeSharedPtrBase(name) {}

	virtual void Serialize(SerializationContext& context, const void* object) override
	{
		std::shared_ptr<T>& ptr = *(std::shared_ptr<T>*)object;
		context.RequestSerialization(ptr);
	}
	virtual void Deserialize(const SerializationContext& context, void* object) override
	{
		std::shared_ptr<T>& ptr = *(std::shared_ptr<T>*)object;
		if (!context.IsDefined()) {
			//TODO (std::shared_ptr<T>*)(object)[0] = nullptr;
			return;
		}
		std::string path;
		::Deserialize(context, path);
		// TODO not here ?
		if (path.size() > 0) {
			context.RequestDeserialization(ptr, path);
		}
	}
};

template<typename T>
std::enable_if_t<is_shared_ptr<T>::value, ReflectedTypeBase*>
inline GetReflectedType() {
	static ReflectedTypeSharedPtr<T::element_type> type(std::string("shared_ptr<"));//TODOTODO +GetReflectedType<T::element_type>()->GetName() + ">");
	return &type;
}

class ReflectedTypeStdVectorBase : public ReflectedTypeBase {
public:
	ReflectedTypeStdVectorBase(const std::string& name) : ReflectedTypeBase(name) {}
	virtual ReflectedTypeBase* GetElementType() = 0;
	virtual int GetElementSizeOf() = 0;
};

template<typename T>
class ReflectedTypeStdVector : public ReflectedTypeStdVectorBase {
public:
	ReflectedTypeStdVector(const std::string& name) : ReflectedTypeStdVectorBase(name) {}
	virtual void Serialize(SerializationContext& context, const void* object) override {
		const std::vector<T>& t = *(std::vector<T>*)object;
		Serialize(context, t);
	}
	virtual void Deserialize(const SerializationContext& context, void* object) override {
		std::vector<T>& t = *(std::vector<T>*)object;
		Deserialize(context, t);
	}

	void Serialize(SerializationContext& context, const std::vector<T>& object) {
		context.SetType(SerializationContextType::Sequence);
		for (int i = 0; i < object.size(); i++) {
			auto child = context.Child(i);
			::Serialize(child, object[i]);
			if (!child.IsDefined())
			{
				//HACK to make sure yaml doesnt convert parent node into map
				//child.yamlNode = YAML::Node();//TODO
			}
		}
	}
	void Deserialize(const SerializationContext& context, std::vector<T>& object) {
		if (!context.IsDefined()) {
			return;
		}
		if (!context.IsSequence()) {
			//TODO error
			return;
		}
		object.clear();
		object.resize(context.Size());
		for (int i = 0; i < object.size(); i++) {
			::Deserialize(context.Child(i), object[i]);
		}
	}

	virtual ReflectedTypeBase* GetElementType() override {
		return ::GetReflectedType<T>();
	}
	virtual int GetElementSizeOf() override {
		return sizeof(T);
	}
};
template<typename T>
std::enable_if_t<is_std_vector<T>::value, ReflectedTypeBase*>
inline GetReflectedType() {
	static ReflectedTypeStdVector<T::value_type> type(std::string("vector<") + GetReflectedType<T::value_type>()->GetName() + ">");
	return &type;
}


template<typename T>
class ReflectedTypeCustom : public ReflectedTypeBase {
	typedef void(*serializeFunc)(SerializationContext&, const T&);
	typedef void(*deserializeFunc)(const SerializationContext&, T&);
public:
	ReflectedTypeCustom(const std::string& name, serializeFunc sf, deserializeFunc df) :
		ReflectedTypeBase(name),
		sf(sf), df(df) {

	}

	serializeFunc sf;
	deserializeFunc df;

	virtual void Serialize(SerializationContext& context, const void* object) override {
		const T& t = *(T*)object;
		sf(context, t);
	}
	virtual void Deserialize(const SerializationContext& context, void* object) override {
		T& t = *(T*)object;
		df(context, t);
	}

};

#define REFLECT_CUSTOM_EXT(typeName, Serialize, Deserialize) template<> inline ReflectedTypeBase * GetReflectedType<##typeName>() { \
	static ReflectedTypeCustom<##typeName> type(#typeName, Serialize, Deserialize); \
	return &type; \
}


class ReflectedField {
public:
	ReflectedField(ReflectedTypeBase* type,
		std::string name,
		size_t offset) :type(type), name(name), offset(offset) {
	}
	ReflectedTypeBase* type;
	std::string name;
	size_t offset;
};

template<typename T, typename U> constexpr size_t offsetOf(U T::* member)
{
	return (char*)&((T*)nullptr->*member) - (char*)nullptr;
}


class ReflectedTypeNonTemplated : public ReflectedTypeBase {
public:
	ReflectedTypeNonTemplated(std::string name) : ReflectedTypeBase(name) {}
	std::vector<ReflectedField> fields;
};

template<typename T>
class ReflectedType : public ReflectedTypeNonTemplated {
public:
	ReflectedType(std::string name) :ReflectedTypeNonTemplated(name) {

	}
	virtual void Serialize(SerializationContext& context, const void* object) override {
		const T& t = *(T*)object;
		Serialize(context, t);
	}
	virtual void Deserialize(const SerializationContext& context, void* object) override {
		T& t = *(T*)object;
		Deserialize(context, t);
	}

	void Serialize(SerializationContext& context, const T& object) {
		CallOnBeforeSerialize(context, object);
		context.SetType(SerializationContextType::Map);
		for (const auto& var : fields) {
			auto childContext = context.Child(var.name);
			var.type->Serialize(childContext, ((char*)&object) + var.offset);
		}
	}
	void Deserialize(const SerializationContext& context, T& object) {
		if (context.IsDefined()) {
			for (const auto& var : fields) {
				const auto child = context.Child(var.name);
				if (child.IsDefined()) {
					//TODO type check somewhere
					var.type->Deserialize(child, ((char*)&object) + var.offset);
				}
			}
		}
		CallOnAfterDeserialize(context, object);
	}

	template<typename T>
	std::enable_if_t<!std::is_assignable<Object, T>::value, void>
		CallOnAfterDeserialize(const SerializationContext& context, T& object) {
		//object.OnAfterDeserializeCallback(context);
	}

	template<typename T>
	std::enable_if_t<std::is_assignable<Object, T>::value, void>
		CallOnAfterDeserialize(const SerializationContext& context, T& object) {
		//TODO rename 
		//TODO not raw pointer ?
		context.rootDeserializedObjects->push_back(&object);
	}

	template<typename T>
	std::enable_if_t<!std::is_assignable<Object, T>::value, void>
		CallOnBeforeSerialize(SerializationContext& context, const T& object) {
		//object.OnBeforeSerializeCallback(context);
	}

	template<typename T>
	std::enable_if_t<std::is_assignable<Object, T>::value, void>
		CallOnBeforeSerialize(SerializationContext& context, const T& object) {
		object.OnBeforeSerializeCallback(context);
	}
};


template<typename T>
std::enable_if_t<!std::is_assignable<Component, T>::value, int>
inline AddComponentTags(ReflectedTypeBase* type) {
	return -1;
}

template<typename T>
std::enable_if_t<std::is_assignable<Component, T>::value, int>
inline AddComponentTags(ReflectedTypeBase* type) {
	if (typeid(&T::Update) != typeid(&Component::Update)) { type->tags.push_back("HasUpdate"); }
	if (typeid(&T::FixedUpdate) != typeid(&Component::FixedUpdate)) { type->tags.push_back("HasFixedUpdate"); }
	return -1;
}

#define _REFLECT_BEGIN_NO_PARENT_(className) \
public:\
class Type : public ReflectedType<##className> { \
	using TYPE = ##className; \
	public:\
	Type() : ReflectedType<##className>(#className) {\
		AddComponentTags<##className>(this);

#define REFLECT_VAR(varName)\
		fields.push_back(ReflectedField(GetReflectedType<decltype(varName)>(), #varName, offsetOf(&TYPE::varName))); \

#define REFLECT_END() \
		} \
	}; \
	static Type* TypeOf() {\
		static Type t;\
		return &t;\
	}\
	ReflectedTypeBase* GetType() const{ \
		return TypeOf(); \
	}



#define _REFLECT_BEGIN_WITH_PARENT_(className, parentClassName) \
_REFLECT_BEGIN_NO_PARENT_(className) \
	__parentType__ = parentClassName::TypeOf(); \
	for(auto parentField : dynamic_cast<ReflectedTypeNonTemplated*>(__parentType__)->fields){\
		this->fields.push_back(parentField);\
	} \
	for (auto parentTag : dynamic_cast<ReflectedTypeNonTemplated*>(__parentType__)->tags) {	\
		this->tags.push_back(parentTag); \
	}

#define _EXPAND_(x) x

#define _GET_REFLECT_MACRO_(_1,_2,NAME,...) NAME

#define REFLECT_BEGIN(...) _EXPAND_(_GET_REFLECT_MACRO_(__VA_ARGS__, _REFLECT_BEGIN_WITH_PARENT_, _REFLECT_BEGIN_NO_PARENT_) (__VA_ARGS__))

#define REFLECT_CUSTOM(className, Serialize, Deserialize) \
public:\
	static ReflectedTypeBase* TypeOf() {\
		static ReflectedTypeCustom<##className> type(#className, Serialize, Deserialize); \
		return &type;\
	}\
	ReflectedTypeBase* GetType() const{ \
		return TypeOf(); \
	}

template<typename T>
void Deserialize(const SerializationContext& context, T& t) {
	GetReflectedType<T>()->Deserialize(context, &t);
}

template<typename T>
std::enable_if_t<!std::is_assignable<Object, T>::value, ReflectedTypeBase*>
inline GetReflectedType(const T* t) {
	return GetReflectedType<T>();
}
template<typename T>
std::enable_if_t<std::is_assignable<Object, T>::value, ReflectedTypeBase*>
GetReflectedType(const T* t) {
	return t->GetType();
}

template<typename T>
void Serialize(SerializationContext& context, const T& t) {
	GetReflectedType(&t)->Serialize(context, &t);
}