#pragma once

#include <vector>
#include "SDL_scancode.h"
#include "Math.h"

class Input {
public:

	static bool Init() {
		justPressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
		pressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
		justReleased = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);

		return true;
	}

	static void Term() {
		justPressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
		pressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
		justReleased = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
	}

	static void Update();


	static bool GetKeyDown(SDL_Scancode code) {
		return justPressed[code];
	}
	static bool GetKey(SDL_Scancode code) {
		return pressed[code];
	}
	static bool GetKeyUp(SDL_Scancode code) {
		return justReleased[code];
	}

	static bool GetMouseButton(int button);
	static bool GetMouseButtonDown(int button);

	static Vector2 GetMousePosition() {
		return mousePos;
	}

	static bool GetQuit() {
		return quitPressed;
	}

private:
	//TODO hashmap
	static std::vector<bool> justPressed;
	static std::vector<bool> pressed;
	static std::vector<bool> justReleased;
	static bool quitPressed;
	static Vector2 mousePos;
	static Uint32 mouseState;
	static Uint32 prevMouseState;
};