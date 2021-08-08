#pragma once

#include "yaml-cpp/yaml.h"
//#include "Math.h"

int CfgGetInt(std::string name);
std::string CfgGetString(std::string name);
int SettingsGetInt(std::string name);
void SettingsSetInt(std::string name, int i);

class Config {
public:
	bool Init();
	void Term();
};