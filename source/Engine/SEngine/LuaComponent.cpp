#include "LuaComponent.h"
#include "LuaSystem.h"
#include "lua.h"
#include "GameObject.h"
#include "LuaReflect.h"
#include "Common.h"

void LuaComponent::Call(char* funcName) {
    if (!ref) {
        return;
    }

    auto L = LuaSystem::Get()->L;
    ref->PushToStack(L);

    lua_getfield(LuaSystem::Get()->L, -1, funcName);  // TODO cache
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pop(L, 1);
        return;
    }
    ref->PushToStack(L);
    auto callResult = lua_pcall(L, 1, 0, 0);
    if (callResult != 0) {
        std::string error = lua_tostring(L, -1);
        LogError(error.c_str());
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

void LuaComponent::OnEnable() {
    InitLua();

    onBeforeScriptsReloadingHandler = LuaSystem::Get()->onBeforeScriptsReloading.Subscribe([this]() {
        ASSERT(!onBeforeReloadingCalled);
        Call("OnDisable");
        TermLua();
        onBeforeReloadingCalled = true;
    });
    onAfterScriptsReloadingHandler = LuaSystem::Get()->onAfterScriptsReloading.Subscribe([this]() {
        if (onBeforeReloadingCalled) {
            InitLua();
            Call("OnEnable");
            onBeforeReloadingCalled = false;
        }
    });

    Call("OnEnable");
}

void LuaComponent::InitLua() {
    if (ref != nullptr) {
        return;  // already inited, probably new object created while reloading
    }
    auto L = LuaSystem::Get()->L;

    // TODO remove double register
    Luna::RegisterShared<LuaComponent>(L);

    LuaSystem::Get()->PushModule(luaObj.scriptName.c_str());
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    std::shared_ptr<Component> thisShared;
    for (auto& c : gameObject()->components) {
        if (c.get() == this) {
            thisShared = c;
            break;
        }
    }

    lua_getfield(L, -1, "new");
    lua_pushvalue(L, -2);
    auto existingRef = Luna::TryGetRef(L, thisShared);
    if (existingRef != nullptr) {
        existingRef->PushToStack(L);
    } else {
        lua_pushnil(L);
    }
    lua_call(LuaSystem::Get()->L, 2, 1);

    // TODO deserialize luaObj to table

    MergeToLua(LuaSystem::Get()->L, luaObj.GetType(), &luaObj, -1, "");

    // Luna::Push(L, thisShared);//binds if not yet //TODO separate method for that ?
    // lua_pop(L, 1);
    ref = Luna::Bind(L, thisShared, -1);

    lua_pop(L, 2);
}
void LuaComponent::TermLua() {
    ref = nullptr;
}

void LuaComponent::OnDisable() {
    Call("OnDisable");

    LuaSystem::Get()->onBeforeScriptsReloading.Unsubscribe(onBeforeScriptsReloadingHandler);
    LuaSystem::Get()->onAfterScriptsReloading.Unsubscribe(onAfterScriptsReloadingHandler);
    TermLua();
}

void LuaComponent::Update() {
    // TODO disable call when no definition
    Call("Update");
}

void LuaComponent::FixedUpdate() {
    // TODO disable call when no definition
    Call("FixedUpdate");
}

void LuaComponent::OnValidate() {
    if (!ref) {
        return;
    }
    auto L = LuaSystem::Get()->L;
    ref->PushToStack(L);
    MergeToLua(LuaSystem::Get()->L, luaObj.GetType(), &luaObj, -1, "");
    lua_pop(L, 1);

    Call("OnValidate");
}

void LuaComponent::OnDrawGizmos() {
    // TODO disable call when no definition
    Call("OnDrawGizmos");
}

DECLARE_TEXT_ASSET(LuaComponent);
