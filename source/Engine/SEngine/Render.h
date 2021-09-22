#pragma once

#include "SMath.h"
#include "bgfx/bgfx.h"
#include "GameEvents.h"

struct SDL_Window;
class SDL_Surface;
class SystemsManager;
class Texture;
class Material;
class MeshRenderer;
class Mesh;
class Shader;
class Camera;
class ShadowRenderer;
class ICamera;

class Render {
	friend class ShadowRenderer;
public:
	bool Init();
	void Draw(SystemsManager& systems);
	void Term();

	int GetWidth() { return prevWidth; }
	int GetHeight() { return prevHeight; }
	bgfx::UniformHandle GetTexColorSampler() { return s_texColor; }

	bgfx::TextureHandle GetCurrentFullScreenTexture() const;
	bgfx::TextureHandle GetNextFullScreenTexture() const;
	int GetNextFullScreenTextureViewId() const;
	void FlipFullScreenTextures();

	//The window we'll be rendering to
	//TODO make non static
	static SDL_Window* window;

	void ApplyMaterialProperties(const Material* material);
	
	//TODO move viewId to camera
	void DrawAll(int viewId, const ICamera& camera, std::shared_ptr<Material> overrideMaterial);

	bgfx::UniformHandle GetOrCreateVectorUniform(const std::string& name);

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

	int currentFreeViewId = 0;
	
	bool DrawMesh(const MeshRenderer* renderer, const Material* material, const ICamera& camera, bool clearMaterialState, bool clearMeshState, bool updateMaterialState, bool updateMeshState, int viewId = 0); //returns true if was not culled
	void UpdateLights(Vector3 poi);
	
	bool IsFullScreen();
	void SetFullScreen(bool isFullScreen);

	void PrepareLights(const ICamera& camera);
	void EndFrame();

	void LoadAssets();
	void UnloadAssets();

	GameEventHandle databaseAfterUnloadedHandle;
	GameEventHandle databaseBeforeUnloadedHandle;

	//The surface contained by the window
	SDL_Surface* screenSurface = nullptr;

	bgfx::UniformHandle u_time;
	bgfx::UniformHandle u_color;
	bgfx::UniformHandle s_texColor;
	bgfx::UniformHandle s_texNormal;
	bgfx::UniformHandle s_texEmissive;
	bgfx::UniformHandle s_texClusterList;
	bgfx::UniformHandle s_texLightsList;
	bgfx::UniformHandle s_texItemsList;
	bgfx::UniformHandle u_sphericalHarmonics;
	bgfx::UniformHandle u_pixelSize;
	bgfx::UniformHandle s_fullScreen;
	bgfx::UniformHandle u_dirLightDirHandle;
	bgfx::UniformHandle u_dirLightColorHandle;

	std::unordered_map<std::string, bgfx::UniformHandle> colorUniforms;
	std::unordered_map<std::string, bgfx::UniformHandle> vectorUniforms;
	std::unordered_map<std::string, bgfx::UniformHandle> textureUniforms;
	std::unordered_map<std::string, bgfx::UniformHandle> matrixUniforms;

	//TODO separate cluster class to handle all this stuff
	int clusterWidth = 16;
	int clusterHeight = 8;
	int clusterDepth = 4;
	bgfx::TextureHandle m_clusterListTex;
	bgfx::TextureHandle m_itemsListTex;
	bgfx::TextureHandle m_lightsListTex;
	
	
	bgfx::FrameBufferHandle m_fullScreenBuffer;
	bgfx::TextureHandle m_fullScreenTex;
	bgfx::FrameBufferHandle m_fullScreenBuffer2;
	bgfx::TextureHandle m_fullScreenTex2;
	int currentFullScreenTextureIdx = 0;
	
	static constexpr int maxLightsCount = 8;
	bgfx::UniformHandle u_lightPosRadius;
	bgfx::UniformHandle u_lightRgbInnerR;
	bgfx::UniformHandle u_viewProjInv;

	int prevWidth;
	int prevHeight;

	std::shared_ptr<Texture> whiteTexture;
	std::shared_ptr<Texture> defaultNormalTexture;
	std::shared_ptr<Texture> defaultEmissiveTexture;
	std::shared_ptr<Material> simpleBlitMat;
	std::shared_ptr<Material> defferedCombineMaterial;
	std::shared_ptr<Shader> deferredLightShader;
	std::shared_ptr<Shader> deferredDirLightShader;

	int dbgMeshesDrawn = 0;
	int dbgMeshesCulled = 0;
	std::shared_ptr<ShadowRenderer> shadowRenderer;
};