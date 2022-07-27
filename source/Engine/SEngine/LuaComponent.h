#pragma once

#include "Component.h"
#include "GameEvents.h"
#include "LuaSystem.h"

class LuaScript;
class LuaObjectRef;

class LuaComponent : public Component {
   private:
    LuaObject luaObj;

    void Call(char* funcName);
    void InitLua();
    void TermLua();

   public:
    virtual void OnEnable();
    virtual void Update();
    virtual void FixedUpdate();
    virtual void OnDisable();
    virtual void OnValidate();
    virtual void OnDrawGizmos();

    std::string GetScriptName() const { return luaObj.scriptName; }
    std::vector<char> dataForLua;  // TODO

   private:
    GameEventHandle onBeforeScriptsReloadingHandler;
    GameEventHandle onAfterScriptsReloadingHandler;

    bool onBeforeReloadingCalled = false;
    // std::shared_ptr<Component> GetComponent(const std::string& typeName);
    std::shared_ptr<LuaObjectRef> ref;
    REFLECT_BEGIN(LuaComponent);
    REFLECT_VAR(luaObj);
    REFLECT_METHOD_EXPLICIT("gameObject", static_cast<std::shared_ptr<GameObject> (Component::*)()>(&Component::gameObject));
    REFLECT_END();
};