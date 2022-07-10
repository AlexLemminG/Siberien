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
	lua_getfield(LuaSystem::Get()->L, -1, funcName);//TODO cache
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_pop(L, 1);
		return;
	}
	ref->PushToStack(L);
	auto callResult = lua_pcall(L, 1, 0, 0);
	if(callResult != 0) {
		std::string error = lua_tostring(L, -1);
		Log(error.c_str());
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

void LuaComponent::OnEnable() {
	InitLua();

	onBeforeScriptsReloadingHandler = LuaSystem::Get()->onBeforeScriptsReloading.Subscribe([this]() {TermLua(); Call("OnDisable"); });
	onAfterScriptsReloadingHandler = LuaSystem::Get()->onAfterScriptsReloading.Subscribe([this]() {InitLua(); Call("OnEnable"); });

	Call("OnEnable");

}


void LuaComponent::InitLua() {
	auto L = LuaSystem::Get()->L;

	//TODO remove double register
	Luna::RegisterShared<LuaComponent>(L);

	LuaSystem::Get()->PushModule(luaObj.scriptName.c_str());
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return;
	}

	lua_getfield(LuaSystem::Get()->L, -1, "new");
	lua_pushvalue(L, -2);
	lua_pushnil(L);
	lua_call(LuaSystem::Get()->L, 2, 1);

	//TODO deserialize luaObj to table

	MergeToLua(LuaSystem::Get()->L, luaObj.GetType(), &luaObj, -1, "");

	//TODO check if binded
	for (auto& c : gameObject()->components) {
		if (c.get() == this) {
			ref = Luna::Bind(L, c, -1);
			break;
		}
	}

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
	//TODO disable call when no definition
	Call("Update");
}

void LuaComponent::FixedUpdate() {
	//TODO disable call when no definition
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
	//TODO disable call when no definition
	Call("OnDrawGizmos");
}

DECLARE_TEXT_ASSET(LuaComponent);
