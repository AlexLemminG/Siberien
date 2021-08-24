#pragma once

#include "Serialization.h"
#include "Defines.h"

class Engine {
public:
};

class GameLibrary {
public:
	GameLibrary() {}
	~GameLibrary() {}
	virtual bool Init(Engine* engine) { return true; }
	virtual void Term() {}
	virtual SerializationInfoStorage& GetSerializationInfoStorage() = 0;
};

#define DECLARE_LIBRARY(className) \
extern "C" { __declspec(dllexport) className* __cdecl SEngine_CreateLibrary() { return new className(); } } \
DECLARE_SERIALATION_INFO_STORAGE(); \
SerializationInfoStorage& className::GetSerializationInfoStorage() { return ::GetSerialiationInfoStorage(); }