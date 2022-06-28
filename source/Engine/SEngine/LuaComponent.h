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

private:
	//std::shared_ptr<Component> GetComponent(const std::string& typeName);

	REFLECT_BEGIN(LuaComponent);
	REFLECT_VAR(script);
	REFLECT_METHOD_EXPLICIT("gameObject", static_cast<std::shared_ptr<GameObject>(Component::*)()>(&Component::gameObject));
	REFLECT_END();
};