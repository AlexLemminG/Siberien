#include "Input.h"
#include <SDL.h>

std::vector<SDL_Scancode> Input::justPressed;
std::vector<SDL_Scancode> Input::pressed;
std::vector<SDL_Scancode> Input::justReleased;
bool Input::quitPressed = false;
Vector2 Input::mousePos = Vector2{ 0,0 };

void Input::Update() {
	quitPressed = false;
	justPressed.clear();
	justReleased.clear();

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0) {
		if (e.type == SDL_QUIT)
		{
			quitPressed = true;
		}

		if (e.type == SDL_KEYDOWN) {
			if (GetKey(e.key.keysym.scancode)) {
				continue;
			}
			justPressed.push_back(e.key.keysym.scancode);
			pressed.push_back(e.key.keysym.scancode);
		}

		if (e.type == SDL_KEYUP) {
			pressed.erase(std::find(pressed.begin(), pressed.end(), e.key.keysym.scancode), pressed.end());
			justReleased.push_back(e.key.keysym.scancode);
		}
	}

	int mouseX;
	int mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);
	mousePos.x = mouseX;
	mousePos.y = mouseY;
}
