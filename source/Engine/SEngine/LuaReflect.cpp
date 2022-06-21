#include "lua.h"
#include "LuaReflect.h"
#include "Component.h"
#include <functional>
#include "System.h"
#include "Common.h"

//TODO cleanup

class Foo : public Object {
public:
    int foo(int i, float f, std::shared_ptr<GameObject> go) {
        printf("in foo\n");
        return 1;
    }

    void boo() {
        printf("in boo\n");
    }

    REFLECT_BEGIN(Foo);
    REFLECT_METHOD(foo);
    REFLECT_METHOD(boo);
    REFLECT_END();
};

static void Test() {
    //auto binding = some_function(args_t<decltype(&Foo::foo)>{});
    //
    for (auto binding : Foo::TypeOf()->methods) {
        Log(binding.ToString());
    }
}



class TestSys : public System<TestSys> {

    bool Init() override{
        Test();
        return true;
    }
};
REGISTER_SYSTEM(TestSys);