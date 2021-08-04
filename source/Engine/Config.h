#pragma once

#include "yaml-cpp/yaml.h"
//#include "Math.h"

int CfgGetInt(std::string name);
void CfgSetInt(std::string name, int i);

class Config {
public:
	bool Init();
	void Term();
};