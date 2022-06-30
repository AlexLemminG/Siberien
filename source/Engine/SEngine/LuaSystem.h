#pragma once

#include "System.h"

struct lua_State;
struct lua_CompileOptions;

class LuaSystem : public System<LuaSystem> {
	friend class LuaComponent;
public:
	bool RegisterAndRun(const char* moduleName, const char* sourceCode, size_t sourceCodeLength);

	bool PushModule(const char* moduleName);

	template <class ReturnType, class...Params>
	void RegisterFunction(const std::string& name, ReturnType(func)(Params...)) {
		auto args = args_t<decltype(func)>{};
		ReflectedMethod method = GenerateMethodBinding(name, args, func);
		RegisterFunction(method);
	}
	void RegisterFunction(const ReflectedMethod& method);

	struct CompiledCode {
		char* code = nullptr;
		size_t size = 0;
	};
	//TODO private or rename or getter
	lua_State* L = nullptr;
private:
	lua_CompileOptions* compilerOptions = nullptr;

	std::unordered_map<std::string, CompiledCode> compiledCode;

	bool Init() override;
	void Term() override;

	void TermInternal();

	void RegisterAndRunAll();

	void ReloadScripts();

	void RegisterFunctionInternal(int functionIdx);

	void Update() override;

	static int LuaRequire(lua_State* L);

	std::vector<ReflectedMethod> registeredFunctions;
};