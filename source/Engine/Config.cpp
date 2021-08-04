#include "Common.h"
#include "Config.h"

#include <fstream>

static YAML::Node config;

int CfgGetInt(std::string name) {
	SDL_assert(config[name]);
	return config[name].as<int>();
}

void CfgSetInt(std::string name, int i) {
	SDL_assert(config[name]);
	config[name] = i;
}

bool Config::Init() {
	config = YAML::LoadFile("config.yaml");
	return true;
}

void Config::Term() {
	std::ofstream fout("config.yaml");
	fout << config;
}
