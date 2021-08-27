#pragma once

#include "SMath.h"
#include "bgfx/bgfx.h"

struct SDL_Window;
class SDL_Surface;
class SystemsManager;
class Texture;
class Material;
class MeshRenderer;
class Mesh;
class Shader;


class Render {
public:
	bool Init();
	void Draw(SystemsManager& systems);
	void Term();

	int GetWidth() { return prevWidth; }
	int GetHeight() { return prevHeight; }
	bgfx::UniformHandle GetTexColorSampler() { return s_texColor; }
	bgfx::TextureHandle GetFullScreenTexture() { return m_fullScreenTex; }

	//The window we'll be rendering to
	//TODO make non static
	static SDL_Window* window;

	void ApplyMaterialProperties(const std::shared_ptr<Material> material);

private:
	class GBuffer {
	public:
		bgfx::FrameBufferHandle buffer;
		bgfx::FrameBufferHandle lightBuffer;

		bgfx::TextureHandle albedoTexture;
		bgfx::TextureHandle normalTexture;
		bgfx::TextureHandle lightTexture;
		bgfx::TextureHandle emissiveTexture;
		bgfx::TextureHandle depthTexture;

		bgfx::UniformHandle albedoSampler;
		bgfx::UniformHandle normalSampler;
		bgfx::UniformHandle lightSampler;
		bgfx::UniformHandle emissiveSampler;
		bgfx::UniformHandle depthSampler;
	};
	GBuffer gBuffer;
	
	bool DrawMesh(const MeshRenderer* renderer, bool clearState, bool updateState); //returns true if was not culled
	void UpdateLights(Vector3 poi);
	
	bool IsFullScreen();
	void SetFullScreen(bool isFullScreen);

	void DrawLights();


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
	bgfx::UniformHandle u_dirLightDirHandle;
	bgfx::UniformHandle u_dirLightColorHandle;

	std::unordered_map<std::string, bgfx::UniformHandle> colorUniforms;
	std::unordered_map<std::string, bgfx::UniformHandle> vectorUniforms;
	std::unordered_map<std::string, bgfx::UniformHandle> textureUniforms;

	bgfx::FrameBufferHandle m_fullScreenBuffer;
	bgfx::TextureHandle m_fullScreenTex;
	
	static constexpr int maxLightsCount = 8;
	bgfx::UniformHandle u_lightPosRadius;
	bgfx::UniformHandle u_lightRgbInnerR;
	bgfx::UniformHandle u_mtx;//TODO naming

	int prevWidth;
	int prevHeight;

	std::shared_ptr<Texture> whiteTexture;
	std::shared_ptr<Texture> defaultNormalTexture;
	std::shared_ptr<Texture> defaultEmissiveTexture;
	std::shared_ptr<Material> simpleBlitMat;
	std::shared_ptr<Shader> deferredLightShader;
	std::shared_ptr<Shader> deferredDirLightShader;
	Matrix4 viewProj;

	int dbgMeshesDrawn = 0;
	int dbgMeshesCulled = 0;

	Vector4 frustumPlanes[6];
};