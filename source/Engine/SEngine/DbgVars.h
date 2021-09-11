#pragma once
#include "System.h"

//TODO non static
template<typename T>
class DbgVar {
public:
	T* val;
	std::string path;
	T defaultValue;
	DbgVar(T* b, std::string path, T defaultValue) :val(b), path(path) {
		DbgVarsSystem::AddDbgVar(this);
	}
};
using DbgVarBool = DbgVar<bool>;

class DbgVarsSystem : public System<DbgVarsSystem> {
public:
	virtual bool Init()override;
	virtual void Update() override;

	static void AddDbgVar(DbgVarBool* dbgvar);
};


#define DBG_VAR_BOOL(varName, path, defaultValue) \
bool varName = defaultValue; \
namespace{ \
DbgVarBool dbgVar(const_cast<bool*>(&varName), path, defaultValue); \
}
