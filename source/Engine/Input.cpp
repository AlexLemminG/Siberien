#include "Common.h"
#include "Input.h"
#include <SDL.h>

std::vector<bool> Input::justPressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
std::vector<bool> Input::pressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
std::vector<bool> Input::justReleased = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
bool Input::quitPressed = false;
Uint32 Input::mouseState = 0;
Uint32 Input::prevMouseState = 0;
Vector2 Input::mousePos = Vector2{ 0,0 };

void Input::Update() {
	OPTICK_EVENT();
	quitPressed = false;
	justPressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
	justReleased = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0) {
		if (e.type == SDL_QUIT)
		{
			quitPressed = true;
		}
	}

	auto keyboardState = SDL_GetKeyboardState(NULL);
	for (int iKey = 0; iKey < SDL_Scancode::SDL_NUM_SCANCODES; iKey++) {
		justPressed[iKey] = false;
		justReleased[iKey] = false;
		if (keyboardState[iKey]) {
			if (!pressed[iKey]) {
				justPressed[iKey] = true;
			}
			pressed[iKey] = true;
		}
		else {
			if (pressed[iKey]) {
				justReleased[iKey] = true;
			}
			pressed[iKey] = false;
		}
	}

	int mouseX;
	int mouseY;
	prevMouseState = mouseState;
	mouseState = SDL_GetMouseState(&mouseX, &mouseY);
	mousePos.x = mouseX;
	mousePos.y = mouseY;
}

bool Input::GetMouseButton(int button) {
	switch (button)
	{
	case 0:
		return mouseState & SDL_BUTTON_LMASK;
	case 1:
		return mouseState & SDL_BUTTON_RMASK;
	default:
		return false;
	}
}

bool Input::GetMouseButtonDown(int button) {
	switch (button)
	{
	case 0:
		return GetMouseButton(button) && !(prevMouseState & SDL_BUTTON_LMASK);
	case 1:
		return GetMouseButton(button) && !(prevMouseState & SDL_BUTTON_RMASK);
	default:
		return false;
	}
}
