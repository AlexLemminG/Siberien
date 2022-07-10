#pragma once

#include "System.h"
#include "GameEvents.h"
#include "Asserts.h"

struct lua_State;
struct lua_CompileOptions;

//Dummy class for inspector to reinterpret memory according to type
class LuaObjectRaw {

	REFLECT_BEGIN(LuaObjectRaw);
	REFLECT_END();
};

namespace Luau {
	class Frontend;
}

class LuaObject : public LuaObjectRaw {
public:
	std::string scriptName;

	std::vector<char> data;
	mutable std::unique_ptr<ReflectedTypeNonTemplated> type;

	static ReflectedTypeBase* TypeOf();
	virtual	ReflectedTypeBase* GetType() const;
};

class LuaSystem : public System<LuaSystem> {
	friend class LuaComponent;
public:
	bool RegisterAndRun(const char* moduleName, const char* sourceCode, size_t sourceCodeLength);

	bool PushModule(const char* moduleName);

	template <class ReturnType, class...Params>
	void RegisterFunction(const std::string& name, ReturnType(*func)(Params...)) {
		RegisterFunction(name, std::function<ReturnType(Params...)>(func));
	}
	template <class ReturnType, class...Params>
	void RegisterFunction(const std::string& name, std::function<ReturnType(Params...)> func) {
		auto args = args_t<ReturnType(*)(Params...)>{};
		ReflectedMethod method = GenerateMethodBinding(name, args, func);
		RegisterFunction(method);
	}
	void RegisterFunction(const ReflectedMethod& method);

	std::unique_ptr<ReflectedTypeNonTemplated> ConstructLuaObjectType(const std::string& scriptName);

	GameEvent<> onBeforeScriptsReloading;
	GameEvent<> onAfterScriptsReloading;

	struct CompiledCode {
		char* code = nullptr;
		size_t size = 0;
	};
	//TODO private or rename or getter
	lua_State* L = nullptr;
private:
	lua_CompileOptions* compilerOptions = nullptr;
	Luau::Frontend* frontend = nullptr;

	std::unordered_map<std::string, CompiledCode> compiledCode;

	bool Init() override;
	void Term() override;

	void TermLua();
	bool InitLua();

	void RegisterAndRunAll();

	void ReloadScripts();

	void RegisterFunctionInternal(int functionIdx);

	void Update() override;

	static int LuaRequire(lua_State* L);

	std::vector<ReflectedMethod> registeredFunctions;
};