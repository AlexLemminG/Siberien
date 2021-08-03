#pragma once

#include "Math.h"

class SDL_Window;
class SDL_Surface;

class Render {
public:
	bool Init();
	void Draw();
	void Term();

private:
	//The window we'll be rendering to
	SDL_Window* window = nullptr;

	//The surface contained by the window
	SDL_Surface* screenSurface = nullptr;
};