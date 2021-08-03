#pragma once

#include "yaml-cpp/yaml.h"
//#include "Math.h"

extern YAML::Node config;

int CfgGetInt(std::string name);
//Color CfgGetColor(std::string name);