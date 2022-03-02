#pragma once
#include "System.h"

class DbgVarBool;
class DbgVarTrigger;

class DbgVarsSystem : public System<DbgVarsSystem> {
public:
	virtual bool Init()override;
	virtual void Update() override;

	static void AddDbgVar(DbgVarBool* dbgvar);
	static void AddDbgVar(DbgVarTrigger* dbgvar);
};

//TODO non static
template<typename T>
class DbgVar {
public:
	T* val;
	std::string path;
	T defaultValue;
	DbgVar(T* b, std::string path, T defaultValue) :val(b), path(path), defaultValue(defaultValue) {}
};

class DbgVarBool : public DbgVar<bool> {
public:
	DbgVarBool(bool* b, std::string path, bool defaultValue) :DbgVar<bool>(b, path, defaultValue) {
		DbgVarsSystem::AddDbgVar(this);
	}
};

class DbgVarTrigger : public  DbgVar<bool> {
public:
	DbgVarTrigger(bool* b, std::string path) :DbgVar<bool>(b, path, false) {
		DbgVarsSystem::AddDbgVar(this);
	}
};

//TODO is it possible to optimize it later?
#define DBG_VAR_BOOL_EXTERN(varName) \
extern bool varName;

#define DBG_VAR_BOOL(varName, path, defaultValue) \
bool varName = defaultValue; \
namespace{ \
DbgVarBool dbgVar_##varName(const_cast<bool*>(&varName), path, defaultValue); \
}

//stays true for 1 frame
#define DBG_VAR_TRIGGER(varName, path) \
bool varName = false; \
namespace{ \
DbgVarTrigger dbgVar_##varName(const_cast<bool*>(&varName), path); \
}