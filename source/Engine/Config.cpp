#include "Common.h"
#include "Config.h"

#include <fstream>

static YAML::Node config;
static YAML::Node settings;

int CfgGetInt(std::string name) {
	SDL_assert(config[name]);
	return config[name].as<int>();
}

float CfgGetFloat(std::string name) {
	SDL_assert(config[name]);
	return config[name].as<float>();
}

bool CfgGetBool(std::string name) {
	SDL_assert(config[name]);
	return config[name].as<bool>();
}

YAML::Node CfgGetNode(std::string name) {
	SDL_assert(config[name]);
	return config[name];
}

std::string CfgGetString(std::string name) {
	SDL_assert(config[name]);
	return config[name].as<std::string>();
}

int SettingsGetInt(std::string name) {
	SDL_assert(settings[name]);
	return settings[name].as<int>();
}

void SettingsSetInt(std::string name, int i) {
	SDL_assert(settings[name]);
	settings[name] = i;
}

bool Config::Init() {
	OPTICK_EVENT();
	config = YAML::LoadFile("config.yaml");
	settings = YAML::LoadFile("settings.yaml");
	return true;
}

void Config::Term() {
	OPTICK_EVENT();
	std::ofstream fout("settings.yaml");
	fout << settings;
}
