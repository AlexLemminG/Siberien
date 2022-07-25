#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include "Object.h"
#include "Component.h"
#include "magic_enum.hpp"

template <class T>
struct is_shared_ptr : std::false_type {};

template <class T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

template <class T>
struct is_std_vector : std::false_type {};

template <class T>
struct is_std_vector<std::vector<T>> : std::true_type {};

class AssetDatabase;

class Object;

class RymlNodeRefDummy {
    char dummy[32];
};

class Attribute {
   public:
    Attribute() {}
    virtual ~Attribute() {}
};

namespace c4 {
namespace yml {
class Tree;
class NodeRef;
}  // namespace yml
}  // namespace c4
namespace ryml {
using namespace c4::yml;
using namespace c4;
}  // namespace ryml

class ExecuteInEditModeAttribute : public Attribute {
   public:
    ExecuteInEditModeAttribute() : Attribute() {}
    virtual ~ExecuteInEditModeAttribute() {}
};

class SE_CPP_API AssetDatabase_TextImporterHandle {
    friend class AssetDatabase;

   public:
    const ryml::NodeRef& yaml;

    // TODO where T : Object
    template <typename T>
    void RequestObjectPtr(std::shared_ptr<T>& dest, const std::string& uid) {
        RequestObjectPtr((void*)&dest, uid);  // TODO store type info as well(at least for debugging
    }
    void RequestObjectPtr(void* dest, const std::string& uid);

   private:
    AssetDatabase_TextImporterHandle(AssetDatabase* database, const ryml::NodeRef& yaml) : database(database), yaml(yaml) {}
    AssetDatabase* database = nullptr;
};

enum class SerializationContextType {
    Sequence,
    Map
};

class SE_CPP_API SerializationContext {
   public:
    SerializationContext(const ryml::NodeRef& yamlNode, std::vector<std::shared_ptr<Object>> objectsAllowedToSerialize = std::vector<std::shared_ptr<Object>>{});

    SerializationContext(std::vector<std::shared_ptr<Object>> objectsAllowedToSerialize = std::vector<std::shared_ptr<Object>>{});

    void SetType(SerializationContextType type);

    const bool IsDefined() const;
    const SerializationContext Child(const std::string& name) const;
    SerializationContext Child(const std::string& name);
    const SerializationContext Child(int iChild) const;
    SerializationContext Child(int iChild);
    bool IsSequence() const;
    bool IsMap() const;
    int Size() const;

    std::vector<std::string> GetChildrenNames() const;  // TODO optimize

    void Clear();       // removes node completely
    void ClearValue();  // leaves key and sets value to empty

    void operator<<(const int& t);
    void operator>>(int& t) const;

    void operator<<(const long& t);
    void operator>>(long& t) const;

    void operator<<(const uint64_t& t);
    void operator>>(uint64_t& t) const;

    void operator<<(const unsigned int& t);
    void operator>>(unsigned int& t) const;

    void operator<<(const float& t);
    void operator>>(float& t) const;

    void operator<<(const bool& t);
    void operator>>(bool& t) const;

    void operator<<(const std::string& t);
    void operator>>(std::string& t) const;

    // TODO only if T is Object
    template <typename T>
    void RequestDeserialization(std::shared_ptr<T>& ptr, const std::string& assetPath) const {
        RequestDeserialization((void*)&ptr, assetPath);  // TODO typecheck
    }

    template <typename T>
    void RequestSerialization(std::shared_ptr<T>& ptr) const {
        auto casted = std::reinterpret_pointer_cast<Object>(ptr);
        if (std::find(rootObjectsRequestedToSerialize->begin(), rootObjectsRequestedToSerialize->end(), casted) == rootObjectsRequestedToSerialize->end()) {
            rootObjectsRequestedToSerialize->push_back(casted);
        }
        (*rootObjectsRequestedToSerializeRequesters)[casted].push_back(yamlNode);
    }

    void AddAllowedToSerializeObject(std::shared_ptr<Object> obj);

    AssetDatabase* database = nullptr;
    AssetDatabase_TextImporterHandle* databaseHandle = nullptr;

    void FlushRequiestedToSerialize();
    void FinishDeserialization();

    std::vector<Object*>* rootDeserializedObjects = nullptr;  // TODO make private

    ryml::NodeRef& GetYamlNode();
    const ryml::NodeRef& GetYamlNode() const;

    bool isLua = false;

   private:
    RymlNodeRefDummy yamlNode{};
    std::shared_ptr<ryml::Tree> yamlTree{};

    SerializationContext(ryml::NodeRef yamlNode, const SerializationContext& parent);

    void RequestDeserialization(void* ptr, const std::string& assetPath) const;

    std::vector<std::shared_ptr<Object>>* rootObjectsRequestedToSerialize = nullptr;
    std::vector<std::shared_ptr<Object>> objectsRequestedToSerialize;
    std::vector<std::shared_ptr<Object>>* rootObjectsAllowedToSerialize = nullptr;
    std::vector<std::shared_ptr<Object>> objectsAllowedToSerialize;
    std::unordered_map<std::shared_ptr<Object>, std::vector<RymlNodeRefDummy>>* rootObjectsRequestedToSerializeRequesters = nullptr;
    std::unordered_map<std::shared_ptr<Object>, std::vector<RymlNodeRefDummy>> objectsRequestedToSerializeRequesters;
    std::vector<Object*> deserializedObjects;
};

class ReflectedTypeBase;

class ReflectedMethod {
   public:
    std::string name;
    ReflectedTypeBase* returnType = nullptr;
    ReflectedTypeBase* objectType = nullptr;
    std::vector<ReflectedTypeBase*> argTypes;

    std::function<void(void*, std::vector<void*>, void*)> func;  // obj, args, returnval

    void Invoke(void* obj, std::vector<void*> args, void* result) {
        func(obj, args, result);
    }

    std::string ToString() const;
};

// TODO hide
template <class, class, class...>
struct types { using type = types; };
template <class Sig>
struct args;

template <class R, class ClassT, class... Args>
struct args<R (ClassT::*)(Args...)> : types<R, ClassT, Args...> {};

template <class R, class ClassT, class... Args>
struct args<R (ClassT::*)(Args...) const> : types<R, ClassT, Args...> {};

template <class R, class... Args>
struct args<R (*)(Args...)> : types<R, void, Args...> {};

template <class Sig>
using args_t = typename args<Sig>::type;

template <class T, class... Params>
void GenerateMethodBindingArgs(ReflectedMethod* method) {
    method->argTypes.push_back(GetReflectedType<std::decay_t<T>>());
    if constexpr (sizeof...(Params) > 0) {
        GenerateMethodBindingArgs<Params...>(method);
    }
}
template <bool isClassMethodCall, int N = 0, class... Params>
void FillParams(std::tuple<Params...>& params, const std::vector<void*>& paramsRaw) {
    std::get<N + (isClassMethodCall ? 1 : 0)>(params) = *((std::remove_reference_t<decltype(std::get<N + (isClassMethodCall ? 1 : 0)>(params))>*)paramsRaw[N]);
    if constexpr (N + (isClassMethodCall ? 1 : 0) + 1 < sizeof...(Params)) {
        FillParams<isClassMethodCall, N + 1>(params, paramsRaw);
    }
}
template <bool isClassMethodCall, class FirstParam, class... Params>
auto FillParams2(std::vector<void*>& paramsRaw, int nextParam = 0) {
    auto* ptr = reinterpret_cast<std::decay_t<FirstParam>*>(paramsRaw[nextParam]);
    
    //if constexpr (std::is_reference<FirstParam>::value) {
        auto tuple = std::tie(*ptr);

        if constexpr (sizeof...(Params) > 0) {
            return std::tuple_cat(tuple, FillParams2<isClassMethodCall, Params...>(paramsRaw, nextParam + 1));
        }
        else {
            return tuple;
        }
}

template <typename... Ts>
constexpr auto decay_types(std::tuple<Ts...> const&)
    -> std::tuple<std::remove_cv_t<std::remove_reference_t<Ts>>...>;
template <typename T>
using decay_tuple = decltype(decay_types(std::declval<T>()));

template <class ReturnType, class ObjectType, class... Params>
ReflectedMethod GenerateMethodBindingNoFunc(const std::string& name, types<ReturnType, ObjectType, Params...>) {
    ReflectedMethod method;
    method.name = name;
    if constexpr (std::is_same<void, ReturnType>()) {
        method.returnType = nullptr;
    } else {
        method.returnType = GetReflectedType<std::decay_t<ReturnType>>();
    }
    method.objectType = GetReflectedType<ObjectType>();
    if constexpr (sizeof...(Params) > 0) {
        GenerateMethodBindingArgs<Params...>(&method);
    }
    return method;
}

template <class ReturnType, class... Params>
ReflectedMethod GenerateMethodBinding(const std::string& name, types<ReturnType, void, Params...> types, ReturnType (*func)(Params...)) {
    return GenerateMethodBinding(name, types, std::function<ReturnType(Params...)>(func));
}
// TODO less duplication
template <class ReturnType, class... Params>
ReflectedMethod GenerateMethodBinding(const std::string& name, types<ReturnType, void, Params...> types, const std::function<ReturnType(Params...)>& func) {
    auto method = GenerateMethodBindingNoFunc(name, types);
    method.func = [=](void* objRaw, std::vector<void*> argsRaw, void* returnRaw) {
        // void* objRaw;
        // std::vector<void*> argsRaw;
        // void* returnRaw;
        if constexpr (sizeof...(Params) > 0) {
            //decay_tuple<std::tuple<Params...>> params;
            std::tuple<Params...> params = FillParams2<false, Params...>(argsRaw, 0);
            //FillParams<false>(params, argsRaw);
            if constexpr (std::is_void<ReturnType>()) {
                std::apply(func, params);
            } else {
                *((ReturnType*)returnRaw) = std::apply(func, params);
            }
        } else {
            if constexpr (std::is_void<ReturnType>()) {
                std::invoke(func, obj);
            } else {
                *((ReturnType*)returnRaw) = std::invoke(func);
            }
        }
    };
    return method;
}

template <class ReturnType, class ObjectType, class... Params>
ReflectedMethod GenerateMethodBinding(const std::string& name, types<ReturnType, ObjectType, Params...> types, ReturnType (ObjectType::*func)(Params...)) {
    auto method = GenerateMethodBindingNoFunc(name, types);
    method.func = [=](void* objRaw, std::vector<void*> argsRaw, void* returnRaw) {
        // void* objRaw;
        // std::vector<void*> argsRaw;
        // void* returnRaw;
        ObjectType* obj = (ObjectType*)objRaw;
        if constexpr (sizeof...(Params) > 0) {
            decay_tuple<std::tuple<ObjectType*, Params...>> params;
            std::get<0>(params) = obj;
            FillParams<true>(params, argsRaw);
            if constexpr (std::is_void<ReturnType>()) {
                std::apply(func, params);
            } else {
                *((ReturnType*)returnRaw) = std::apply(func, params);
            }
        } else {
            if constexpr (std::is_void<ReturnType>()) {
                std::invoke(func, obj);
            } else {
                *((ReturnType*)returnRaw) = std::invoke(func, obj);
            }
        }
    };
    return method;
}

template <class ReturnType, class ObjectType, class... Params>
ReflectedMethod GenerateMethodBinding(const std::string& name, types<ReturnType, ObjectType, Params...> types, ReturnType (ObjectType::*func)(Params...) const) {
    auto method = GenerateMethodBindingNoFunc(name, types);
    method.func = [=](void* objRaw, std::vector<void*> argsRaw, void* returnRaw) {
        // void* objRaw;
        // std::vector<void*> argsRaw;
        // void* returnRaw;
        ObjectType* obj = (ObjectType*)objRaw;
        if constexpr (sizeof...(Params) > 0) {
            decay_tuple<std::tuple<ObjectType*, Params...>> params;
            std::get<0>(params) = obj;
            FillParams<true>(params, argsRaw);
            if constexpr (std::is_void<ReturnType>()) {
                std::apply(func, params);
            } else {
                *((ReturnType*)returnRaw) = std::apply(func, params);
            }
        } else {
            if constexpr (std::is_void<ReturnType>()) {
                std::invoke(func, obj);
            } else {
                *((std::decay_t<ReturnType>*)returnRaw) = static_cast<std::decay_t<ReturnType>>(std::invoke(func, obj));
            }
        }
    };
    return method;
}

class ReflectedField {
   public:
    ReflectedField(ReflectedTypeBase* type,
                   std::string name,
                   size_t offset) : type(type), name(name), offset(offset) {
    }
    virtual void* GetPtr(void* baseObject) const { return (char*)baseObject + offset; }
    virtual const void* GetPtr(const void* baseObject) const { return (char*)baseObject + offset; }
    ReflectedTypeBase* type;
    std::string name;
    size_t offset;
};

class ReflectedTypeBase {
   public:
    virtual void Construct(void* ptr) const {}  // = 0;//TODO destruct
    virtual void Serialize(SerializationContext& context, const void* object) const = 0;
    virtual void Deserialize(const SerializationContext& context, void* object) const = 0;
    virtual size_t SizeOf() const = 0;

    const std::string& GetName() const {
        return __name__;
    }
    bool HasTag(const std::string tag) const {
        return std::find(tags.begin(), tags.end(), tag) != tags.end();
    }

    ReflectedTypeBase* GetParentType() const { return __parentType__; }

    const std::vector<ReflectedMethod>& GetMethods() const { return __methods__; };

    std::vector<std::string> tags;  // TODO make protected
    std::vector<std::shared_ptr<Attribute>> attributes;
    std::vector<ReflectedField> fields;

   protected:
    ReflectedTypeBase(const std::string& name) : __name__(name) {}

    ReflectedTypeBase* __parentType__ = nullptr;

    std::vector<ReflectedMethod> __methods__;

   private:
    std::string __name__;
};

template <typename T>
class ReflectedTypeSimple : public ReflectedTypeBase {
   public:
    ReflectedTypeSimple(std::string name) : ReflectedTypeBase(name) {
    }

    virtual void Construct(void* ptr) const override {
        new (ptr)(T);
    }
    virtual void Serialize(SerializationContext& context, const void* object) const override {
        context << *((T*)object);
    }
    virtual void Deserialize(const SerializationContext& context, void* object) const override {
        if (context.IsDefined()) {
            context >> (*(T*)object);
        }
    }
    virtual size_t SizeOf() const override {
        return sizeof(T);
    }
};

class SE_CPP_API ReflectedTypeString : public ReflectedTypeBase {
   public:
    ReflectedTypeString(std::string name) : ReflectedTypeBase(name) {
    }
    virtual void Construct(void* ptr) const override;
    virtual void Serialize(SerializationContext& context, const void* object) const override;
    virtual void Deserialize(const SerializationContext& context, void* object) const override;
    virtual size_t SizeOf() const override { return sizeof(std::string); }
};
// TODO export to dll to make type objects single instanced
template <typename T>
inline std::enable_if_t<!is_shared_ptr<T>::value && !is_std_vector<T>::value, ReflectedTypeBase*>
GetReflectedType() {
    return T::TypeOf();
}

class ReflectedTypeEnumBase : public ReflectedTypeBase {
   public:
    ReflectedTypeEnumBase(std::string name) : ReflectedTypeBase(name) {}
    virtual const std::vector<std::pair<uint64_t, std::string>>& GetEnumEntries() = 0;
    virtual const bool IsFlags() = 0;
};

template <typename T, bool is_flags>
class ReflectedTypeEnum : public ReflectedTypeEnumBase {
   public:
    ReflectedTypeEnum(std::string name) : ReflectedTypeEnumBase(name) {
    }
    virtual void Serialize(SerializationContext& context, const void* object) const override {
        context << *((uint64_t*)(T*)object);
    }
    virtual void Deserialize(const SerializationContext& context, void* object) const override {
        if (context.IsDefined()) {
            context >> (*(uint64_t*)(T*)object);
        }
    }
    virtual void Construct(void* ptr) const override {
        new (ptr)(T);
    }
    virtual size_t SizeOf() const override { return sizeof(T); }

    virtual const std::vector<std::pair<uint64_t, std::string>>& GetEnumEntries() override {
        static std::vector<std::pair<uint64_t, std::string>> result;
        if (result.size() == 0) {
            for (auto e : magic_enum::enum_entries<T>()) {
                result.push_back(std::make_pair(uint64_t(e.first), std::string(e.second)));
            }
        }
        return result;
    }
    virtual const bool IsFlags() override { return is_flags; };
};

class SE_CPP_API ReflectedTypeVoid : public ReflectedTypeBase {
   public:
    ReflectedTypeVoid(std::string name) : ReflectedTypeBase(name) {}
    // TODO warnings ?
    virtual void Construct(void* ptr) const override { return; }
    virtual void Serialize(SerializationContext& context, const void* object) const override {}
    virtual void Deserialize(const SerializationContext& context, void* object) const override {}
    virtual size_t SizeOf() const override { return 0; }
};

template <>
inline ReflectedTypeBase* GetReflectedType<void>() {
    static ReflectedTypeVoid type("void");
    return &type;
}

template <>
inline ReflectedTypeBase* GetReflectedType<int>() {
    static ReflectedTypeSimple<int> type("int");
    return &type;
}

template <>
inline ReflectedTypeBase* GetReflectedType<uint64_t>() {
    static ReflectedTypeSimple<uint64_t> type("uint64_t");
    return &type;
}

template <>
inline ReflectedTypeBase* GetReflectedType<long>() {
    static ReflectedTypeSimple<long> type("long");
    return &type;
}

template <>
inline ReflectedTypeBase* GetReflectedType<float>() {
    static ReflectedTypeSimple<float> type("float");
    return &type;
}

template <>
inline ReflectedTypeBase* GetReflectedType<bool>() {
    static ReflectedTypeSimple<bool> type("bool");
    return &type;
}

template <>
inline ReflectedTypeBase* GetReflectedType<std::string>() {
    static ReflectedTypeString type("string");
    return &type;
}

class ReflectedTypeSharedPtrBase : public ReflectedTypeBase {
   public:
    ReflectedTypeSharedPtrBase(const std::string& name) : ReflectedTypeBase(name) {}
};

// TODO where T is Object
template <typename T>
class ReflectedTypeSharedPtr : public ReflectedTypeSharedPtrBase {
   public:
    ReflectedTypeSharedPtr(const std::string& name) : ReflectedTypeSharedPtrBase(name) {}

    virtual void Serialize(SerializationContext& context, const void* object) const override {
        std::shared_ptr<T>& ptr = *(std::shared_ptr<T>*)object;
        context.RequestSerialization(ptr);
    }
    virtual void Deserialize(const SerializationContext& context, void* object) const override {
        std::shared_ptr<T>& ptr = *(std::shared_ptr<T>*)object;
        if (!context.IsDefined()) {
            // TODO (std::shared_ptr<T>*)(object)[0] = nullptr;
            return;
        }
        std::string path;
        ::Deserialize(context, path);
        // TODO not here ?
        if (path.size() > 0) {
            context.RequestDeserialization(ptr, path);
        }
    }
    virtual size_t SizeOf() const override { return sizeof(std::shared_ptr<T>); }
    virtual void Construct(void* ptr) const override {
        new (ptr)(std::shared_ptr<T>);
    }
};

template <typename T>
std::enable_if_t<is_shared_ptr<T>::value, ReflectedTypeBase*> inline GetReflectedType() {
    static ReflectedTypeSharedPtr<T::element_type> type(std::string("shared_ptr<") /* + TODO */ + ">");
    return &type;
}

class ReflectedTypeStdVectorBase : public ReflectedTypeBase {
   public:
    ReflectedTypeStdVectorBase(const std::string& name) : ReflectedTypeBase(name) {}
    virtual ReflectedTypeBase* GetElementType() = 0;
    virtual int GetElementSizeOf() = 0;
};

template <typename T>
class ReflectedTypeStdVector : public ReflectedTypeStdVectorBase {
   public:
    ReflectedTypeStdVector(const std::string& name) : ReflectedTypeStdVectorBase(name) {}
    virtual void Serialize(SerializationContext& context, const void* object) const override {
        const std::vector<T>& t = *(std::vector<T>*)object;
        Serialize(context, t);
    }
    virtual void Deserialize(const SerializationContext& context, void* object) const override {
        std::vector<T>& t = *(std::vector<T>*)object;
        Deserialize(context, t);
    }

    void Serialize(SerializationContext& context, const std::vector<T>& object) const {
        context.SetType(SerializationContextType::Sequence);
        for (int i = 0; i < object.size(); i++) {
            auto child = context.Child(i);
            ::Serialize(child, object[i]);
            if (!child.IsDefined()) {
                // HACK to make sure yaml doesnt convert parent node into map
                // child.yamlNode = YAML::Node();//TODO
            }
        }
    }
    void Deserialize(const SerializationContext& context, std::vector<T>& object) const {
        if (!context.IsDefined()) {
            return;
        }
        if (!context.IsSequence()) {
            // TODO error
            return;
        }
        object.clear();
        object.resize(context.Size());
        for (int i = 0; i < object.size(); i++) {
            ::Deserialize(context.Child(i), object[i]);
        }
    }

    virtual void Construct(void* ptr) const override {
        new (ptr)(std::vector<T>);
    }

    virtual size_t SizeOf() const override { return sizeof(std::vector<T>); }

    virtual ReflectedTypeBase* GetElementType() override {
        return ::GetReflectedType<T>();
    }
    virtual int GetElementSizeOf() override {
        return sizeof(T);
    }
};
template <typename T>
std::enable_if_t<is_std_vector<T>::value, ReflectedTypeBase*> inline GetReflectedType() {
    static ReflectedTypeStdVector<T::value_type> type(std::string("vector<") + GetReflectedType<T::value_type>()->GetName() + ">");
    return &type;
}

template <typename T>
class ReflectedTypeCustom : public ReflectedTypeBase {
   public:
    typedef void (*serializeFunc)(SerializationContext&, const T&);
    typedef void (*deserializeFunc)(const SerializationContext&, T&);
    ReflectedTypeCustom(const std::string& name, serializeFunc sf, deserializeFunc df) : ReflectedTypeBase(name),
                                                                                         sf(sf),
                                                                                         df(df) {
    }

    serializeFunc sf;
    deserializeFunc df;

    virtual void Construct(void* ptr) const override {
        new (ptr)(T);
    }
    virtual void Serialize(SerializationContext& context, const void* object) const override {
        const T& t = *(T*)object;
        sf(context, t);
    }
    virtual void Deserialize(const SerializationContext& context, void* object) const override {
        T& t = *(T*)object;
        df(context, t);
    }
    virtual size_t SizeOf() const override { return sizeof(T); }
};

#define REFLECT_CUSTOM_EXT_BEGIN(typeName, Serialize, Deserialize)                \
    template <>                                                                   \
    inline ReflectedTypeBase* GetReflectedType<##typeName>() {                    \
        using TYPE = ##typeName;                                                  \
        class ReflectedTypeCustom_ext : public ReflectedTypeCustom<TYPE> {        \
           public:                                                                \
            std::vector<std::function<ReflectedMethod(void)>> methodInitializers; \
            ReflectedTypeCustom_ext() : ReflectedTypeCustom(#typeName, Serialize, Deserialize) {
#define REFLECT_CUSTOM_EXT_END()                       \
    }                                                  \
    void Init() {                                      \
        for (auto& initializer : methodInitializers) { \
            __methods__.push_back(initializer());      \
        }                                              \
        methodInitializers.clear();                    \
    }                                                  \
    }                                                  \
    ;                                                  \
    static ReflectedTypeCustom_ext type;               \
    static bool isInited = false;                      \
    if (!isInited) {                                   \
        isInited = true;                               \
        type.Init();                                   \
    }                                                  \
    return &type;                                      \
    }

#define REFLECT_CUSTOM_EXT(typeName, Serialize, Deserialize) REFLECT_CUSTOM_EXT_BEGIN(typeName, Serialize, Deserialize) REFLECT_CUSTOM_EXT_END()

template <typename T, typename U>
constexpr size_t offsetOf(U T::*member) {
    return (char*)&((T*)nullptr->*member) - (char*)nullptr;
}

class ReflectedTypeNonTemplated : public ReflectedTypeBase {
   public:
    ReflectedTypeNonTemplated(std::string name) : ReflectedTypeBase(name) {}
};

template <typename T>
class ReflectedType : public ReflectedTypeNonTemplated {
   public:
    ReflectedType(std::string name) : ReflectedTypeNonTemplated(name) {
    }
    virtual void Serialize(SerializationContext& context, const void* object) const override {
        const T& t = *(T*)object;
        Serialize(context, t);
    }
    virtual void Deserialize(const SerializationContext& context, void* object) const override {
        T& t = *(T*)object;
        Deserialize(context, t);
    }

    void Serialize(SerializationContext& context, const T& object) const {
        CallOnBeforeSerialize(context, object);
        context.SetType(SerializationContextType::Map);
        for (const auto& var : fields) {
            auto childContext = context.Child(var.name);
            var.type->Serialize(childContext, ((char*)&object) + var.offset);
        }
    }
    void Deserialize(const SerializationContext& context, T& object) const {
        if (context.IsDefined()) {
            for (const auto& var : fields) {
                const auto child = context.Child(var.name);
                if (child.IsDefined()) {
                    // TODO type check somewhere
                    var.type->Deserialize(child, ((char*)&object) + var.offset);
                }
            }
        }
        CallOnAfterDeserialize(context, object);
    }

    virtual void Construct(void* ptr) const override {
        if constexpr (std::is_default_constructible<T>()) {
            new (ptr)(T);
        } else {
            static_assert(std::is_abstract<T>());  // TODO message
        }
    }
    virtual size_t SizeOf() const override { return sizeof(T); }

    template <typename T>
    std::enable_if_t<!std::is_assignable<Object, T>::value, void>
    CallOnAfterDeserialize(const SerializationContext& context, T& object) const {
        // object.OnAfterDeserializeCallback(context);
    }

    template <typename T>
    std::enable_if_t<std::is_assignable<Object, T>::value, void>
    CallOnAfterDeserialize(const SerializationContext& context, T& object) const {
        // TODO rename
        // TODO not raw pointer ?
        context.rootDeserializedObjects->push_back(&object);
    }

    template <typename T>
    std::enable_if_t<!std::is_assignable<Object, T>::value, void>
    CallOnBeforeSerialize(SerializationContext& context, const T& object) const {
        // object.OnBeforeSerializeCallback(context);
    }

    template <typename T>
    std::enable_if_t<std::is_assignable<Object, T>::value, void>
    CallOnBeforeSerialize(SerializationContext& context, const T& object) const {
        object.OnBeforeSerializeCallback(context);
    }
};

template <typename T>
std::enable_if_t<!std::is_assignable<Component, T>::value, int> inline AddComponentTags(ReflectedTypeBase* type) {
    return 0;
}

// TODO maybe this should not be here (Component.h is included in Reflect.h is yikes
template <typename T>
std::enable_if_t<std::is_assignable<Component, T>::value, int> inline AddComponentTags(ReflectedTypeBase* type) {
    if (typeid(&T::Update) != typeid(&Component::Update)) {
        type->tags.push_back("HasUpdate");
    }
    if (typeid(&T::FixedUpdate) != typeid(&Component::FixedUpdate)) {
        type->tags.push_back("HasFixedUpdate");
    }
    return 1;
}

#define _REFLECT_BEGIN_NO_PARENT_(className)                                  \
   public:                                                                    \
    class Type : public ReflectedType<##className> {                          \
        using TYPE = ##className;                                             \
                                                                              \
       public:                                                                \
        std::vector<std::function<ReflectedMethod(void)>> methodInitializers; \
        Type() : ReflectedType<##className>(#className) {                     \
            if (AddComponentTags<##className>(this) != 0) {                   \
                __parentType__ = GetReflectedType<Component>();               \
            }

#define REFLECT_VAR(varName) \
    fields.push_back(ReflectedField(GetReflectedType<decltype(varName)>(), #varName, offsetOf(&TYPE::varName)));

#define REFLECT_METHOD(func) \
    methodInitializers.push_back([]() { return GenerateMethodBinding(#func, args_t<decltype(&TYPE::func)>{}, &TYPE::func); });

#define REFLECT_METHOD_EXPLICIT(funcName, func) \
    methodInitializers.push_back([]() { return GenerateMethodBinding(funcName, args_t<decltype(func)>{}, func); });

#define REFLECT_ATTRIBUTE(attribute) \
    attributes.push_back(std::shared_ptr<Attribute>(new attribute));

#define REFLECT_END()                                                                                                                                                    \
    }                                                                                                                                                                    \
    void Init() {                                                                                                                                                        \
        for (auto& initializer : methodInitializers) {                                                                                                                   \
            __methods__.push_back(initializer());                                                                                                                        \
        }                                                                                                                                                                \
        methodInitializers.clear();                                                                                                                                      \
    }                                                                                                                                                                    \
    virtual size_t SizeOf() const override { return sizeof(TYPE); }                                                                                                      \
    }                                                                                                                                                                    \
    ;                                                                                                                                                                    \
    static Type* TypeOf() {                                                                                                                                              \
        static Type t;                                                                                                                                                   \
        static bool isInited = false;                                                                                                                                    \
        if (!isInited) { /* to properly init methods we need to be able to call TypeOf recursively TODO maybe create GetMethods() method and move initialization there*/ \
            isInited = true;                                                                                                                                             \
            t.Init();                                                                                                                                                    \
        }                                                                                                                                                                \
        return &t;                                                                                                                                                       \
    }                                                                                                                                                                    \
    ReflectedTypeBase* GetType() const {                                                                                                                                 \
        return TypeOf();                                                                                                                                                 \
    }

// TODO try to not duplicate
#define REFLECT_END_CUSTOM(Serialize, Deserialize)                                               \
    }                                                                                            \
    virtual void Serialize(SerializationContext& context, const void* object) const override {   \
        const TYPE& t = *(TYPE*)object;                                                          \
        Serialize(context, t);                                                                   \
    }                                                                                            \
    virtual void Deserialize(const SerializationContext& context, void* object) const override { \
        TYPE& t = *(TYPE*)object;                                                                \
        Deserialize(context, t);                                                                 \
        REFLECT_END()

#define _REFLECT_BEGIN_WITH_PARENT_(className, parentClassName)                                         \
    _REFLECT_BEGIN_NO_PARENT_(className)                                                                \
    __parentType__ = parentClassName::TypeOf();                                                         \
    for (auto parentField : dynamic_cast<ReflectedTypeNonTemplated*>(__parentType__)->fields) {         \
        this->fields.push_back(parentField);                                                            \
    }                                                                                                   \
    for (auto parentTag : dynamic_cast<ReflectedTypeNonTemplated*>(__parentType__)->tags) {             \
        this->tags.push_back(parentTag);                                                                \
    }                                                                                                   \
    for (auto parentAttribute : dynamic_cast<ReflectedTypeNonTemplated*>(__parentType__)->attributes) { \
        this->attributes.push_back(parentAttribute);                                                    \
    }

#define _EXPAND_(x) x

#define _GET_REFLECT_MACRO_(_1, _2, NAME, ...) NAME

#define REFLECT_BEGIN(...) _EXPAND_(_GET_REFLECT_MACRO_(__VA_ARGS__, _REFLECT_BEGIN_WITH_PARENT_, _REFLECT_BEGIN_NO_PARENT_)(__VA_ARGS__))

#define REFLECT_CUSTOM(className, Serialize, Deserialize)                                 \
   public:                                                                                \
    static ReflectedTypeBase* TypeOf() {                                                  \
        static ReflectedTypeCustom<##className> type(#className, Serialize, Deserialize); \
        return &type;                                                                     \
    }                                                                                     \
    ReflectedTypeBase* GetType() const {                                                  \
        return TypeOf();                                                                  \
    }

#define REFLECT_ENUM_FLAGS(enumClass)                                 \
    template <>                                                       \
    inline ReflectedTypeBase* GetReflectedType<##enumClass>() {       \
        static ReflectedTypeEnum<##enumClass, true> type(#enumClass); \
        return &type;                                                 \
    }

#define REFLECT_ENUM(enumClass)                                        \
    template <>                                                        \
    inline ReflectedTypeBase* GetReflectedType<##enumClass>() {        \
        static ReflectedTypeEnum<##enumClass, false> type(#enumClass); \
        return &type;                                                  \
    }

template <typename T>
void Deserialize(const SerializationContext& context, T& t) {
    GetReflectedType<T>()->Deserialize(context, &t);
}

template <typename T>
std::enable_if_t<!std::is_assignable<Object, T>::value, ReflectedTypeBase*> inline GetReflectedType(const T* t) {
    return GetReflectedType<T>();
}
template <typename T>
std::enable_if_t<std::is_assignable<Object, T>::value, ReflectedTypeBase*>
GetReflectedType(const T* t) {
    return t->GetType();
}

template <typename T>
void Serialize(SerializationContext& context, const T& t) {
    GetReflectedType(&t)->Serialize(context, &t);
}