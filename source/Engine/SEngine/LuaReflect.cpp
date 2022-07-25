#include "lua.h"
#include "luaLib.h"
#include "Component.h"
#include <functional>
#include "System.h"
#include "Common.h"
#include "LuaSystem.h"
#include "LuaReflect.h"
#include "SMath.h"


std::map<std::weak_ptr<Object>, std::shared_ptr<LuaObjectRef>, std::owner_less<std::weak_ptr<Object>>> sharedPointersInLua;
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

    Luna::Register<Foo>(LuaSystem::Get()->L);

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
static void DeserializeFromLuaToContext(lua_State* L, int idx, SerializationContext& context) {
    if (lua_isnumber(L, idx)) {

        double num = lua_tonumber(L, idx);
        context << (float)num;
        //if (type == GetReflectedType<int>()) {
        //    context << (int)num;
        //}
        //else {
        //    ASSERT(type == GetReflectedType<float>());
        //    context << (float)num;
        //}
    }
    else if (lua_isstring(L, idx)) {
        std::string str = lua_tostring(L, idx);
        context << str;
    }
    else if (lua_isvector(L, idx)) {
        Vector3 vec(lua_tovector(L, idx));
        Serialize(context, vec);
    }
    else if(lua_isuserdata(L, idx)) {
        //TODO ?
    }
    else if (lua_isnil(L, idx)) {
        //TODO ?
    } else{
        ASSERT(lua_istable(L, idx));

        lua_pushnil(L);  /* first key */
        int tableIdx = idx - 1;

        int currentOffset = 0;
        while (lua_next(L, tableIdx) != 0) {
            bool keyWasString = lua_type(L, -2) == lua_Type::LUA_TSTRING;
            const char* key;

            if (!keyWasString) {
                lua_pushvalue(L, -2);
                key = lua_tostring(L, -1);
                lua_pushvalue(L, -2);
            }
            else {
                key = lua_tostring(L, -2);
            }

            DeserializeFromLuaToContext(L, -1, context.Child(key));
            if (!keyWasString) {
                lua_pop(L, 2);
            }
            lua_pop(L, 1);
        }
    }
}
//TODO optimize
static void DeserializeFromLua(lua_State* L, ReflectedTypeBase* type, void** dst, int idx, bool canChangePtr) {

    if (dynamic_cast<ReflectedTypeSharedPtrBase*>(type) != nullptr) {
        //TODO typecheck and stuff
        auto& dstObj = *(std::shared_ptr<Object>*)(*dst);
        //TODO think about it. Do we need deserialize the rest of lua object or not in that case?
        if (lua_isnil(L, -1)) {
            dstObj = std::shared_ptr<Object>();
            return;
        }
        lua_pushnumber(L, 0);
        lua_gettable(L, idx - 1);

        dstObj = *static_cast<std::shared_ptr<Object>*> (lua_touserdata(L, -1));
        //std::shared_ptr<Object>& obj = *static_cast<std::shared_ptr<Object>*>(luaL_checkudata(L, -1, (GetReflectedType<Object>()->GetName() + shared_ptr_suffix).c_str()));
        lua_remove(L, -1);
        return;
    }

    if (lua_istable(L, idx)) {
        lua_pushnumber(L, 0);
        lua_rawget(L, idx - 1);
        if (!lua_isnil(L, -1)) {
            SimpleUserData& obj = *static_cast<SimpleUserData*> (lua_touserdata(L, -1));
            lua_pop(L, 1);
            if (type == obj.type) {
                //TODO use type to copy instead memcpy
                if (canChangePtr) {
                    *dst = obj.dataPtr;
                }
                else {
                    memcpy(*dst, obj.dataPtr, type->SizeOf());
                }
                return;
            }
            else {
                ASSERT(false);
            }
        }
        else {
            lua_pop(L, 1);
        }
    }


    SerializationContext context{};
    context.isLua = true;
    DeserializeFromLuaToContext(L, idx, context);
    type->Deserialize(context, *dst);
}

void DeserializeFromLua(lua_State* L, ReflectedTypeBase* type, void** dst, int idx) {
    DeserializeFromLua(L, type, dst, idx, true);
}

void DeserializeFromLua(lua_State* L, ReflectedTypeBase* type, void* dst, int idx) {
    DeserializeFromLua(L, type, &dst, idx, false);
}

static void MergeToLua(lua_State* L, const SerializationContext& context, int targetIdx, const std::string& targetField) {
    //TODO remove GetName() // currently required for lib / engine interaction
    if (!context.IsDefined()) {
        return;
    }
    const auto node = context.GetYamlNode();
    auto tag = ryml::to_tag(node.val_tag());
    if (!context.IsMap()) {
        float f;
        int i;
        bool b; if (c4::from_chars(node.val(), &i)) {
            int val = i;
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
        else if (c4::from_chars(node.val(), &f)) {
            float val = f;
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
        else if (c4::from_chars(node.val(), &b)) {
            bool val = b;
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
            std::string val = std::string(node.val().str, node.val().len);
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
    }
    else {
        //TODO check
        if (targetField.empty()) {
            if (!lua_istable(L, targetIdx)) {
                //TODO assert if it's not
                lua_newtable(L);
                lua_replace(L, targetIdx - 1);
            }
            for (auto field : context.GetChildrenNames()) {
                MergeToLua(L, context.Child(field), targetIdx, field);
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
            MergeToLua(L, context, -1, "");
            lua_pop(L, 1);
        }
    }
}

void MergeToLua(lua_State* L, ReflectedTypeBase* srcType, void* src, int targetIdx, const std::string& targetField) {
    SerializationContext context{};
    context.isLua = true;
    srcType->Serialize(context, src);
    MergeToLua(L, context, targetIdx, targetField);
}

int PushToLua(lua_State* L, ReflectedTypeBase* type, void* src) {
    //TODO get rid of ->GetName()
    SerializationContext context{};
    context.isLua = true;
    if (type->GetName() == GetReflectedType<int>()->GetName()) {
        int val = *(int*)src;
        lua_pushnumber(L, val);
        return 1;
    }
    else if (type->GetName() == GetReflectedType<float>()->GetName()) {
        float val = *(float*)src;
        lua_pushnumber(L, val);
        return 1;
    }
    else if (type->GetName() == GetReflectedType<std::string>()->GetName()) {
        std::string str = *(std::string*)src;
        lua_pushstring(L, str.c_str());
        return 1;
    }
    else if (dynamic_cast<ReflectedTypeSharedPtrBase*>(type)) { //TODO costy and not good generaly
        auto obj = *(std::shared_ptr<Object>*)(src);
        if (obj) {
            Luna::Push(L, obj);
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    else if (type->GetName() == GetReflectedType<Vector3>()->GetName()) {
        Vector3 vec = *(Vector3*)src;
        lua_pushvector(L, vec.x, vec.y, vec.z);
        return 1;
    }
    else if (type->GetName() == GetReflectedType<bool>()->GetName()) {
        bool b = *(bool*)src;
        lua_pushboolean(L, b);
        return 1;
    }
    else {
        //TODO bind c++ methods and stuff (not just raw table) and maybe just bind userdata and go on
        Luna::Push(L, type, src);
        return 1;
    }
    return 0;
}

LuaObjectRef::LuaObjectRef(lua_State* L, int stackIdx) :L(L), refId(lua_ref(L, stackIdx)) {
    ASSERT(L == LuaSystem::Get()->L);
}
