#pragma once

#include <vector>
#include "SDL_scancode.h"

class Input {
public:

	static bool Init() {
		justPressed.clear();
		pressed.clear();
		justReleased.clear();
		return true;
	}

	static void Term() {
		justPressed.clear();
		pressed.clear();
		justReleased.clear();
	}

	static void Update();


	static bool GetKeyDown(SDL_Scancode code) {
		return std::find(justPressed.begin(), justPressed.end(), code) != justPressed.end();
	}
	static bool GetKey(SDL_Scancode code) {
		return (std::find(pressed.begin(), pressed.end(), code) != pressed.end()) || GetKeyDown(code);
	}
	static bool GetKeyUp(SDL_Scancode code) {
		return std::find(justReleased.begin(), justReleased.end(), code) != justReleased.end();
	}

	static bool GetQuit() {
		return quitPressed;
	}

private:
	//TODO hashmap
	static std::vector<SDL_Scancode> justPressed;
	static std::vector<SDL_Scancode> pressed;
	static std::vector<SDL_Scancode> justReleased;
	static bool quitPressed;
};