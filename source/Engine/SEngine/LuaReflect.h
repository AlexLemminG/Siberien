#pragma once 

#include "lua.h"
#include "luaLib.h"
#include "Reflect.h"
#include "ryml.hpp"
#include <map>
#include "Asserts.h"

void DeserializeFromLua(lua_State* L, ReflectedTypeBase* type, void* dst, int idx);

class LuaObjectRef{
public:
    LuaObjectRef(lua_State* L, int stackIdx):L(L),refId(lua_ref(L, stackIdx)) {
    }
    ~LuaObjectRef(){
        lua_unref(L, refId);
    }
    void PushToStack(lua_State* L) {
        ASSERT(L == this->L);
        lua_getref(L, refId);
    }
private:
    lua_State* L = nullptr;
    int refId = 0;
};
int PushToLua(lua_State* L, ReflectedTypeBase* type, void* src);
void MergeToLua(lua_State* L, ReflectedTypeBase* srcType, void* src, int targetIdx, const std::string& targetField);
//TODO try to remove or replace "_shared_ptr"

constexpr char* shared_ptr_suffix = "";
//constexpr char* shared_ptr_suffix = "_shared_ptr";

//TODO not static
//TODO unordered_map if possible
extern std::map<std::weak_ptr<Object>, std::weak_ptr<LuaObjectRef>, std::owner_less<std::weak_ptr<Object>>> sharedPointersInLua;
extern std::vector<ReflectedTypeBase*> registeredTypesInLua;

//TODO nontemplated version and stuff
//TODO WIP
template<class T> class Luna {
public:
    //TODO add to some registered list so we would know what to do, when scripts are reloaded
    //Registers T() function/constuctor in lua
    static void Register(lua_State* L) {
        lua_pushcfunction(L, &Luna<T>::constructorSimple, (GetReflectedType<T>()->GetName() + "::Constructor").c_str());
        lua_setglobal(L, GetReflectedType<T>()->GetName().c_str());
        
        luaL_newmetatable(L, GetReflectedType<T>()->GetName().c_str());
        lua_pop(L, 1);
    }

    //TODO add to some registered list so we would know what to do, when scripts are reloaded
    static void RegisterShared(lua_State* L, ReflectedTypeBase* type) {
        lua_pushnumber(L, registeredTypesInLua.size());
        registeredTypesInLua.push_back(type);
        //TODO remove T
        lua_pushcclosure(L, &Luna<T>::constructorShared, (type->GetName() + "::ConstructorShared").c_str(), 1);
        lua_setglobal(L, (type->GetName() + shared_ptr_suffix).c_str());

        luaL_newmetatable(L, (type->GetName() + shared_ptr_suffix).c_str());
        lua_pop(L, 1);
    }
    static void RegisterShared(lua_State* L) {
        RegisterShared(L, GetReflectedType<T>());
    }


    //does not pop
    //TODO checks
    static std::shared_ptr<LuaObjectRef> Bind(lua_State* L, std::shared_ptr<T> obj, int stackIdx) {
        return Bind(L, std::static_pointer_cast<Object>(obj), GetReflectedType<T>(), stackIdx);
    }
    static std::shared_ptr<LuaObjectRef> Bind(lua_State* L, std::shared_ptr<Object> obj, ReflectedTypeBase* type, int stackIdx) {
        //TODO stackIdx not used
        lua_pushnumber(L, 0);
        std::shared_ptr<Object>* a = (std::shared_ptr<Object>*)lua_newuserdatadtor(L, sizeof(std::shared_ptr<Object>), &destructor_shared_ptr);
        new(a)(std::shared_ptr<Object>);
        *a = obj;
        luaL_getmetatable(L, (type->GetName() + shared_ptr_suffix).c_str());
        if (lua_isnil(L, -1)) {
            RegisterShared(L, type);
            lua_pop(L, 1);
            luaL_getmetatable(L, (type->GetName() + shared_ptr_suffix).c_str());
            //ASSERT(!lua_isnil(L, -1), "Unknown type for lua '%s'", type->GetName().c_str());
        }
        //lua_pushvalue(L, -1);
        //lua_setmetatable(L, -5);//table metatable
        lua_setmetatable(L, -2);//userdata metatable
        lua_settable(L, -3); // table[0] = obj;

        bindMethodsShared(L, type);

        auto ref = std::make_shared<LuaObjectRef>(L, -1);

        std::weak_ptr<Object> weakObj = std::static_pointer_cast<Object>(obj);
        sharedPointersInLua[weakObj] = std::weak_ptr(ref);
        return ref;
    }

    // Pushes obj to top of the stack and binds if required
    static void Push(lua_State* L, std::shared_ptr<Object> obj, ReflectedTypeBase* type) {
        if (obj == nullptr) {
            lua_pushnil(L);
            return;
        }
        auto baseObj = std::static_pointer_cast<Object>(obj);
        std::weak_ptr<Object> weakObj = baseObj;
        auto it = sharedPointersInLua.find(weakObj);
        if (it != sharedPointersInLua.end()) {
            auto locked = it->second.lock();
            if (locked) {
                ASSERT(!it->first.expired());
                locked->PushToStack(L);
                return;
            }
        }

        lua_newtable(L);
        Bind(L, obj, type, -1);
    }
    static void Push(lua_State* L, std::shared_ptr<T> obj) {
        Push(L, obj, GetReflectedType<T>());
    }

    static int CallFunctionFromLua(lua_State* L, const ReflectedMethod& method) {
        return Luna<void>::call_function(L, method, nullptr);
    }
private:
    static int constructorSimple(lua_State* L) {
        T* obj = new T();

        lua_newtable(L);
        lua_pushnumber(L, 0);
        T** a = (T**)lua_newuserdatadtor(L, sizeof(T*), &destructor_simple);
        *a = obj;
        luaL_getmetatable(L, GetReflectedType<T>()->GetName().c_str());
        //lua_pushvalue(L, -1);
        //lua_setmetatable(L, -5);//table metatable
        lua_setmetatable(L, -2);//userdata metatable
        lua_settable(L, -3); // table[0] = obj;

        bindMethodsSimple(L);

        return 1;
    }
    static int constructorShared(lua_State* L) {
        //TODO asserts
        int i = (int)lua_tonumber(L, lua_upvalueindex(1));
        auto* type = registeredTypesInLua[i];
        //TODO callConstructor
        Object* rawObj = (Object*)(void*)new char[type->SizeOf()];
        type->Construct(rawObj);
        SerializationContext context;
        type->Deserialize(context, rawObj);
        Push(L, std::shared_ptr<Object>(rawObj), type);
        return 1;
    }

    static void destructor_simple(void* obj_raw) {
        T* obj = *(T**)obj_raw;
        delete obj;
    }

    static void destructor_shared_ptr(void* obj_raw) {
        std::shared_ptr<T>& obj = *(std::shared_ptr<T>*)obj_raw;
        obj = nullptr;
    }

    static void bindMethodsSimple(lua_State* L) {
        for (int i = 0; i < GetReflectedType<T>()->GetMethods().size(); i++) {
            lua_pushstring(L, GetReflectedType<T>()->GetMethods()[i].name.c_str());
            lua_pushnumber(L, i);
            lua_pushcclosure(L, &Luna<T>::thunkSimple, (GetReflectedType<T>()->GetMethods()[i].name + " Luna::thunk").c_str(), 1);
            lua_settable(L, -3);
        }
    }

    static void bindMethodsShared(lua_State* L, ReflectedTypeBase* type) {
        for (int i = 0; i < type->GetMethods().size(); i++) {
            lua_pushstring(L, type->GetMethods()[i].name.c_str());
            lua_pushnumber(L, i);
            lua_pushcclosure(L, &Luna<T>::thunkShared, (type->GetMethods()[i].name + " Luna::thunk").c_str(), 1);
            lua_settable(L, -3);
        }
    }
    static int call_function(lua_State* L, const ReflectedMethod& method, T* obj) {
        size_t totalSize = 0;
        for (auto argType : method.argTypes) {
            totalSize += argType->SizeOf();
        }
        std::vector<void*> params(method.argTypes.size());
        std::vector<char> rawParamsData(totalSize, 0);

        char* currentPtr = rawParamsData.data();
        for (int i = 0; i < method.argTypes.size(); i++) {
            params[i] = (void*)currentPtr;
            int stackIdx = i - method.argTypes.size();
            DeserializeFromLua(L, method.argTypes[i], params[i], stackIdx);
            currentPtr += method.argTypes[i]->SizeOf();
        }

        size_t resultSize = 0;
        if (method.returnType != nullptr) {
            resultSize = method.returnType->SizeOf();
        }
        std::vector<char> rawResultData(resultSize, 0);

        method.func(obj, params, rawResultData.data());

        int resultCount = 0;
        if (method.returnType != nullptr) {
            resultCount += PushToLua(L, method.returnType, rawResultData.data());
        }
        return resultCount;
    }

    static int call_method(lua_State* L, int index, T* obj) {
        return call_method(L, index, obj, GetReflectedType<T>(obj));
    }
    static int call_method(lua_State* L, int index, T* obj, ReflectedTypeBase* type) {
        //TODO memory is probably leaking due to no destructors calls
        const auto& method = type->GetMethods()[index];

        return call_function(L, method, obj);
    }

    static int thunkShared(lua_State* L) {
        int i = (int)lua_tonumber(L, lua_upvalueindex(1));
        lua_pushnumber(L, 0);
        lua_gettable(L, 1);

        //TODO typecheck and stuff
        std::shared_ptr<T>& obj = *static_cast<std::shared_ptr<T>*> (lua_touserdata(L, -1));
        //std::shared_ptr<Object>& obj = *static_cast<std::shared_ptr<Object>*>(luaL_checkudata(L, -1, (GetReflectedType<Object>()->GetName() + shared_ptr_suffix).c_str()));
        lua_remove(L, -1);

        return call_method(L, i, obj.get(), obj.get()->GetType());
    }

    static int thunkSimple(lua_State* L) {
        int i = (int)lua_tonumber(L, lua_upvalueindex(1));
        lua_pushnumber(L, 0);
        lua_gettable(L, 1);

        T** obj = static_cast<T**>(luaL_checkudata(L, -1, GetReflectedType<T>()->GetName().c_str()));
        lua_remove(L, -1);

        return call_method(L, i, *obj);
    }

    static int gc_obj(lua_State* L) {
        T** obj = static_cast<T**>(luaL_checkudata(L, -1, GetReflectedType<T>()->GetName().c_str()));
        delete (*obj);
        return 0;
    }

    struct RegType {
        const char* name;
        int(T::* mfunc)(lua_State*);
    };
};