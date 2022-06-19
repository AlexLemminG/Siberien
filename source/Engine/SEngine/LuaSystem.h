#pragma once

#include "System.h"

struct lua_State;

class LuaSystem : public System<LuaSystem> {
	friend class LuaComponent;
public:
	bool RegisterAndRun(const char* moduleName, const char* sourceCode, size_t sourceCodeLength);

	bool PushModule(const char* moduleName);

	struct CompiledCode {
		char* code = nullptr;
		size_t size = 0;
	};
private:

	std::unordered_map<std::string, CompiledCode> compiledCode;

	bool Init() override;
	void Term() override;
	lua_State* L = nullptr;

	void RegisterAndRunAll();

	void ReloadScripts();

	void Update() override;

	static int LuaRequire(lua_State* L);
};