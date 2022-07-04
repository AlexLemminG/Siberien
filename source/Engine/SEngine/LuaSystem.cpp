#include "LuaSystem.h"
#include "lua.h"
#include "luacode.h"
#include "lualib.h"
#include "Common.h"
#include "Resources.h"
#include "Input.h"
#include "LuaReflect.h"

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

class LuaScript : public Object {
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
	code.code = luau_compile(sourceCode, sourceCodeLength, compilerOptions, &code.size);
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
	return InitLua();
}
void LuaSystem::TermLua() {
	for (auto& kv : compiledCode) {
		free(kv.second.code);
	}
	compiledCode.clear();
	lua_close(L);
	L = nullptr;
	SAFE_DELETE(compilerOptions);
}
bool LuaSystem::InitLua() {
	ASSERT(!compilerOptions);
	compilerOptions = new lua_CompileOptions();
	compilerOptions->optimizationLevel = 1;
	compilerOptions->debugLevel = 1;
	compilerOptions->coverageLevel = 0;

	compilerOptions->vectorLib = nullptr;
	compilerOptions->vectorCtor = "vector";

	compilerOptions->mutableGlobals = nullptr;

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
	//auto sourceA = R"(local A = {} print("A") function A.pp() print("Hello LuaA!") end A.pp() return A)";
	//auto sourceB = R"(print("B") local A = require("A") A.pp())";
	//RegisterAndRun("A", sourceA, strlen(sourceA));
	//RegisterAndRun("B", sourceB, strlen(sourceB));

	RegisterAndRunAll();

	for (int i = 0; i < registeredFunctions.size(); i++) {
		RegisterFunctionInternal(i);
	}

	return true;
}


void LuaSystem::Term() {
	TermLua();
	registeredFunctions.clear();
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
	onBeforeScriptsReloading.Invoke();

	for (auto& kv : compiledCode) {
		std::string assetName = kv.first;
		assetName = scriptsFolder + assetName + ".lua";
		AssetDatabase::Get()->Unload(assetName);
	}
	TermLua();

	InitLua();
	onAfterScriptsReloading.Invoke();

}

void LuaSystem::Update() {
	if (Input::GetKeyDown(SDL_SCANCODE_F7)) {
		//TODO move hotkeys to one place
		ReloadScripts();
	}
	lua_gc(L, LUA_GCCOLLECT, 0);
}

void LuaSystem::RegisterFunction(const ReflectedMethod& method) {
	registeredFunctions.push_back(method);
	if (L != nullptr) {
		RegisterFunctionInternal(registeredFunctions.size() - 1);
	}
}
void LuaSystem::RegisterFunctionInternal(int functionIdx) {
	ASSERT(L);
	if (functionIdx < 0 || functionIdx >= registeredFunctions.size()) {
		ASSERT("Illegal RegisterFunctionInternal functionIdx");
		return;
	}

	auto method = registeredFunctions[functionIdx];
	static auto callFunc = [](lua_State* L) {
		int i = (int)lua_tonumber(L, lua_upvalueindex(1));
		const ReflectedMethod& func = LuaSystem::Get()->registeredFunctions[i];
		return Luna<void>::CallFunctionFromLua(LuaSystem::Get()->L, func);
	};

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	lua_pushnumber(L, functionIdx);
	lua_pushcclosure(L, callFunc, method.name.c_str(), 1);
	lua_setfield(L, -2, method.name.c_str());

	lua_pop(L, 1);
}


class ReflectedLuaType : public ReflectedTypeNonTemplated {
	//TODO make sure ReflectedTypeBase has virtual destructor
	//TODO destructor should delete inner ReflectedLuaType types
public:
	//TODO make sure nobody uses name to compare types
	ReflectedLuaType() : ReflectedTypeNonTemplated("LuaObjectRaw") {}
	ReflectedLuaType(const std::string& name) : ReflectedTypeNonTemplated(name) {}
	std::vector<std::unique_ptr<ReflectedTypeBase>> dynamicFieldTypes;

	virtual void Construct(void* ptr) const override {
		for (auto field : fields) {
			field.type->Construct(field.GetPtr(ptr));//TODO default values from lua
		}
	}
	virtual void Serialize(SerializationContext& context, const void* object) const override {
		for (auto field : fields) {
			field.type->Serialize(context.Child(field.name), field.GetPtr(object));
		}
	}
	virtual void Deserialize(const SerializationContext& context, void* object) const override {
		for (auto field : fields) {
			field.type->Deserialize(context.Child(field.name), field.GetPtr(object));
		}
	}

	//TODO it is not applicable here!!!
	virtual size_t SizeOf() const {
		size_t size = 0;
		for (auto field : fields) {
			size += field.type->SizeOf();
		}
		return size;
	};
};

class ReflectedLuaTypeWithScript : public ReflectedLuaType {
public:
	std::string scriptName;

	virtual void Serialize(SerializationContext& context, const void* object) const override {
		auto obj = (LuaObject*)object;
		context.Child("scriptName") << obj->scriptName;
		auto dynamicType = obj->GetType();
		if (dynamicType) {
			ReflectedLuaType::Serialize(context, obj->data.data());
		}
	}
	virtual void Deserialize(const SerializationContext& context, void* object) const override {
		auto obj = (LuaObject*)object;
		context.Child("scriptName") >> obj->scriptName;
		auto dynamicType = obj->GetType();
		if (dynamicType) {
			dynamicType->Construct(object);
			const auto dataContext = context.Child("data");
			for (auto field : dynamicType->fields) {
				field.type->Deserialize(dataContext.Child(field.name), field.GetPtr(obj->data.data()));
			}
		}
	}

	ReflectedLuaTypeWithScript() : ReflectedLuaType("LuaObject") {}
	virtual void Construct(void* ptr) const override {
		auto obj = (LuaObject*)ptr;
		obj->data.resize(SizeOf());
		for (auto field : fields) {
			field.type->Construct(field.GetPtr(obj->data.data()));
		}
	}
};

std::unique_ptr<ReflectedTypeNonTemplated> ConstructLuaObjectType(lua_State* L, int idx, std::string scriptName) {

	/* table is in the stack at index 't' */
	lua_pushnil(L);  /* first key */
	int tableIdx = idx - 1;

	std::vector<std::unique_ptr<ReflectedTypeBase>> dynamicFieldTypes;
	auto fields = std::vector<ReflectedField>();
	int currentOffset = 0;
	while (lua_next(L, tableIdx) != 0) {
		/* uses 'key' (at index -2) and 'value' (at index -1) */
		auto key = lua_tostring(L, -2);
		//printf("%s ", key);
		//printf("%s - %s\n",
		//	lua_typename(L, lua_type(L, -2)),
		//	lua_typename(L, lua_type(L, -1)));
		/* removes 'value'; keeps 'key' for next iteration */


		auto valueType = lua_type(L, -1);
		ReflectedTypeBase* type = nullptr;
		if (strcmp(key, "__index") == 0) {
			//skipping
		}else
		if (valueType == lua_Type::LUA_TNUMBER) {
			type = GetReflectedType<float>();
		}
		else if (valueType == lua_Type::LUA_TTABLE) {
			auto unique_type = ConstructLuaObjectType(L, -1, "");
			type = unique_type.get();
			dynamicFieldTypes.push_back(std::move(unique_type));
		}
		else if (valueType == lua_Type::LUA_TSTRING) {
			type = GetReflectedType<std::string>();
		}
		else if (valueType == lua_Type::LUA_TBOOLEAN) {
			type = GetReflectedType<bool>();
		}
		else if (valueType == lua_Type::LUA_TVECTOR) {
			type = GetReflectedType<Vector3>();
		}

		if (type != nullptr) {
			fields.push_back(ReflectedField(type, key, currentOffset));
			currentOffset += type->SizeOf();
		}
		lua_pop(L, 1);
	}

	std::unique_ptr<ReflectedLuaType> type;
	if (scriptName.empty()) {
		type = std::make_unique<ReflectedLuaType>();
	}
	else {
		auto t = std::make_unique<ReflectedLuaTypeWithScript>();
		t->scriptName = scriptName;
		type = std::move(t);
	}
	type->fields = fields;
	type->dynamicFieldTypes = std::move(dynamicFieldTypes);

	//lua_settable
	return type;
}
std::unique_ptr<ReflectedTypeNonTemplated> LuaSystem::ConstructLuaObjectType(const std::string& scriptName) {
	PushModule(scriptName.c_str());
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return nullptr;
	}

	//TODO non table support
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return nullptr;
	}

	auto type = ::ConstructLuaObjectType(L, -1, scriptName);
	lua_pop(L, 1);
	return type;
}

ReflectedTypeBase* LuaObject::TypeOf() {
	static ReflectedLuaTypeWithScript type;
	return &type;
}
ReflectedTypeBase* LuaObject::GetType() const {
	if (type == nullptr) {
		type = std::move(LuaSystem::Get()->ConstructLuaObjectType(scriptName));
	}
	return type.get();
}