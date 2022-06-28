#pragma once 

#include "lua.h"
#include "luaLib.h"
#include "Reflect.h"
#include "ryml.hpp"
#include <map>

void DeserializeFromLua(lua_State* L, ReflectedTypeBase* type, void* dst, int idx);

int PushToLua(lua_State* L, ReflectedTypeBase* type, void* src);

//TODO try to remove or replace "/shared_ptr"

//TODO not static
//TODO unordered_map if possible
extern std::map<std::weak_ptr<Object>, int, std::owner_less<std::weak_ptr<Object>>> sharedPointersInLua;

//TODO WIP
template<class T> class Luna {
public:
    //Registers T() function/constuctor in lua
    static void Register(lua_State* L) {
        lua_pushcfunction(L, &Luna<T>::constructorSimple, (GetReflectedType<T>()->GetName() + "::Constructor").c_str());
        lua_setglobal(L, GetReflectedType<T>()->GetName().c_str());
        
        luaL_newmetatable(L, GetReflectedType<T>()->GetName().c_str());
        lua_pop(L, 1);
    }

    static void RegisterShared(lua_State* L) {
        lua_pushcfunction(L, &Luna<T>::constructorShared, (GetReflectedType<T>()->GetName() + "::ConstructorShared").c_str());
        lua_setglobal(L, (GetReflectedType<T>()->GetName() + "/shared_ptr").c_str());

        luaL_newmetatable(L, (GetReflectedType<T>()->GetName() + "/shared_ptr").c_str());
        lua_pop(L, 1);
    }


    //does not pop
    //TODO checks
    static void Bind(lua_State* L, std::shared_ptr<T> obj, int stackIdx, int refId = LUA_REFNIL) {
        Bind(L, std::static_pointer_cast<Object>(obj), GetReflectedType<T>(), stackIdx, refId);
    }
    static void Bind(lua_State* L, std::shared_ptr<Object> obj, ReflectedTypeBase* type, int stackIdx, int refId = LUA_REFNIL) {
        lua_pushnumber(L, 0);
        std::shared_ptr<Object>* a = (std::shared_ptr<Object>*)lua_newuserdatadtor(L, sizeof(std::shared_ptr<Object>), &destructor_shared_ptr);
        //clearing a so destructor wont be called
        memcpy(a, &std::shared_ptr<Object>(), sizeof(std::shared_ptr<Object>));
        *a = obj;
        luaL_getmetatable(L, (type->GetName() + "/shared_ptr").c_str());
        if (lua_isnil(L, -1)) {
            RegisterShared(L);
            lua_pop(L, 1);
            luaL_getmetatable(L, (type->GetName() + "/shared_ptr").c_str());
        }
        //lua_pushvalue(L, -1);
        //lua_setmetatable(L, -5);//table metatable
        lua_setmetatable(L, -2);//userdata metatable
        lua_settable(L, -3); // table[0] = obj;

        bindMethodsShared(L, type);

        if (refId == LUA_REFNIL) {
            refId = lua_ref(L, -1);
        }
        std::weak_ptr<Object> weakObj = std::static_pointer_cast<Object>(obj);
        sharedPointersInLua[weakObj] = refId;
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
            lua_rawgeti(L, LUA_REGISTRYINDEX, it->second);
            return;
        }

        lua_newtable(L);
        Bind(L, obj, type, -1);
    }
    static void Push(lua_State* L, std::shared_ptr<T> obj) {
        Push(L, obj, GetReflectedType<T>());
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
        Push(L, std::make_shared<T>());
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

    static int call_method(lua_State* L, int index, T* obj) {
        //TODO push/convert params, pop/convert result
        const auto& method = GetReflectedType<T>()->GetMethods()[index];

        size_t totalSize = 0;
        for (auto argType : method.argTypes) {
            totalSize += argType->SizeOf();
        }
        std::vector<void*> params(method.argTypes.size());
        std::vector<char> rawParamsData(totalSize, 0);

        char* currentPtr = rawParamsData.data();
        for (int i = 0; i < method.argTypes.size(); i++) {
            params[i] = (void*)currentPtr;
            int stackIdx = -method.argTypes.size() + i;
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

    static int thunkShared(lua_State* L) {
        int i = (int)lua_tonumber(L, lua_upvalueindex(1));
        lua_pushnumber(L, 0);
        lua_gettable(L, 1);

        std::shared_ptr<T>& obj = *static_cast<std::shared_ptr<T>*>(luaL_checkudata(L, -1, (GetReflectedType<T>()->GetName() + "/shared_ptr").c_str()));
        lua_remove(L, -1);

        return call_method(L, i, obj.get());
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