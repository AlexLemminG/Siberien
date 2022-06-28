#include "LuaComponent.h"
#include "LuaSystem.h"
#include "lua.h"
#include "GameObject.h"
#include "LuaReflect.h"


void LuaComponent::Call(char* funcName) {
	if (ref == LUA_REFNIL) {
		return;
	}
	
	auto L = LuaSystem::Get()->L;
	lua_getref(L, ref);
	lua_getfield(LuaSystem::Get()->L, -1, funcName);//TODO cache
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_pop(L, 1);
		return;
	}
	lua_getref(L, ref);
	lua_call(LuaSystem::Get()->L, 1, 0);
	lua_pop(L, 1);
}

void LuaComponent::OnEnable() {
	auto L = LuaSystem::Get()->L;

	Luna<LuaComponent>::RegisterShared(L);

	LuaSystem::Get()->PushModule(script.c_str());
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return;
	}

	lua_getfield(LuaSystem::Get()->L, -1, "new");
	lua_pushvalue(L, -2);
	lua_pushnil(L);
	lua_call(LuaSystem::Get()->L, 2, 1);
	ref = lua_ref(L, -1);

	//TODO check if binded
	for (auto& c : gameObject()->components) {
		if(c.get() == this) {
			Luna<LuaComponent>::Bind(L, std::dynamic_pointer_cast<LuaComponent>(c), -1, ref);
			break;
		}
	}
	lua_pop(L, 2);

	Call("OnEnable");

}

void LuaComponent::OnDisable() {
	Call("OnDisable");

	auto L = LuaSystem::Get()->L;
	lua_unref(L, ref);
	ref = LUA_REFNIL;
}

void LuaComponent::Update() {
	//TODO disable call when no definition
	Call("Update");
}

void LuaComponent::FixedUpdate() {
	//TODO disable call when no definition
	Call("FixedUpdate");
}

void LuaComponent::OnValidate() {
	Call("OnValidate");
}

void LuaComponent::OnDrawGizmos() {
	//TODO disable call when no definition
	Call("OnDrawGizmos");
}

DECLARE_TEXT_ASSET(LuaComponent);
