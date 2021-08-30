#pragma once

#include <string>
#include <unordered_map>
#include "yaml-cpp/yaml.h" //TODO this is not the way
#include "Object.h"

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

class SE_CPP_API SerializationContext {
public:
	SerializationContext(YAML::Node yamlNode, std::vector<std::shared_ptr<Object>> objectsAllowedToSerialize = std::vector<std::shared_ptr<Object>>{})
		:yamlNode(yamlNode)
		, objectsAllowedToSerialize(objectsAllowedToSerialize)
		, rootObjectsRequestedToSerialize(&objectsRequestedToSerialize)
		, rootObjectsRequestedToSerializeRequesters(&objectsRequestedToSerializeRequesters)
		, rootObjectsAllowedToSerialize(&(this->objectsAllowedToSerialize))
	{}
	//SerializationContext(const TextAsset& textAsset) :yamlNode(textAsset.GetYamlNode()) {}

	const bool IsDefined() const {
		return yamlNode.IsDefined();
	}
	const SerializationContext Child(const std::string& name) const {
		return SerializationContext(yamlNode[name], *this);
	}
	SerializationContext Child(const std::string& name) {
		return SerializationContext(yamlNode[name], *this);
	}
	const SerializationContext Child(int iChild) const {
		return SerializationContext(yamlNode[iChild], *this);
	}
	SerializationContext Child(int iChild) {
		return SerializationContext(yamlNode[iChild], *this);
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
	YAML::Node yamlNode{};

	void FlushRequiestedToSerialize();

private:
	SerializationContext(YAML::Node yamlNode, const SerializationContext& parent)
		:yamlNode(yamlNode)
		, rootObjectsRequestedToSerialize(parent.rootObjectsRequestedToSerialize)
		, rootObjectsAllowedToSerialize(parent.rootObjectsAllowedToSerialize)
		, rootObjectsRequestedToSerializeRequesters(parent.rootObjectsRequestedToSerializeRequesters) {}

	void RequestDeserialization(void* ptr, const std::string& assetPath) const;
	std::vector<std::shared_ptr<Object>>* rootObjectsRequestedToSerialize = nullptr;
	std::vector<std::shared_ptr<Object>> objectsRequestedToSerialize;
	std::vector<std::shared_ptr<Object>>* rootObjectsAllowedToSerialize = nullptr;
	std::vector<std::shared_ptr<Object>> objectsAllowedToSerialize;
	std::unordered_map<std::shared_ptr<Object>, std::vector<YAML::Node>>* rootObjectsRequestedToSerializeRequesters = nullptr;
	std::unordered_map<std::shared_ptr<Object>, std::vector<YAML::Node>> objectsRequestedToSerializeRequesters;
};

class ReflectedTypeBase {
public:
	virtual void Serialize(SerializationContext& context, const void* object) = 0;
	virtual void Deserialize(const SerializationContext& context, void* object) = 0;
	const std::string& GetName() {
		return __name__;
	}

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
		context.yamlNode = *((T*)object);
	}
	virtual void Deserialize(const SerializationContext& context, void* object) override
	{
		if (context.yamlNode.IsDefined()) {
			(*(T*)object) = context.yamlNode.as<T>();
		}
	}
};

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
	static ReflectedTypeSimple<std::string> type("string");
	return &type;
}


template<typename T>
class ReflectedTypeSharedPtr : public ReflectedTypeBase {
public:
	ReflectedTypeSharedPtr(const std::string& name) : ReflectedTypeBase(name) {}

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
		std::string path = context.yamlNode.as<std::string>();
		context.RequestDeserialization(ptr, path);
	}
};

template<typename T>
std::enable_if_t<is_shared_ptr<T>::value, ReflectedTypeBase*>
inline GetReflectedType() {
	static ReflectedTypeSharedPtr<T::element_type> type(std::string("shared_ptr<"));//TODOTODO +GetReflectedType<T::element_type>()->GetName() + ">");
	return &type;
}

template<typename T>
class ReflectedTypeStdVector : public ReflectedTypeBase {
public:
	ReflectedTypeStdVector(const std::string& name) : ReflectedTypeBase(name) {}
	virtual void Serialize(SerializationContext& context, const void* object) override {
		const std::vector<T>& t = *(std::vector<T>*)object;
		Serialize(context, t);
	}
	virtual void Deserialize(const SerializationContext& context, void* object) override {
		std::vector<T>& t = *(std::vector<T>*)object;
		Deserialize(context, t);
	}

	void Serialize(SerializationContext& context, const std::vector<T>& object) {
		context.yamlNode = YAML::Node(YAML::NodeType::Sequence);
		for (int i = 0; i < object.size(); i++) {
			auto child = context.Child(i);
			::Serialize(child, object[i]);
			if (!child.IsDefined())
			{
				//HACK to make sure yaml doesnt convert parent node into map
				child.yamlNode = YAML::Node();
			}
		}
	}
	void Deserialize(const SerializationContext& context, std::vector<T>& object) {
		if (!context.IsDefined()) {
			return;
		}
		if (!context.yamlNode.IsSequence()) {
			//TODO error
			return;
		}
		object.clear();
		object.resize(context.yamlNode.size());
		for (int i = 0; i < object.size(); i++) {
			::Deserialize(context.Child(i), object[i]);
		}
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
		context.yamlNode = YAML::Node(YAML::NodeType::Map);
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
		object.OnAfterDeserializeCallback(context);
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


#define _REFLECT_BEGIN_NO_PARENT_(className) \
public:\
class Type : public ReflectedType<##className> { \
	using TYPE = ##className; \
	public:\
	Type() : ReflectedType<##className>(#className) {\

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