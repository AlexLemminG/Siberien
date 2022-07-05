#include "lua.h"
#include "luaLib.h"
#include "Component.h"
#include <functional>
#include "System.h"
#include "Common.h"
#include "LuaSystem.h"
#include "LuaReflect.h"
#include "SMath.h"


std::map<std::weak_ptr<Object>, std::weak_ptr<LuaObjectRef>, std::owner_less<std::weak_ptr<Object>>> sharedPointersInLua;
std::vector<ReflectedTypeBase*> registeredTypesInLua;

//TODO cleanup
//TODO const func test
class Foo : public Object {
public:
    int foo(int i) {
        printf("in foo %d\n", i);
        return 42;
    }
    int goo(int i, float f) {
        printf("in goo %d %.2f\n", i, f);
        return 3;
    }

    void boo() {
        printf("in boo\n");
    }

    virtual ~Foo() {
        printf("Foo no more\n");

    }

    int i = 0;

    REFLECT_BEGIN(Foo);
    REFLECT_METHOD(foo);
    REFLECT_METHOD(goo);
    REFLECT_METHOD(boo);
    REFLECT_END();
};


static void Teste2(int i, float f) {

}

template<class... Params>
static void Teste(int(Foo::* func)(Params...)) {
    std::tuple<Foo*, Params...> pa{};
    //auto pb = std::make_from_tuple<Params...>(std::move(pa));
    std::apply(func, pa);
}

static void Test() {
    //auto binding = some_function(args_t<decltype(&Foo::foo)>{});
    //
    for (auto binding : Foo::TypeOf()->GetMethods()) {
        Log(binding.ToString());
    }
    auto t = std::make_tuple(3, 3.44, "sdfs");
    //std::make_from_tuple(t);

    Luna<Foo>::Register(LuaSystem::Get()->L);

    auto func = std::function(&Teste2);
    //Foo f;
    Teste(&Foo::foo);
}


class TestSys : public System<TestSys> {

    bool Init() override{
        Test();
        return true;
    }
    bool b = false;

    void Update() override {
        if (!b) {
            b = true;
        }
    }
    virtual PriorityInfo GetPriorityInfo() const {
        return PriorityInfo{1};
    }
};
REGISTER_SYSTEM(TestSys);

//TODO optimize
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
    else if (lua_isstring(L, idx)) {
        std::string str = lua_tostring(L, idx);
        context << str;
    }
    else if (lua_isvector(L, idx)) {
        Vector3 vec(lua_tovector(L, idx));
        Serialize(context, vec);
    }
    else {
        ASSERT(lua_istable(L, idx));
        //TODO
    }
    type->Deserialize(context, dst);
}

void MergeToLua(lua_State* L, ReflectedTypeBase* srcType, void* src, int targetIdx, const std::string& targetField) {
    if (srcType == GetReflectedType<int>()) {
        int val = *(int*)src;
        if (targetField.empty()) {
            lua_pushnumber(L, val);
            lua_replace(L, targetIdx - 1);
        }
        else {
            lua_pushstring(L, targetField.c_str());
            lua_pushnumber(L, val);
            lua_settable(L, targetIdx - 2);
        }
    }
    else if (srcType == GetReflectedType<std::string>()) {
        std::string& val = *(std::string*)src;
        if (targetField.empty()) {
            lua_pushstring(L, val.c_str());
            lua_replace(L, targetIdx - 1);
        }
        else {
            lua_pushstring(L, targetField.c_str());
            lua_pushstring(L, val.c_str());
            lua_settable(L, targetIdx - 2);
        }
    }
    else if (srcType == GetReflectedType<float>()) {
        float val = *(float*)src;
        if (targetField.empty()) {
            lua_pushnumber(L, val);
            lua_replace(L, targetIdx - 1);
        }
        else {
            lua_pushstring(L, targetField.c_str());
            lua_pushnumber(L, val);
            lua_settable(L, targetIdx - 2);
        }
    }
    else if (srcType == GetReflectedType<bool>()) {
        bool val = *(bool*)src;
        if (targetField.empty()) {
            lua_pushboolean(L, val);
            lua_replace(L, targetIdx - 1);
        }
        else {
            lua_pushstring(L, targetField.c_str());
            lua_pushboolean(L, val);
            lua_settable(L, targetIdx - 2);
        }
    }
    else {
        //TODO check
        if (targetField.empty()) {
            if (!lua_istable(L, targetIdx)) {
                //TODO assert if it's not
                lua_newtable(L);
                lua_replace(L, targetIdx - 1);
            }
            for (auto field : srcType->fields) {
                MergeToLua(L, field.type, field.GetPtr(src), targetIdx, field.name);
            }
        }
        else {
            lua_getfield(L, targetIdx, targetField.c_str());
            if (lua_isnil(L, -1) || !lua_istable(L, -1)) {
                lua_pop(L, 1);
                lua_pushstring(L, targetField.c_str());
                lua_newtable(L);
                lua_settable(L, targetIdx - 2);
                lua_getfield(L, targetIdx, targetField.c_str());
            }
            ASSERT(!lua_isnil(L, -1) && lua_istable(L, -1));
            MergeToLua(L, srcType, src, -1, "");
            lua_pop(L, 1);
        }
    }
}
int PushToLua(lua_State* L, ReflectedTypeBase* type, void* src) {
    SerializationContext context{};
    if (type == GetReflectedType<int>()) {
        int val = *(int*)src;
        lua_pushnumber(L, val);
        return 1;
    }
    else if (type == GetReflectedType<float>()) {
        float val = *(float*)src;
        lua_pushnumber(L, val);
        return 1;
    }
    else if (type == GetReflectedType<std::string>()) {
        std::string str = *(std::string*)src;
        lua_pushstring(L, str.c_str());
        return 1;
    }
    else if (dynamic_cast<ReflectedTypeSharedPtrBase*>(type)) { //TODO costy and not good generaly
        auto obj = *(std::shared_ptr<Object>*)(src);
        if (obj) {
            Luna<Object>::Push(L, obj, obj->GetType());
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    else if (type == GetReflectedType<Vector3>()) {
        Vector3 vec = *(Vector3*)src;
        lua_pushvector(L, vec.x, vec.y, vec.z);
        return 1;
    }
    else if (type == GetReflectedType<bool>()) {
        bool b = *(bool*)src;
        lua_pushboolean(L, b);
        return 1;
    }
    else {
        ASSERT_FAILED("Unknown type for lua: '%s'", type->GetName().c_str());
        //TODO
    }
    return 0;
}