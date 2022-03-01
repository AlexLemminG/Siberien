#include "Common.h"
#include "Input.h"
#include <SDL.h>
#include <dear-imgui/imgui_impl_sdl.h>

std::vector<bool> Input::justPressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
std::vector<bool> Input::pressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
std::vector<bool> Input::justReleased = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
bool Input::quitPressed = false;
Uint32 Input::mouseState = 0;
Uint32 Input::prevMouseState = 0;
Vector2 Input::mousePos = Vector2{ 0,0 };
Vector2 Input::mouseDeltaPos = Vector2{ 0,0 };
float Input::mouseScrollY = 0.f;

bool Input::Init() {
	OPTICK_EVENT();
	justPressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
	pressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
	justReleased = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);

	return true;
}

void Input::Term() {
	OPTICK_EVENT();
	justPressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
	pressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
	justReleased = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
}

void Input::Update() {
	OPTICK_EVENT();
	mouseDeltaPos = Vector2{ 0,0 };

	quitPressed = false;
	justPressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
	justReleased = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
	mouseScrollY = 0.f;

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0) {
		ImGui_ImplSDL2_ProcessEvent(&e);
		if (e.type == SDL_QUIT)
		{
			quitPressed = true;
		}
		else
			if (e.type == SDL_MOUSEMOTION) {
				mouseDeltaPos.x += e.motion.xrel;
				mouseDeltaPos.y += e.motion.yrel;
			}
			else if (e.type == SDL_MOUSEWHEEL) {
				mouseScrollY += e.wheel.y;
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

	auto prevMousePos = mousePos;
	mousePos.x = mouseX;
	mousePos.y = mouseY;
	//mouseDeltaPos = mousePos - prevMousePos;
}

bool Input::GetKeyDown(SDL_Scancode code) {
	return justPressed[code];
}

bool Input::GetKey(SDL_Scancode code) {
	return pressed[code];
}

bool Input::GetKeyUp(SDL_Scancode code) {
	return justReleased[code];
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

float Input::GetMouseScrollY() {
	return Input::mouseScrollY;
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

Vector2 Input::GetMousePosition() {
	return mousePos;
}

Vector2 Input::GetMouseDeltaPosition() {
	return mouseDeltaPos;
}

bool Input::GetQuit() {
	return quitPressed;
}
