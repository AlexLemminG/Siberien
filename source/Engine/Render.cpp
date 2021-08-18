#include "Render.h"

#include "bgfx/bgfx.h"
#include "Common.h"

#include "Render.h"

#include "SDL.h"
#include "yaml-cpp/yaml.h"
#include <SDL_syswm.h>
#include <bgfx/platform.h>
#include "Camera.h"
#include "MeshRenderer.h"
#include "bgfx/bgfx.h"
#include <bx\math.h>
#include "GameObject.h"
#include "Transform.h"
#include "Dbg.h"
#include "System.h"
#include "Time.h"
#include "Scene.h"
#include "SDL_mixer.h"
#include "Input.h"

SDL_Window* Render::window = nullptr;


bool Render::IsFullScreen() {
	auto flags = SDL_GetWindowFlags(window);
	return flags & SDL_WINDOW_FULLSCREEN;
}
void Render::SetFullScreen(bool isFullScreen) {
	SDL_SetWindowFullscreen(window, isFullScreen ? SDL_WINDOW_FULLSCREEN : 0);
}
bool Render::Init()
{
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

	//TODO not in render
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0 || Mix_Init(0) < 0)
	{
		printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
		return false;
	}
	Mix_AllocateChannels(64);

	int width = SettingsGetInt("screenWidth");
	int height = SettingsGetInt("screenHeight");
	int posX = SettingsGetInt("screenPosX");
	int posY = SettingsGetInt("screenPosY");
	//Create window
	auto flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	window = SDL_CreateWindow("Siberien", posX, posY, width, height, flags);
	if (window == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return false;
	}
	SetFullScreen(CfgGetBool("fullscreen"));
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(window, &wmi)) {
		printf("Failed to get window wm info! SDL_Error: %s\n", SDL_GetError());
		return false;
	}
	bgfx::PlatformData pd;
	pd.ndt = NULL;
	pd.nwh = wmi.info.win.window;
	pd.context = NULL;
	pd.backBuffer = NULL;
	pd.backBufferDS = NULL;
	bgfx::setPlatformData(pd);

	bgfx::Init initInfo{};
	initInfo.debug = true;
	initInfo.type = bgfx::RendererType::Direct3D11;
	bgfx::init(initInfo);
	bgfx::reset(width, height, BGFX_RESET_VSYNC);

	bgfx::resetView(0);
	bgfx::resetView(1);
	bgfx::setDebug(BGFX_DEBUG_TEXT /*| BGFX_DEBUG_STATS*/);

	bgfx::setViewRect(0, 0, 0, width, height);
	bgfx::setViewRect(1, 0, 0, width, height);

	InitVertexLayouts();

	u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
	u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
	u_playerHealthParams = bgfx::createUniform("u_playerHealthParams", bgfx::UniformType::Vec4);
	s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
	s_texEmissive = bgfx::createUniform("s_texEmissive", bgfx::UniformType::Sampler);

	u_lightPosRadius = bgfx::createUniform("u_lightPosRadius", bgfx::UniformType::Vec4, maxLightsCount);
	u_lightRgbInnerR = bgfx::createUniform("u_lightRgbInnerR", bgfx::UniformType::Vec4, maxLightsCount);

	u_sphericalHarmonics = bgfx::createUniform("u_sphericalHarmonics", bgfx::UniformType::Vec4, 9);

	u_pixelSize = bgfx::createUniform("u_pixelSize", bgfx::UniformType::Vec4);

	whiteTexture = AssetDatabase::Get()->LoadByPath<Texture>("textures\\white.png");
	defaultNormalTexture = AssetDatabase::Get()->LoadByPath<Texture>("textures\\defaultNormal.png");
	defaultEmissiveTexture = AssetDatabase::Get()->LoadByPath<Texture>("textures\\defaultEmissive.png");

	m_fullScreenTex.idx = bgfx::kInvalidHandle;
	m_gbuffer.idx = bgfx::kInvalidHandle;

	prevWidth = width;
	prevHeight = height;
	post = AssetDatabase::Get()->LoadByPath<PostProcessingEffect>("playerHealthEffect.asset");

	Dbg::Init();

	return true;
}
void Render::Draw(SystemsManager& systems)
{
	OPTICK_EVENT();

	if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_RETURN) && (Input::GetKey(SDL_Scancode::SDL_SCANCODE_LALT) || Input::GetKey(SDL_Scancode::SDL_SCANCODE_RALT))) {
		SetFullScreen(!IsFullScreen());
	}


	Matrix4 cameraViewMatrix;
	auto camera = Camera::GetMain();
	int width;
	int height;
	float fov = 60.f;
	float nearPlane = 0.1f;
	float farPlane = 100.f;
	Vector3 lightsPoi = Vector3_zero;
	SDL_GetWindowSize(window, &width, &height);

	if (height != prevHeight || width != prevWidth || !bgfx::isValid(m_fullScreenTex)) {
		prevWidth = width;
		prevHeight = height;
		bgfx::reset(width, height, BGFX_RESET_VSYNC);
		if (bgfx::isValid(m_fullScreenTex)) {
			bgfx::destroy(m_fullScreenTex);
		}
		if (bgfx::isValid(m_gbuffer)) {
			bgfx::destroy(m_gbuffer);
		}

		const uint64_t tsFlags = 0
			| BGFX_TEXTURE_RT
			| BGFX_SAMPLER_U_CLAMP
			| BGFX_SAMPLER_V_CLAMP
			;

		m_fullScreenTex = bgfx::createFrameBuffer(
			(uint16_t)(width)
			, (uint16_t)(height)
			, bgfx::TextureFormat::RGBA32F
			, tsFlags
		);

		bgfx::TextureHandle gbufferTex[] =
		{
			bgfx::getTexture(m_fullScreenTex),
			bgfx::createTexture2D(uint16_t(width), uint16_t(height), false, 1, bgfx::TextureFormat::D32F, tsFlags),
		};

		m_gbuffer = bgfx::createFrameBuffer(BX_COUNTOF(gbufferTex), gbufferTex, true);
	}
	bgfx::setViewRect(0, 0, 0, width, height);
	bgfx::setViewRect(1, 0, 0, width, height);
	bgfx::setViewFrameBuffer(0, m_gbuffer);
	bgfx::setViewFrameBuffer(1, BGFX_INVALID_HANDLE);


	if (camera == nullptr) {
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, Colors::black.ToIntRGBA(), 1.0f, 0);
	}
	else {
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, camera->GetClearColor().ToIntRGBA(), 1.0f, 0);
		auto cameraMatrix = camera->gameObject()->transform()->matrix;
		SetScale(cameraMatrix, Vector3_one);
		cameraViewMatrix = cameraMatrix.Inverse();
		fov = camera->GetFov();
		nearPlane = camera->GetNearPlane();
		farPlane = camera->GetFarPlane();
		lightsPoi = GetPos(cameraMatrix) + GetRot(cameraMatrix) * Vector3_forward * 15.f;
	}

	float realTime = Time::time();
	float time = Time::time();
	bgfx::setUniform(u_time, &time);

	static float prevTimeCheck = 0;
	static int prevFramesCount = 0;
	static float fps = 0;
	if ((int)realTime != (int)prevTimeCheck) {
		fps = (Time::frameCount() - prevFramesCount) / (time - prevTimeCheck);
		prevTimeCheck = realTime;
		prevFramesCount = Time::frameCount();
	}

	bgfx::touch(0);
	bgfx::dbgTextClear();

	bgfx::dbgTextPrintf(1, 2, 0x0f, "FPS: %.1f", fps);

	if (camera == nullptr) {
		//bgfx::dbgTextPrintf(1, 1, 0x0f, "NO CAMERA");
	}
	else {
		//bgfx::dbgTextPrintf(1, 1, 0x0f, "YES CAMERA");
	}

	{
		float proj[16];
		bx::mtxProj(proj, fov, float(width) / float(height), nearPlane, farPlane, bgfx::getCaps()->homogeneousDepth);
		bgfx::setViewTransform(0, &cameraViewMatrix(0, 0), proj);
		bgfx::setViewTransform(1, &cameraViewMatrix(0, 0), proj);
	}

	UpdateLights(lightsPoi);

	for (auto mesh : MeshRenderer::enabledMeshRenderers) {
		DrawMesh(mesh);
	}

	systems.Draw();

	if (post) {
		post->Draw(*this);
	}

	Dbg::DrawAll();

	bgfx::frame();
}

void Render::Term()
{
	Dbg::Term();
	post = nullptr;
	whiteTexture = nullptr;
	defaultNormalTexture = nullptr;
	defaultEmissiveTexture = nullptr;

	//TODO destroy programs, buffers and other shit
	bgfx::destroy(u_time);
	bgfx::destroy(u_color);
	bgfx::destroy(u_playerHealthParams);
	bgfx::destroy(s_texColor);
	bgfx::destroy(s_texNormal);
	bgfx::destroy(s_texEmissive);
	bgfx::destroy(u_lightPosRadius);
	bgfx::destroy(u_lightRgbInnerR);
	bgfx::destroy(u_sphericalHarmonics);
	bgfx::destroy(u_pixelSize);

	if (bgfx::isValid(m_fullScreenTex)) {
		bgfx::destroy(m_fullScreenTex);
	}
	if (bgfx::isValid(m_gbuffer)) {
		bgfx::destroy(m_gbuffer);
	}

	bgfx::shutdown();

	if (window != nullptr) {
		int width;
		int height;
		SDL_GetWindowSize(window, &width, &height);
		SettingsSetInt("screenWidth", width);
		SettingsSetInt("screenHeight", height);

		int posX;
		int posY;
		SDL_GetWindowPosition(window, &posX, &posY);
		SettingsSetInt("screenPosX", posX);
		SettingsSetInt("screenPosY", posY);

		SDL_DestroyWindow(window);
	}
	//Destroy window

	//Quit SDL subsystems
	Mix_Quit();
	SDL_Quit();
}

void Render::UpdateLights(Vector3 poi) {
	std::vector<PointLight*> lights = PointLight::pointLights;

	Vector4 posRadius[maxLightsCount];
	Vector4 colorInnerRadius[maxLightsCount];

	for (int i = 0; i < maxLightsCount; i++) {
		if (lights.size() == 0) {
			for (int j = i; j < maxLightsCount; j++) {
				posRadius[j] = Vector4(FLT_MAX, FLT_MAX, FLT_MAX, 0.f);
				colorInnerRadius[j].x = 0;
				colorInnerRadius[j].y = 0;
				colorInnerRadius[j].z = 0;
				colorInnerRadius[j].w = 0;
			}
			break;
		}
		int closestLightIdx = 0;
		float closestDistance = FLT_MAX;
		for (int lightIdx = 0; lightIdx < lights.size(); lightIdx++) {
			float distance = Vector3::Distance(poi, lights[lightIdx]->gameObject()->transform()->GetPosition());
			distance -= lights[lightIdx]->radius;
			if (distance < closestDistance) {
				closestDistance = distance;
				closestLightIdx = lightIdx;
			}
		}

		auto closestLight = lights[closestLightIdx];
		lights.erase(lights.begin() + closestLightIdx);

		Vector3 pos = closestLight->gameObject()->transform()->GetPosition();
		posRadius[i] = Vector4(pos, closestLight->radius);
		Color color = closestLight->color;
		colorInnerRadius[i] = Vector4(color.r, color.g, color.b, closestLight->innerRadius);
	}
	bgfx::setUniform(u_lightPosRadius, &posRadius, maxLightsCount);
	bgfx::setUniform(u_lightRgbInnerR, &colorInnerRadius, maxLightsCount);

	if (Scene::Get()) {
		if (Scene::Get()->sphericalHarmonics) {
			bgfx::setUniform(u_sphericalHarmonics, &Scene::Get()->sphericalHarmonics->coeffs[0], 9);
		}
	}
}

void Render::DrawMesh(MeshRenderer* renderer) {
	if (!renderer->mesh || !renderer->material || !renderer->material->shader) {
		return;
	}
	//TODO wtf is this
	uint64_t state = 0
		| BGFX_STATE_WRITE_R
		| BGFX_STATE_WRITE_G
		| BGFX_STATE_WRITE_B
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_WRITE_Z
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CCW
		| BGFX_STATE_MSAA
		;

	float f = renderer->worldMatrix(0, 0);
	if (renderer->bonesFinalMatrices.size() != 0) {
		bgfx::setTransform(&renderer->bonesFinalMatrices[0], renderer->bonesFinalMatrices.size());
	}
	else {
		bgfx::setTransform(&(renderer->gameObject()->transform()->matrix));
	}
	bgfx::setUniform(u_color, &renderer->material->color);


	if (renderer->material->colorTex) {
		if (renderer->material->randomColorTextures.size() > 0) {
			bgfx::setTexture(0, s_texColor, renderer->material->randomColorTextures[renderer->randomColorTextureIdx]->handle);
		}
		else {
			bgfx::setTexture(0, s_texColor, renderer->material->colorTex->handle);
		}
	}
	else {
		if (whiteTexture) {
			bgfx::setTexture(0, s_texColor, whiteTexture->handle);
		}
	}

	if (renderer->material->normalTex) {
		bgfx::setTexture(1, s_texNormal, renderer->material->normalTex->handle);
	}
	else {
		if (defaultNormalTexture) {
			bgfx::setTexture(1, s_texNormal, defaultNormalTexture->handle);
		}
	}

	if (renderer->material->emissiveTex) {
		bgfx::setTexture(2, s_texEmissive, renderer->material->emissiveTex->handle);
	}
	else {
		if (defaultEmissiveTexture) {
			bgfx::setTexture(2, s_texEmissive, defaultEmissiveTexture->handle);
		}
	}

	bgfx::setVertexBuffer(0, renderer->mesh->vertexBuffer);
	bgfx::setIndexBuffer(renderer->mesh->indexBuffer);

	bgfx::setState(state);

	bgfx::submit(0, renderer->material->shader->program);
}
