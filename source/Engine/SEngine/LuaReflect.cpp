#include "lua.h"
#include "luaLib.h"
#include "Component.h"
#include <functional>
#include "System.h"
#include "Common.h"
#include "LuaSystem.h"
#include "LuaReflect.h"
#include "SMath.h"


std::map<std::weak_ptr<Object>, int, std::owner_less<std::weak_ptr<Object>>> sharedPointersInLua;
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
        Luna<Object>::Push(L, *(std::shared_ptr<Object>*)(src), ((std::shared_ptr<Object>*)(src))->get()->GetType());//TODO not good at all
        return 1;
    }
    else if (type == GetReflectedType<Vector3>()) {
        Vector3 vec = *(Vector3*)src;
        lua_pushvector(L, vec.x, vec.y, vec.z);
        return 1;
    }
    else {
        ASSERT_FAILED("Unknown type for lua: '%s'", type->GetName().c_str());
        //TODO
    }
    return 0;
}