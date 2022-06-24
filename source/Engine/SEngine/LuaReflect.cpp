#include "lua.h"
#include "luaLib.h"
#include "Component.h"
#include <functional>
#include "System.h"
#include "Common.h"
#include "LuaSystem.h"
#include "LuaReflect.h"

//TODO cleanup

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
    Foo f;
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