#pragma once

#include "Serialization.h"
#include "Defines.h"
#include "optick.h"

class SE_CPP_API Engine {
public:
	static Engine* Get();

	std::string GetExeName();
	bool IsEditorMode() const;
};


class SE_CPP_API GameLibrary {
public:
	GameLibrary();
	virtual ~GameLibrary();
	virtual bool Init(Engine* engine);
	virtual void Term();
	virtual GameLibraryStaticStorage& GetStaticStorage() = 0;
};


#define INNER_LIBRARY(className) \
	virtual GameLibraryStaticStorage& GetStaticStorage() override;


#define DEFINE_LIBRARY(className) \
extern "C" { __declspec(dllexport) className* __cdecl SEngine_CreateLibrary() { return new className(); } } \
GameLibraryStaticStorage& ##className::GetStaticStorage() { return GameLibraryStaticStorage::Get(); } \
GameLibraryStaticStorage& GameLibraryStaticStorage::Get() { static GameLibraryStaticStorage instance; return instance; } \
