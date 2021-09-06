#pragma once

#include "yaml-cpp/yaml.h"
//#include "SMath.h"

int CfgGetInt(std::string name);
float CfgGetFloat(std::string name);
bool CfgGetBool(std::string name);
std::string CfgGetString(std::string name);
YAML::Node CfgGetNode(std::string name);

int SettingsGetInt(std::string name);
void SettingsSetInt(std::string name, int i);

class Config {
public:
	bool Init();
	void Term();
};