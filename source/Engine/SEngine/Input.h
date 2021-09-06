#pragma once

#include <vector>
#include "SDL_scancode.h"
#include "SMath.h"

class SE_CPP_API Input {
public:

	static bool Init();

	static void Term();

	static void Update();


	static bool GetKeyDown(SDL_Scancode code);
	static bool GetKey(SDL_Scancode code);
	static bool GetKeyUp(SDL_Scancode code);

	static bool GetMouseButton(int button);
	static bool GetMouseButtonDown(int button);

	static Vector2 GetMousePosition();
	static Vector2 GetMouseDeltaPosition();

	static bool GetQuit();

private:
	//TODO hashmap
	static std::vector<bool> justPressed;
	static std::vector<bool> pressed;
	static std::vector<bool> justReleased;
	static bool quitPressed;
	static Vector2 mousePos;
	static Vector2 mouseDeltaPos;
	static Uint32 mouseState;
	static Uint32 prevMouseState;
};