#pragma once

#include "yaml-cpp/yaml.h"
//#include "SMath.h"
//TODO remove export

SE_CPP_API int CfgGetInt(std::string name);
SE_CPP_API float CfgGetFloat(std::string name);
SE_CPP_API bool CfgGetBool(std::string name);
SE_CPP_API std::string CfgGetString(std::string name);
SE_CPP_API YAML::Node CfgGetNode(std::string name);

SE_CPP_API int SettingsGetInt(std::string name);
SE_CPP_API void SettingsSetInt(std::string name, int i);

class Config {
public:
	bool Init();
	void Term();
};