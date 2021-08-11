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
	void UpdateLights(Vector3 poi);
	

	//The surface contained by the window
	SDL_Surface* screenSurface = nullptr;

	bgfx::UniformHandle u_time;
	bgfx::UniformHandle u_color;
	bgfx::UniformHandle s_texColor;
	bgfx::UniformHandle s_texNormal;
	
	static constexpr int maxLightsCount = 8;
	bgfx::UniformHandle u_lightPosRadius;
	bgfx::UniformHandle u_lightRgbInnerR;

	int prevWidth;
	int prevHeight;

	std::shared_ptr<Texture> whiteTexture;
	std::shared_ptr<Texture> defaultNormalTexture;

};