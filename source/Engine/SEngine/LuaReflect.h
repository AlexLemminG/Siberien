#pragma once 

#include "Reflect.h"
#include "ryml.hpp"

void DeserializeFromLua(lua_State* L, ReflectedTypeBase* type, void* dst, int idx) {
    SerializationContext context{};
    if (lua_isnumber(L, idx)) {
        double num = lua_tonumber(L, idx);
        if (type == GetReflectedType<int>()) {
            context << (int)num;
        }
        else {
            ASSERT(type == GetReflectedType<float>());
            context << (float)num;
        }
    }
    else if(lua_isstring(L, idx)) {
        std::string str = lua_tostring(L, idx);
        context << str;
    }
    else {
        ASSERT(lua_istable(L, idx));
        //TODO
    }
    type->Deserialize(context, dst);
}

int PushToLua(lua_State* L, ReflectedTypeBase* type, void* src) {
    SerializationContext context{};
    if (type == GetReflectedType<int>()) {
        int val = *(int*)src;
        lua_pushnumber(L, val);
    }
    else if (type == GetReflectedType<float>()) {
        float val = *(float*)src;
        lua_pushnumber(L, val);
    }
    else if (type == GetReflectedType<std::string>()) {
        std::string str = *(std::string*)src;
        lua_pushstring(L, str.c_str());
    }
    else {
        ASSERT_FAILED("Unknown type for lua: '%s'", type->GetName());
        //TODO
    }
    return 1;
}

//TODO WIP
template<class T> class Luna {
public:
    static void Register(lua_State* L) {
        lua_pushcfunction(L, &Luna<T>::constructor, (GetReflectedType<T>()->GetName() + "::Constructor").c_str());
        lua_setglobal(L, GetReflectedType<T>()->GetName().c_str());
        
        luaL_newmetatable(L, GetReflectedType<T>()->GetName().c_str());
        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, &Luna<T>::gc_obj, (GetReflectedType<T>()->GetName() + "::GC_OBJ").c_str());
        lua_settable(L, -3);
    }

    static int constructor(lua_State* L) {
        T* obj = new T();

        lua_newtable(L);
        lua_pushnumber(L, 0);
        T** a = (T**)lua_newuserdata(L, sizeof(T*));
        *a = obj;
        luaL_getmetatable(L, GetReflectedType<T>()->GetName().c_str());
        lua_setmetatable(L, -2);
        lua_settable(L, -3); // table[0] = obj;


        for (int i = 0; i < GetReflectedType<T>()->GetMethods().size(); i++) {
            lua_pushstring(L, GetReflectedType<T>()->GetMethods()[i].name.c_str());
            lua_pushnumber(L, i);
            lua_pushcclosure(L, &Luna<T>::thunk, (GetReflectedType<T>()->GetMethods()[i].name + " Luna::thunk").c_str(), 1);
            lua_settable(L, -3);
        }
        return 1;
    }

    static int thunk(lua_State* L) {
        int i = (int)lua_tonumber(L, lua_upvalueindex(1));
        lua_pushnumber(L, 0);
        lua_gettable(L, 1);

        T** obj = static_cast<T**>(luaL_checkudata(L, -1, GetReflectedType<T>()->GetName().c_str()));
        lua_remove(L, -1);

        //TODO push/convert params, pop/convert result
        const auto& method = GetReflectedType<T>()->GetMethods()[i];

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

        method.func(*obj, params, rawResultData.data());

        int resultCount = 0;
        if (method.returnType != nullptr) {
            resultCount += PushToLua(L, method.returnType, rawResultData.data());
        }
        return resultCount;
        //return ((*obj)->*(GetReflectedType<T>()->methods[i].mfunc))(L);
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