#pragma once 

#include "lua.h"
#include "luaLib.h"
#include "Reflect.h"
#include "ryml.hpp"
#include <map>
#include "Asserts.h"

SE_CPP_API void DeserializeFromLua(lua_State* L, ReflectedTypeBase* type, void* dst, int idx);

class LuaObjectRef{
public:
    LuaObjectRef(lua_State* L, int stackIdx);
    ~LuaObjectRef(){
        if (L) {
            lua_unref(L, refId);
        }
    }
    void PushToStack(lua_State* L) {
        ASSERT(L == this->L);
        lua_getref(L, refId);
    }
    void HandleLuaStateDestroyed() {
        ASSERT(L);
        L = nullptr;
    }
private:
    lua_State* L = nullptr;
    int refId = 0;
};
SE_CPP_API int PushToLua(lua_State* L, ReflectedTypeBase* type, void* src);
SE_CPP_API void MergeToLua(lua_State* L, ReflectedTypeBase* srcType, void* src, int targetIdx, const std::string& targetField);
//TODO try to remove or replace "_shared_ptr"

constexpr char* shared_ptr_suffix = "";
//constexpr char* shared_ptr_suffix = "_shared_ptr";

//TODO rename and move somewhere
//TODO not static
//TODO unordered_map if possible
extern std::map<std::weak_ptr<Object>, std::shared_ptr<LuaObjectRef>, std::owner_less<std::weak_ptr<Object>>> sharedPointersInLua;
extern std::vector<ReflectedTypeBase*> registeredTypesInLua;


//TODO nontemplated version and stuff
//TODO WIP
class SE_CPP_API Luna {
public:
    //TODO add to some registered list so we would know what to do, when scripts are reloaded
    //Registers T() function/constuctor in lua

    static void Register(lua_State* L, lua_CFunction fn, ReflectedTypeBase* type) {
        lua_pushcfunction(L, fn, (type->GetName() + "::Constructor").c_str());
        lua_setglobal(L, type->GetName().c_str());

        luaL_newmetatable(L, type->GetName().c_str());
        lua_pop(L, 1);
    }

    template<class T>
    static void Register(lua_State* L) {
        Register(L, &Luna::constructorSimple<T>, GetReflectedType<T>());
    }

    //TODO add to some registered list so we would know what to do, when scripts are reloaded
    static void RegisterShared(lua_State* L, ReflectedTypeBase* type) {
        lua_pushnumber(L, registeredTypesInLua.size());
        registeredTypesInLua.push_back(type);
        //TODO remove T
        lua_pushcclosure(L, &Luna::constructorShared, (type->GetName() + "::ConstructorShared").c_str(), 1);
        lua_setglobal(L, (type->GetName() + shared_ptr_suffix).c_str());

        luaL_newmetatable(L, (type->GetName() + shared_ptr_suffix).c_str());
        lua_pushstring(L, "__newindex");
        lua_pushcclosure(L, &Luna::__newindexShared, "__newindexShared", 0);
        lua_settable(L, -3);
        lua_pushstring(L, "__index");
        lua_pushcclosure(L, &Luna::__indexShared, "__indexShared", 0);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }
    template<class T>
    static inline void RegisterShared(lua_State* L) {
        RegisterShared(L, GetReflectedType<T>());
    }


    //does not pop
    //TODO checks
    static std::shared_ptr<LuaObjectRef> Bind(lua_State* L, std::shared_ptr<Object> obj, int stackIdx) {
        ASSERT(obj);
        if (obj) {
            return Bind(L, obj, obj->GetType(), stackIdx);
        }
        else {
            return Bind(L, nullptr, GetReflectedType<Object>(), stackIdx);
        }
    }
    static std::shared_ptr<LuaObjectRef> Bind(lua_State* L, std::shared_ptr<Object> obj, ReflectedTypeBase* type, int stackIdx) {
        //TODO stackIdx not used
        bindMethodsShared(L, type);
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


        lua_pushvalue(L, -4);
        bool hasMeta = lua_getmetatable(L, -1);
        if (hasMeta && lua_equal(L, -1, -2)) {
            hasMeta = false;
            lua_pop(L, 1);
        }
        int metatablesCount = 0;
        while (hasMeta) {
            metatablesCount++;
            hasMeta = lua_getmetatable(L, -1);
            if (hasMeta && lua_equal(L, -1, -2)) {
                hasMeta = false;
                lua_pop(L, 1);
            }
        }
        lua_pushvalue(L, -2 - metatablesCount);
        lua_setmetatable(L, -2);
        lua_pop(L, metatablesCount + 1);

        lua_setmetatable(L, -2);//userdata metatable
        lua_rawset(L, -3); // table[0] = obj;


        lua_getmetatable(L, -1);
        lua_pushstring(L, "__newindex");
        lua_pushcclosure(L, &Luna::__newindexShared, "__newindexShared", 0);
        lua_settable(L, -3);
        lua_pop(L, 1);


        auto ref = std::make_shared<LuaObjectRef>(L, -1);

        std::weak_ptr<Object> weakObj = obj;
        sharedPointersInLua[weakObj] = ref;
        return ref;
    }

    //returns nullptr if not binded
    static std::shared_ptr<LuaObjectRef> TryGetRef(lua_State* L, std::shared_ptr<Object> obj) {
        if (obj == nullptr) {
            return nullptr;
        }
        std::weak_ptr<Object> weakObj = obj;
        auto it = sharedPointersInLua.find(weakObj);
        if (it != sharedPointersInLua.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Pushes obj to top of the stack and binds if required
    static void Push(lua_State* L, std::shared_ptr<Object> obj) {
        if (obj == nullptr) {
            lua_pushnil(L);
            return;
        }
        std::weak_ptr<Object> weakObj = obj;
        auto it = sharedPointersInLua.find(weakObj);
        if (it != sharedPointersInLua.end()) {
            it->second->PushToStack(L);
            return;
        }

        lua_newtable(L);
        Bind(L, obj, obj->GetType(), -1);
    }

    static int CallFunctionFromLua(lua_State* L, const ReflectedMethod& method) {
        return call_function(L, method, nullptr);
    }
private:
    template<class T>
    static int constructorSimple(lua_State* L) {
        T* obj = new T();

        lua_newtable(L);
        lua_pushnumber(L, 0);
        T** a = (T**)lua_newuserdatadtor(L, sizeof(T*), &destructor_simple<T>);
        *a = obj;
        luaL_getmetatable(L, GetReflectedType<T>()->GetName().c_str());
        //lua_pushvalue(L, -1);
        //lua_setmetatable(L, -5);//table metatable
        lua_setmetatable(L, -2);//userdata metatable
        lua_settable(L, -3); // table[0] = obj;

        bindMethodsSimple<T>(L);

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
        Push(L, std::shared_ptr<Object>(rawObj));
        return 1;
    }

    template<class T>
    static void destructor_simple(void* obj_raw) {
        T* obj = *(T**)obj_raw;
        delete obj;
    }

    static void destructor_shared_ptr(void* obj_raw) {
        //TODO is it legal to just c-cast ?
        std::shared_ptr<Object>& obj = *(std::shared_ptr<Object>*)obj_raw;
        obj = nullptr;
    }

    template<class T>
    static void bindMethodsSimple(lua_State* L) {
        for (int i = 0; i < GetReflectedType<T>()->GetMethods().size(); i++) {
            lua_pushstring(L, GetReflectedType<T>()->GetMethods()[i].name.c_str());
            lua_pushnumber(L, i);
            lua_pushcclosure(L, &Luna::thunkSimple<T>, (GetReflectedType<T>()->GetMethods()[i].name + " Luna::thunk").c_str(), 1);
            lua_settable(L, -3);
        }
    }

    static void bindMethodsShared(lua_State* L, ReflectedTypeBase* type) {
        for (int i = 0; i < type->GetMethods().size(); i++) {
            lua_pushstring(L, type->GetMethods()[i].name.c_str());
            lua_pushnumber(L, i);
            lua_pushcclosure(L, &Luna::thunkShared, (type->GetMethods()[i].name + " Luna::thunk").c_str(), 1);
            lua_settable(L, -3);
        }
    }
    static int __newindexShared(lua_State* L) {
        //TODO typecheck and stuff
        //TODO should return not 0 ?
        lua_pushnumber(L, 0);
        lua_rawget(L, -4);
        if (!lua_isnil(L, -1)) {
            std::shared_ptr<Object>& obj = *static_cast<std::shared_ptr<Object>*> (lua_touserdata(L, -1));
            lua_pop(L, 1);
            if (obj != nullptr) {
                std::string fieldName = lua_tostring(L, -2);
                for (auto field : obj->GetType()->fields) {
                    if (field.name == fieldName) {
                        DeserializeFromLua(L, field.type, field.GetPtr(obj.get()), -1);
                        return 0;
                    }
                }
            }
        }
        else {
            lua_pop(L, 1);
        }
        lua_rawset(L, -3);
        return 0;
    }
    static int __indexShared(lua_State* L) {
        //TODO get from user data if exists
        return lua_rawget(L, -2);
    }
    static int call_function(lua_State* L, const ReflectedMethod& method, void* obj) {
        size_t totalSize = 0;
        for (auto argType : method.argTypes) {
            totalSize += argType->SizeOf();
        }
        std::vector<void*> params(method.argTypes.size());
        std::vector<char> rawParamsData(totalSize, 0);

        char* currentPtr = rawParamsData.data();
        if (lua_gettop(L) != (method.argTypes.size() + (method.objectType == GetReflectedType<void>() ? 0 : 1))) {
            auto str = FormatString(
                "Unexpected number of arguments for function '%s'. Expected %d, Got %d",
                method.ToString().c_str(),
                method.argTypes.size() + (method.objectType == GetReflectedType<void>() ? 0 : 1),
                lua_gettop(L)
            );
            luaL_error(L, str.c_str());
            //lua_pushstring(L, str.c_str());
            //lua_error(L);
            return 0;
        }
        
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

    static int call_method(lua_State* L, int index, void* obj, ReflectedTypeBase* type) {
        //TODO memory is probably leaking due to no destructors calls
        const auto& method = type->GetMethods()[index];

        return call_function(L, method, obj);
    }

    static int thunkShared(lua_State* L) {
        int i = (int)lua_tonumber(L, lua_upvalueindex(1));
        lua_pushnumber(L, 0);
        lua_gettable(L, 1);

        //TODO typecheck and stuff
        std::shared_ptr<Object>& obj = *static_cast<std::shared_ptr<Object>*> (lua_touserdata(L, -1));
        //std::shared_ptr<Object>& obj = *static_cast<std::shared_ptr<Object>*>(luaL_checkudata(L, -1, (GetReflectedType<Object>()->GetName() + shared_ptr_suffix).c_str()));
        lua_remove(L, -1);

        return call_method(L, i, obj.get(), obj.get()->GetType());
    }

    template<class T>
    static int thunkSimple(lua_State* L) {
        int i = (int)lua_tonumber(L, lua_upvalueindex(1));
        lua_pushnumber(L, 0);
        lua_gettable(L, 1);

        T** obj = static_cast<T**>(luaL_checkudata(L, -1, GetReflectedType<T>()->GetName().c_str()));
        lua_remove(L, -1);

        return call_method(L, i, *obj, GetReflectedType<T>());
    }
};