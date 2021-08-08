#pragma once

#include "Math.h"
#include "MeshRenderer.h"

struct SDL_Window;
class SDL_Surface;
class SystemsManager;

class Render {
public:
	bool Init();
	void Draw(SystemsManager& systems);
	void Term();

	//The window we'll be rendering to
	//TODO make non static
	static SDL_Window* window;
private:
	void DrawMesh(MeshRenderer* renderer);
	

	//The surface contained by the window
	SDL_Surface* screenSurface = nullptr;

	bgfx::UniformHandle u_time;
	
	int prevWidth;
	int prevHeight;


};