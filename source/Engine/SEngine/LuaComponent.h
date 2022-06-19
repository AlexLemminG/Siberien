#pragma once

#include "Component.h"

class LuaScript;

class LuaComponent : public Component {
private:
	std::string script;
	int ref = 0;

	void Call(char* funcName);
public:
	virtual void OnEnable();
	virtual void Update();
	virtual void FixedUpdate();
	virtual void OnDisable();
	virtual void OnValidate();
	virtual void OnDrawGizmos();

	REFLECT_BEGIN(LuaComponent);
	REFLECT_VAR(script);
	REFLECT_END();
};