#include "Common.h"
#include "Config.h"
#include "Math.h"

int CfgGetInt(std::string name) {
	SDL_assert(config[name]);
	return config[name].as<int>();
}

Color CfgGetColor(std::string name) {
	SDL_assert(config[name]);
	int rgbaColor = config[name].as<int>();
	return Color::FromIntRGBA(rgbaColor);
}