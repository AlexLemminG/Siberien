#include "Common.h"
#include "Config.h"
#include "ryml.hpp"
#include "ryml_std.hpp"

#include <fstream>

static ryml::Tree config;
static ryml::Tree settings;

ryml::Tree RymlParseFile(std::string fileName) {
	std::ifstream input(fileName, std::ios::binary | std::ios::ate);
	if (input) {
		std::vector<char> buffer;
		std::streamsize size = input.tellg();
		input.seekg(0, std::ios::beg);

		ResizeVectorNoInit(buffer, size);
		input.read((char*)buffer.data(), size);
		return ryml::parse(c4::csubstr(&buffer[0], buffer.size()));
	}
	return ryml::Tree();
}

int CfgGetInt(std::string name) {
	int i;
	CfgGetNode(name) >> i;
	return i;
}

float CfgGetFloat(std::string name) {
	float f;
	CfgGetNode(name) >> f;
	return f;
}

bool CfgGetBool(std::string name) {
	bool b;
	CfgGetNode(name) >> b;
	return b;
}

const ryml::NodeRef CfgGetNode(std::string name) {
	auto nodeRef = config[c4::to_substr(name)];
	ASSERT(nodeRef.valid());
	return nodeRef;
}

std::string CfgGetString(std::string name) {
	c4::csubstr s;
	CfgGetNode(name) >> s;
	return std::string(s.str, s.len);;//TODO what if nullptr?
}
//TODO remove settings and use assets instead
//TODO or at least do not save/load them to git folder
int SettingsGetInt(std::string name) {
	auto nodeRef = settings[c4::to_substr(name)];
	ASSERT(nodeRef.valid());

	int i;
	nodeRef >> i;
	return i;
}

void SettingsSetInt(std::string name, int i) {
	auto nodeRef = settings[c4::to_substr(name)];
	ASSERT(nodeRef.valid());
	
	SDL_assert(nodeRef.valid());
	nodeRef << i;
}

bool Config::Init() {
	OPTICK_EVENT();
	config = RymlParseFile("config.yaml");
	settings = RymlParseFile("settings.yaml"); 
	return true;
}

void Config::Term() {
	OPTICK_EVENT();
	std::ofstream fout("settings.yaml");
	fout << settings;
}
