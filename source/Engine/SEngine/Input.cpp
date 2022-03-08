#include "Common.h"
#include "Input.h"
#include <SDL.h>
#include <dear-imgui/imgui_impl_sdl.h>

static std::vector<bool> justPressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
static std::vector<bool> pressed = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
static std::vector<bool> justReleased = std::vector<bool>(((int)SDL_Scancode::SDL_NUM_SCANCODES), false);
static bool quitPressed = false;
static Uint32 mouseState = 0;
static Uint32 prevMouseState = 0;
static Vector2 mousePos = Vector2{ 0,0 };
static Vector2 mouseDeltaPos = Vector2{ 0,0 };
static float mouseScrollY = 0.f;

bool Input::Init() {
	OPTICK_EVENT();
	std::fill(justPressed.begin(), justPressed.end(), false);
	std::fill(pressed.begin(), pressed.end(), false);
	std::fill(justReleased.begin(), justReleased.end(), false);

	return true;
}

void Input::Term() {
	OPTICK_EVENT();
	std::fill(justPressed.begin(), justPressed.end(), false);
	std::fill(pressed.begin(), pressed.end(), false);
	std::fill(justReleased.begin(), justReleased.end(), false);
}

void Input::Update() {
	OPTICK_EVENT();
	mouseDeltaPos = Vector2{ 0,0 };

	quitPressed = false;
	std::fill(justPressed.begin(), justPressed.end(), false);
	std::fill(justReleased.begin(), justReleased.end(), false);
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

	//TODO is it possible to do same stuff using pollevents instead of checking every key every frame?
	auto keyboardState = SDL_GetKeyboardState(NULL);
	for (int iKey = 0; iKey < SDL_Scancode::SDL_NUM_SCANCODES; iKey++) {
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
	return mouseScrollY;
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
