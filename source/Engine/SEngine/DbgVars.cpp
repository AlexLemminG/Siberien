#include "DbgVars.h"
#include "dear-imgui/imgui.h"
#include "Config.h"
#include "Input.h"
#include "ryml.hpp"
#include "Resources.h"
#include <fstream>

std::vector<DbgVarTrigger*>& GetTriggerVars() {
	static std::vector<DbgVarTrigger*> triggerVars;
	return triggerVars;
}
std::vector<DbgVarBool*>& GetBoolVars() {
	static std::vector<DbgVarBool*> boolVars;
	return boolVars;
}

REGISTER_SYSTEM(DbgVarsSystem);

static bool dbgVarsShown = false;

bool DbgVarsSystem::Init() {
	//TODO disable in retail
	for (auto var : GetBoolVars()) {
		*var->val = var->defaultValue;
	}
	for (auto var : GetTriggerVars()) {
		*var->val = var->defaultValue;
	}

	if (CfgGetBool("godMode")) {
		Load();
	}

	return true;
}

void DbgVarsSystem::Save() const {
	SerializationContext context;
	for (auto var : GetBoolVars()) {
		context.Child(var->varName) << *var->val;
	}

	std::ofstream fout("dbgvars.yaml");
	fout << context.GetYamlNode();
}

void DbgVarsSystem::Load() {
	std::ifstream input("dbgvars.yaml", std::ios::binary | std::ios::ate);
	if (input) {
		std::vector<char> buffer;
		std::streamsize size = input.tellg();
		input.seekg(0, std::ios::beg);

		ResizeVectorNoInit(buffer, size);
		input.read((char*)buffer.data(), size);
		auto tree = ryml::parse(c4::csubstr(&buffer[0], buffer.size()));


		const SerializationContext context{ tree };

		for (auto var : GetBoolVars()) {
			if (context.Child(var->varName).IsDefined()) {
				context.Child(var->varName) >> *var->val;
			}
		}
	}
}

void DbgVarsSystem::Update() {
	//TODO CfgVars
	if (!CfgGetBool("godMode")) {
		return;
	}

	if (Input::GetKeyDown(SDL_SCANCODE_F2)) {
		dbgVarsShown = !dbgVarsShown;
	}
	bool changed = false;
	if (dbgVarsShown) {
		ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Once);
		ImGui::Begin("dbg vars", &dbgVarsShown);
		for (auto var : GetBoolVars()) {
			changed |= ImGui::Checkbox(var->path.c_str(), var->val);
		}
		for (auto var : GetTriggerVars()) {
			*var->val = ImGui::Button(var->path.c_str());
		}
		ImGui::End();
	}

	for (auto var : GetBoolVars()) {
		changed |= *var->val != var->prevVal;
		var->prevVal = *var->val;
	}

	if (changed) {
		Save();
	}
}

void DbgVarsSystem::Term() {
	if (CfgGetBool("godMode")) {
		Save();
	}
}

void DbgVarsSystem::AddDbgVar(DbgVarBool* dbgvar) {
	GetBoolVars().push_back(dbgvar);
}
void DbgVarsSystem::AddDbgVar(DbgVarTrigger* dbgvar) {
	GetTriggerVars().push_back(dbgvar);
}
