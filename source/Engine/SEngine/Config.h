#pragma once

#include "ryml.hpp"//TODO try not to include this fatty
//#include "SMath.h"
//TODO remove export
//TODO consts and char* instead of string

ryml::Tree RymlParseFile(std::string fileName);//TODO not here

SE_CPP_API int CfgGetInt(std::string name);
SE_CPP_API float CfgGetFloat(std::string name);
SE_CPP_API bool CfgGetBool(std::string name);
SE_CPP_API std::string CfgGetString(std::string name);
SE_CPP_API const ryml::NodeRef CfgGetNode(std::string name);

SE_CPP_API int SettingsGetInt(std::string name);
SE_CPP_API void SettingsSetInt(std::string name, int i);

class Config {
public:
	bool Init();
	void Term();
};