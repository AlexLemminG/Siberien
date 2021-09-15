#include "DbgVars.h"
#include "dear-imgui/imgui.h"
#include "Config.h"
#include "Input.h"

static std::vector<DbgVarBool*> boolVars;
static std::vector<DbgVarTrigger*> triggerVars;

REGISTER_SYSTEM(DbgVarsSystem);

static bool dbgVarsShown = false;

bool DbgVarsSystem::Init() {
	//TODO disable in retail
	for (auto var : boolVars) {
		*var->val = var->defaultValue;
	}
	for (auto var : triggerVars) {
		*var->val = var->defaultValue;
	}
	return true;
}

void DbgVarsSystem::Update() {
	//TODO CfgVars
	if (!CfgGetBool("godMode")) {
		return;
	}

	if (Input::GetKeyDown(SDL_SCANCODE_F2)) {
		dbgVarsShown = !dbgVarsShown;
	}

	if (dbgVarsShown) {
		ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Once);
		ImGui::Begin("dbg vars", &dbgVarsShown);
		for (auto var : boolVars) {
			ImGui::Checkbox(var->path.c_str(), var->val);
		}
		for (auto var : triggerVars) {
			*var->val = ImGui::Button(var->path.c_str());
		}
		ImGui::End();
	}
}

void DbgVarsSystem::AddDbgVar(DbgVarBool* dbgvar) {
	boolVars.push_back(dbgvar);
}
void DbgVarsSystem::AddDbgVar(DbgVarTrigger* dbgvar) {
	triggerVars.push_back(dbgvar);
}
