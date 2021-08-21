#pragma once

#include "Math.h"
#include "bgfx/bgfx.h"

struct SDL_Window;
class SDL_Surface;
class SystemsManager;
class PostProcessingEffect;
class Texture;
class Material;
class MeshRenderer;
class Mesh;

class Render {
public:
	bool Init();
	void Draw(SystemsManager& systems);
	void Term();

	int GetWidth() { return prevWidth; }
	int GetHeight() { return prevHeight; }
	bgfx::UniformHandle GetPixelSizeUniform() { return u_pixelSize; }
	bgfx::UniformHandle GetTexColorSampler() { return s_texColor; }
	bgfx::UniformHandle GetEmissiveColorSampler() { return s_texEmissive; }
	bgfx::UniformHandle GetPlayerHealthParamsUniform() { return u_playerHealthParams; }
	bgfx::FrameBufferHandle GetFullScreenBuffer() { return m_fullScreenTex; }

	//The window we'll be rendering to
	//TODO make non static
	static SDL_Window* window;
private:
	void DrawMesh(const MeshRenderer* renderer, bool clearState, bool updateState);
	void UpdateLights(Vector3 poi);
	
	bool IsFullScreen();
	void SetFullScreen(bool isFullScreen);

	//The surface contained by the window
	SDL_Surface* screenSurface = nullptr;

	bgfx::UniformHandle u_time;
	bgfx::UniformHandle u_color;
	bgfx::UniformHandle s_texColor;
	bgfx::UniformHandle s_texNormal;
	bgfx::UniformHandle s_texEmissive;
	bgfx::UniformHandle u_sphericalHarmonics;
	bgfx::UniformHandle u_pixelSize;
	bgfx::UniformHandle s_fullScreen;
	bgfx::UniformHandle u_playerHealthParams;

	bgfx::FrameBufferHandle m_gbuffer;
	bgfx::FrameBufferHandle m_fullScreenTex;
	
	static constexpr int maxLightsCount = 8;
	bgfx::UniformHandle u_lightPosRadius;
	bgfx::UniformHandle u_lightRgbInnerR;

	int prevWidth;
	int prevHeight;

	std::shared_ptr<Texture> whiteTexture;
	std::shared_ptr<Texture> defaultNormalTexture;
	std::shared_ptr<Texture> defaultEmissiveTexture;
	std::shared_ptr<PostProcessingEffect> post;

	int dbgMeshesDrawn = 0;
};