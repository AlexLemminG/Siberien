#include "Common.h"
#include "Config.h"

#include <fstream>

static YAML::Node config;
static YAML::Node settings;

int CfgGetInt(std::string name) {
	SDL_assert(config[name]);
	return config[name].as<int>();
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
	config = YAML::LoadFile("config.yaml");
	settings = YAML::LoadFile("settings.yaml");
	return true;
}

void Config::Term() {
	std::ofstream fout("settings.yaml");
	fout << settings;
}
