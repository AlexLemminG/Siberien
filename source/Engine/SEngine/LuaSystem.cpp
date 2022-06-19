#include "LuaSystem.h"
#include "lua.h"
#include "luacode.h"
#include "lualib.h"
#include "Common.h"
#include "Resources.h"
#include "Input.h"

static constexpr char* scriptsFolder = "scripts/";

static int finishrequire(lua_State* L) {
	if (lua_isstring(L, -1))
		lua_error(L);

	return 1;
}

int LuaSystem::LuaRequire(lua_State* L)
{
	std::string name = luaL_checkstring(L, 1);

	if (LuaSystem::Get()->compiledCode.find(name) == LuaSystem::Get()->compiledCode.end()) {
		std::string fileName = scriptsFolder;
		fileName += name;
		fileName += ".lua";//TODO constexpr
		AssetDatabase::Get()->Load(fileName);
	}

	luaL_findtable(L, LUA_REGISTRYINDEX, "_MODULES", 1);

	// return the module from the cache
	lua_getfield(L, -1, name.c_str());
	if (!lua_isnil(L, -1))
		return finishrequire(L);

	return 1;
}

class LuaScript : public Object{
	REFLECT_BEGIN(LuaScript);
	REFLECT_END();
};

class LuaScriptImporter : public AssetImporter {
	virtual bool ImportAll(AssetDatabase_BinaryImporterHandle& databaseHandle) override;
};
DECLARE_BINARY_ASSET(LuaScript, LuaScriptImporter);

REGISTER_SYSTEM(LuaSystem);

bool LuaScriptImporter::ImportAll(AssetDatabase_BinaryImporterHandle& databaseHandle)
{
	std::vector<uint8_t> buffer;
	databaseHandle.ReadAssetAsBinary(buffer);
	std::string name = databaseHandle.GetAssetFileName();
	if (buffer.size() == 0) {
		return false;
	}
	//TODO asserts
	name = name.substr(strlen(scriptsFolder), name.length() - strlen(scriptsFolder) - strlen(".lua"));//TODO constexpr ".lua"
	LuaSystem::Get()->RegisterAndRun(name.c_str(), (char*)&buffer[0], buffer.size());
	databaseHandle.AddAssetToLoaded("script", std::make_shared<LuaScript>());//just a mock to prevent double loading
	return true;
}

bool LuaSystem::RegisterAndRun(const char* moduleName, const char* sourceCode, size_t sourceCodeLength) {
	if (moduleName == nullptr || strlen(moduleName) == 0) {
		ASSERT_FAILED("Trying to register module with empty name");
		return false;
	}
	std::string moduleNameStr = moduleName;
	if ((compiledCode.find(moduleNameStr) != compiledCode.end())) {
		ASSERT_FAILED("Module with name '%s' already registered", moduleNameStr.c_str());
		return false;
	}

	CompiledCode code;
	code.code = luau_compile(sourceCode, sourceCodeLength, NULL, &code.size);
	if (code.code == nullptr) {
		ASSERT_FAILED("Failed to compile module '%s'", moduleNameStr.c_str());
		return false;
	}



	// module needs to run in a new thread, isolated from the rest
	lua_State* GL = lua_mainthread(L);
	lua_State* ML = lua_newthread(GL);
	lua_xmove(GL, L, 1);

	// new thread needs to have the globals sandboxed
	luaL_sandboxthread(ML);

	int loadResult = luau_load(ML, moduleName, code.code, code.size, 0);
	if (loadResult != 0) {
		ASSERT_FAILED("Failed to load module '%s'", moduleNameStr.c_str());
		return false;
	}


	int stackDepth = lua_gettop(ML);
	lua_resume(ML, L, 0);
	if (lua_gettop(ML) == stackDepth) { // pointer to call module changed to pointer to module object
		//TODO additional checks if it is valid return obj
		//TODO check lua_ref + stored in compiledCode map
		luaL_findtable(L, LUA_REGISTRYINDEX, "_MODULES", 1);
		lua_xmove(ML, L, 1);
		lua_setfield(L, -2, moduleName);
		lua_pop(L, 1); //LUA_REGISTRYINDEX
	}
	lua_pop(L, 1); // ML

	compiledCode[moduleNameStr] = code;
	return true;
}

bool LuaSystem::PushModule(const char* moduleName) {
	std::string name = moduleName;

	luaL_findtable(L, LUA_REGISTRYINDEX, "_MODULES", 1);
	// return the module from the cache
	lua_getfield(L, -1, name.c_str());
	lua_remove(L, -2);
	if (!lua_isnil(L, -1)) {
		if (lua_isstring(L, -1))
			lua_error(L);
		return true;
	}
	return false;
}

bool LuaSystem::Init() {
	ASSERT(!L);
	L = luaL_newstate();
	ASSERT(L);
	lua_setsafeenv(L, LUA_ENVIRONINDEX, true);
	luaL_openlibs(L);

	static const luaL_Reg funcs[] = {
		//{"loadstring", lua_loadstring},
		{"require", LuaRequire},
		//{"collectgarbage", lua_collectgarbage},
		{NULL, NULL},
	};

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, funcs);
	lua_pop(L, 1);

	// ***EXAMPLE***
	auto sourceA = R"(local A = {} print("A") function A.pp() print("Hello LuaA!") end A.pp() return A)";
	auto sourceB = R"(print("B") local A = require("A") A.pp())";
	RegisterAndRun("A", sourceA, strlen(sourceA));
	RegisterAndRun("B", sourceB, strlen(sourceB));

	RegisterAndRunAll();

	return true;
}

void LuaSystem::Term() {
	for (auto& kv : compiledCode) {
		free(kv.second.code);
	}
	compiledCode.clear();
	lua_close(L);
	L = nullptr;
}


void LuaSystem::RegisterAndRunAll() {
	auto allAssets = AssetDatabase::Get()->GetAllAssetNames();

	constexpr char* extention = ".lua";
	for (auto assetName : allAssets) {
		//TODO case insensitive
		auto last = assetName.find_last_of(extention);
		if (assetName.size() >= strlen(extention) + strlen(scriptsFolder)
			&& strncmp(assetName.c_str() + assetName.length() - strlen(extention), extention, strlen(extention)) == 0
			&& strncmp(assetName.c_str(), scriptsFolder, strlen(scriptsFolder)) == 0
			) {
			//TODO split between plugins and add prefixes to module names
			AssetDatabase::Get()->Load(assetName);
		}
	}
}

void LuaSystem::ReloadScripts() {
	for (auto& kv : compiledCode) {
		std::string assetName = kv.first;
		assetName = scriptsFolder + assetName + ".lua";
		AssetDatabase::Get()->Unload(assetName);
	}
	Term();//TODO not fully

	Init();
}

void LuaSystem::Update() {
	if (Input::GetKeyDown(SDL_SCANCODE_F7)) {
		//TODO move hotkeys to one place
		ReloadScripts();
	}
}