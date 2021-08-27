#pragma once

#include "System.h"

class GameLibrary;

class GameLibrariesManager {
public:
	bool Init();
	void Term();
private:
	class LibraryHandle {
	public:
		std::string name;
		std::shared_ptr<GameLibrary> library;
		void* objectHandle = nullptr;
	};
	std::vector<LibraryHandle> libraries;
};