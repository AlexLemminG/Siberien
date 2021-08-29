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
#include "STime.h"
#include "Scene.h"
#include "Input.h"
#include "Material.h"
#include "Texture.h"
#include "SphericalHarmonics.h"
#include "Shader.h"
#include "MeshRenderer.h"
#include "Material.h"
#include "Mesh.h"
#include "bounds.h"
#include "RenderEvents.h"
#include "Graphics.h"
#include "Config.h"
#include "imgui/imgui.h"
#include "Light.h"
#include "ShadowRenderer.h"
#include "dear-imgui/imgui_impl_sdl.h"

SDL_Window* Render::window = nullptr;

//TODO deal with viewId ordering from other systems
constexpr bgfx::ViewId kRenderPassGeometry = 0;
constexpr bgfx::ViewId kRenderPassClearUav = 1;
constexpr bgfx::ViewId kRenderPassLight = 10;
constexpr bgfx::ViewId kRenderPassCombine = 11;
constexpr bgfx::ViewId kRenderPassDebugLights = 12;
constexpr bgfx::ViewId kRenderPassDebugGBuffer = 13;


bool Render::IsFullScreen() {
	auto flags = SDL_GetWindowFlags(window);
	return flags & SDL_WINDOW_FULLSCREEN_DESKTOP;
}

void Render::SetFullScreen(bool isFullScreen) {
	SDL_SetWindowFullscreen(window, isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}


//TODO cleanup
Vector3 multH(const Matrix4& m, const Vector3& v) {
	Vector4 v4(v[0], v[1], v[2], 1);
	v4 = m * v4;
	float t = 1.f / Mathf::Abs(v4[3]);
	return Vector3(v4[0], v4[1], v4[2]) * t;
}

void Render::DrawLights(const ICamera& camera) {
	if (!deferredLightShader || !deferredDirLightShader) {
		return;
	}
	std::vector<PointLight*> pointLights = PointLight::pointLights;

	for (auto light : pointLights) {
		if (light->radius <= 0.f) {
			continue;
		}

		auto pos = light->gameObject()->transform()->GetPosition();
		auto sphere = Sphere{ pos, light->radius };
		bool isVisible = camera.IsVisible(sphere);
		if (!isVisible) {
			continue;
		}

		shadowRenderer->Draw(light, camera);

		auto posRadius = Vector4(pos, light->radius);
		bgfx::setUniform(u_lightPosRadius, &posRadius, 1);
		auto colorInnerRadius = Vector4(light->color.r, light->color.g, light->color.b, light->innerRadius);
		bgfx::setUniform(u_lightRgbInnerR, &colorInnerRadius, 1);

		AABB aabb = sphere.ToAABB();

		const Vector3 box[8] =
		{
			{ aabb.min.x, aabb.min.y, aabb.min.z },
			{ aabb.min.x, aabb.min.y, aabb.max.z },
			{ aabb.min.x, aabb.max.y, aabb.min.z },
			{ aabb.min.x, aabb.max.y, aabb.max.z },
			{ aabb.max.x, aabb.min.y, aabb.min.z },
			{ aabb.max.x, aabb.min.y, aabb.max.z },
			{ aabb.max.x, aabb.max.y, aabb.min.z },
			{ aabb.max.x, aabb.max.y, aabb.max.z },
		};


		Vector3 xyz = multH(camera.GetViewProjectionMatrix(), box[0]);
		Vector3 min = xyz;
		Vector3 max = xyz;

		for (uint32_t ii = 1; ii < 8; ++ii)
		{
			xyz = multH(camera.GetViewProjectionMatrix(), box[ii]);
			min = Vector3::Min(min, xyz);
			max = Vector3::Max(max, xyz);
		}

		const float x0 = Mathf::Clamp((min.x * 0.5f + 0.5f) * prevWidth, 0.0f, (float)prevWidth);
		const float y0 = Mathf::Clamp((min.y * 0.5f + 0.5f) * prevHeight, 0.0f, (float)prevHeight);
		const float x1 = Mathf::Clamp((max.x * 0.5f + 0.5f) * prevWidth, 0.0f, (float)prevWidth);
		const float y1 = Mathf::Clamp((max.y * 0.5f + 0.5f) * prevHeight, 0.0f, (float)prevHeight);

		bgfx::setTexture(0, gBuffer.normalSampler, gBuffer.normalTexture);
		bgfx::setTexture(1, gBuffer.depthSampler, gBuffer.depthTexture);
		const uint16_t scissorHeight = uint16_t(y1 - y0);
		//TODO
		//bgfx::setScissor(uint16_t(x0), uint16_t(prevHeight - scissorHeight - y0), uint16_t(x1 - x0), uint16_t(scissorHeight));
		bgfx::setState(0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_BLEND_ADD
		);
		Graphics::Get()->SetScreenSpaceQuadBuffer();
		bgfx::submit(kRenderPassLight, deferredLightShader->program);
	}

	std::vector<DirLight*> dirLights = DirLight::dirLights;
	for (auto light : dirLights) {
		shadowRenderer->Draw(light, camera);
		auto dir = Vector4(GetRot(light->gameObject()->transform()->matrix) * Vector3_forward, 0.f);
		auto color = Vector4(light->color.r, light->color.g, light->color.b, 0.f);
		bgfx::setUniform(u_dirLightDirHandle, &dir, 1);
		bgfx::setUniform(u_dirLightColorHandle, &color, 1);

		bgfx::setTexture(0, gBuffer.normalSampler, gBuffer.normalTexture);
		bgfx::setTexture(1, gBuffer.depthSampler, gBuffer.depthTexture);
		bgfx::setState(0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_BLEND_ADD
		);
		Graphics::Get()->SetScreenSpaceQuadBuffer();
		bgfx::submit(kRenderPassLight, deferredDirLightShader->program);
	}

}


bool Render::Init()
{
	OPTICK_EVENT();
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

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
	initInfo.debug = false;//TODO cfgvar?
	initInfo.profile = false;
	initInfo.type = bgfx::RendererType::Direct3D11;
	//initInfo.limits.transientVbSize *= 10;//TODO debug only
	//initInfo.limits.transientIbSize *= 10;//TODO debug only
	bgfx::init(initInfo);
	bgfx::reset(width, height, CfgGetBool("vsync") ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);

	bgfx::resetView(0);
	bgfx::resetView(1);
	bgfx::setDebug(BGFX_DEBUG_TEXT /*| BGFX_DEBUG_STATS*/);

	bgfx::setViewRect(0, 0, 0, width, height);
	bgfx::setViewRect(1, 0, 0, width, height);

	u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
	u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
	s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
	s_texEmissive = bgfx::createUniform("s_texEmissive", bgfx::UniformType::Sampler);

	u_lightPosRadius = bgfx::createUniform("u_lightPosRadius", bgfx::UniformType::Vec4, maxLightsCount);
	u_lightRgbInnerR = bgfx::createUniform("u_lightRgbInnerR", bgfx::UniformType::Vec4, maxLightsCount);
	u_viewProjInv = bgfx::createUniform("u_viewProjInv", bgfx::UniformType::Mat4);

	u_sphericalHarmonics = bgfx::createUniform("u_sphericalHarmonics", bgfx::UniformType::Vec4, 9);

	u_pixelSize = bgfx::createUniform("u_pixelSize", bgfx::UniformType::Vec4);

	u_dirLightDirHandle = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
	u_dirLightColorHandle = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4);


	m_fullScreenTex.idx = bgfx::kInvalidHandle;
	m_fullScreenBuffer.idx = bgfx::kInvalidHandle;
	gBuffer.buffer.idx = bgfx::kInvalidHandle;

	prevWidth = width;
	prevHeight = height;

	shadowRenderer = std::make_shared<ShadowRenderer>();
	shadowRenderer->Init();

	imguiCreate();
	ImGui_ImplSDL2_InitForMetal(window);

	Dbg::Init();

	return true;
}

void Render::Draw(SystemsManager& systems)
{
	OPTICK_EVENT();


	whiteTexture = AssetDatabase::Get()->LoadByPath<Texture>("textures\\white.png");
	defaultNormalTexture = AssetDatabase::Get()->LoadByPath<Texture>("textures\\defaultNormal.png");
	defaultEmissiveTexture = AssetDatabase::Get()->LoadByPath<Texture>("textures\\defaultEmissive.png");
	simpleBlitMat = AssetDatabase::Get()->LoadByPath<Material>("materials\\simpleBlit.asset");
	deferredLightShader = AssetDatabase::Get()->LoadByPath<Shader>("shaders\\deferredLight.asset");
	deferredDirLightShader = AssetDatabase::Get()->LoadByPath<Shader>("shaders\\deferredDirLight.asset");

	//TODO not here
	if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_RETURN) && (Input::GetKey(SDL_Scancode::SDL_SCANCODE_LALT) || Input::GetKey(SDL_Scancode::SDL_SCANCODE_RALT))) {
		SetFullScreen(!IsFullScreen());
	}

	int width;
	int height;
	SDL_GetWindowSize(window, &width, &height);

	if (height != prevHeight || width != prevWidth || !bgfx::isValid(m_fullScreenBuffer)) {
		prevWidth = width;
		prevHeight = height;
		bgfx::reset(width, height, CfgGetBool("vsync") ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);
		if (bgfx::isValid(m_fullScreenBuffer)) {
			bgfx::destroy(m_fullScreenBuffer);
		}
		if (bgfx::isValid(gBuffer.buffer)) {
			bgfx::destroy(gBuffer.buffer);
		}

		const uint64_t tsFlags = 0
			| BGFX_TEXTURE_RT
			| BGFX_SAMPLER_U_CLAMP
			| BGFX_SAMPLER_V_CLAMP
			;

		m_fullScreenTex = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		m_fullScreenBuffer = bgfx::createFrameBuffer(1, &m_fullScreenTex, true);

		gBuffer.albedoTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		gBuffer.normalTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		gBuffer.lightTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		gBuffer.emissiveTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		gBuffer.depthTexture = bgfx::createTexture2D(uint16_t(width), uint16_t(height), false, 1, bgfx::TextureFormat::D32F, tsFlags);
		bgfx::TextureHandle gbufferTex[] =
		{
			gBuffer.albedoTexture,
			gBuffer.normalTexture,
			gBuffer.lightTexture,
			gBuffer.emissiveTexture,
			gBuffer.depthTexture
		};
		gBuffer.buffer = bgfx::createFrameBuffer(BX_COUNTOF(gbufferTex), gbufferTex, true);
		gBuffer.lightBuffer = bgfx::createFrameBuffer(1, &gBuffer.lightTexture, true);//TODO delete
		//TODO destroy
		gBuffer.albedoSampler = bgfx::createUniform("s_albedo", bgfx::UniformType::Sampler);
		gBuffer.normalSampler = bgfx::createUniform("s_normals", bgfx::UniformType::Sampler);
		gBuffer.lightSampler = bgfx::createUniform("s_light", bgfx::UniformType::Sampler);
		gBuffer.emissiveSampler = bgfx::createUniform("s_emissive", bgfx::UniformType::Sampler);
		gBuffer.depthSampler = bgfx::createUniform("s_depth", bgfx::UniformType::Sampler);

		bgfx::setName(gBuffer.buffer, "gbuffer");
		bgfx::setName(gBuffer.albedoTexture, "albedo");
		bgfx::setName(gBuffer.normalTexture, "normal");
		bgfx::setName(gBuffer.lightTexture, "light");
		bgfx::setName(gBuffer.lightBuffer, "light");
		bgfx::setName(gBuffer.emissiveTexture, "emissive");
		bgfx::setName(gBuffer.depthTexture, "depth");

		bgfx::setName(m_fullScreenTex, "fullScreenTex");
		bgfx::setName(m_fullScreenBuffer, "fullScreenBuffer");

		bgfx::setViewFrameBuffer(kRenderPassGeometry, gBuffer.buffer);
		bgfx::setViewFrameBuffer(kRenderPassCombine, m_fullScreenBuffer);
		bgfx::setViewFrameBuffer(kRenderPassLight, gBuffer.lightBuffer);
		bgfx::setViewFrameBuffer(1, BGFX_INVALID_HANDLE);//TODO do we need third buffer to go postprocessing?
	}

	bgfx::dbgTextClear();
	bgfx::setViewRect(kRenderPassGeometry, 0, 0, width, height);
	bgfx::setViewRect(1, 0, 0, width, height);
	bgfx::setViewRect(kRenderPassCombine, 0, 0, width, height);
	bgfx::setViewRect(kRenderPassLight, 0, 0, width, height);

	const float pixelSize[4] =
	{
		1.0f / width,
		1.0f / height,
		0.0f,
		0.0f,
	};
	bgfx::setUniform(u_pixelSize, pixelSize);


	auto camera = Camera::GetMain();
	if (camera == nullptr) {
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, Colors::black.ToIntRGBA(), 1.0f, 0);
		bgfx::dbgTextPrintf(1, 1, 0x0f, "NO CAMERA");
		EndFrame();
		return;
	}

	camera->OnBeforeRender();

	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, camera->GetClearColor().ToIntRGBA(), 1.0f, 0);

	Vector3 lightsPoi = GetPos(camera->gameObject()->transform()->matrix) + GetRot(camera->gameObject()->transform()->matrix) * Vector3_forward * 15.f;

	//TODO not here
	float realTime = Time::time();
	float time = Time::time();
	static float prevTimeCheck = 0;
	static int prevFramesCount = 0;
	static float fps = 0;
	if ((int)realTime != (int)prevTimeCheck) {
		fps = (Time::frameCount() - prevFramesCount) / (time - prevTimeCheck);
		prevTimeCheck = realTime;
		prevFramesCount = Time::frameCount();
	}

	{
		float time = Time::time();
		bgfx::setUniform(u_time, &time);
	}
	{
		OPTICK_EVENT("bgfx::touch");
		bgfx::touch(0);//TODO not needed ?
	}

	bgfx::dbgTextPrintf(1, 2, 0x0f, "FPS: %.1f", fps);

	if (camera == nullptr) {
		if (camera == nullptr) {
			bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, Colors::black.ToIntRGBA(), 1.0f, 0);
			bgfx::dbgTextPrintf(1, 1, 0x0f, "NO CAMERA");
		}
	}


	{
		bgfx::setViewTransform(0, &camera->GetViewMatrix()(0, 0), &camera->GetProjectionMatrix()(0, 0));
		bgfx::setViewTransform(1, &camera->GetViewMatrix()(0, 0), &camera->GetProjectionMatrix()(0, 0));
		bgfx::setViewTransform(kRenderPassGeometry, &camera->GetViewMatrix()(0, 0), &camera->GetProjectionMatrix()(0, 0));
		Vector3 cameraPos = GetPos(camera->GetViewMatrix().Inverse());
		bgfx::setUniform(GetOrCreateVectorUniform("u_cameraPos"), &cameraPos.x);

		auto invViewProj = camera->GetViewProjectionMatrix().Inverse();
		bgfx::setUniform(u_viewProjInv, &invViewProj, 1);

		const bgfx::Caps* caps = bgfx::getCaps();
		float proj[16];
		bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, caps->homogeneousDepth);
		bgfx::setViewTransform(kRenderPassLight, &camera->GetViewMatrix()(0, 0), proj);
		bgfx::setViewTransform(kRenderPassCombine, nullptr, proj);

	}

	UpdateLights(lightsPoi);


	dbgMeshesDrawn = 0;
	dbgMeshesCulled = 0;

	DrawAll(0, *camera, nullptr);

	systems.Draw();

	DrawLights(*camera);

	// combining gbuffer
	{
		auto material = AssetDatabase::Get()->LoadByPath<Material>("materials\\deferredCombine.asset");//TOOD on init
		if (!material || !material->shader) {
			return;
		}
		ApplyMaterialProperties(material);

		bgfx::setTexture(0, gBuffer.albedoSampler, gBuffer.albedoTexture);
		bgfx::setTexture(1, gBuffer.normalSampler, gBuffer.normalTexture);
		bgfx::setTexture(2, gBuffer.lightSampler, gBuffer.lightTexture);
		bgfx::setTexture(3, gBuffer.emissiveSampler, gBuffer.emissiveTexture);
		bgfx::setTexture(4, gBuffer.depthSampler, gBuffer.depthTexture);

		bgfx::setState(0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
		);

		Graphics::Get()->SetScreenSpaceQuadBuffer();

		bgfx::submit(kRenderPassCombine, material->shader->program);
	}



	RenderEvents::Get()->onSceneRendered.Invoke(*this);

	Graphics::Get()->Blit(simpleBlitMat, 1);

	EndFrame();
}

void Render::EndFrame() {

	Dbg::DrawAll();
	{
		OPTICK_EVENT("bgfx::frame");
		bgfx::frame();
	}
}

void Render::Term()
{
	OPTICK_EVENT();
	shadowRenderer->Term();
	shadowRenderer = nullptr;
	imguiDestroy();
	Dbg::Term();
	whiteTexture = nullptr;
	defaultNormalTexture = nullptr;
	defaultEmissiveTexture = nullptr;
	simpleBlitMat = nullptr;
	deferredLightShader = nullptr;
	deferredDirLightShader = nullptr;

	for (auto u : textureUniforms) {
		bgfx::destroy(u.second);
	}
	for (auto u : colorUniforms) {
		bgfx::destroy(u.second);
	}
	for (auto u : vectorUniforms) {
		bgfx::destroy(u.second);
	}
	for (auto u : matrixUniforms) {
		bgfx::destroy(u.second);
	}

	//TODO destroy programs, buffers and other shit
	bgfx::destroy(u_time);
	bgfx::destroy(u_color);
	bgfx::destroy(s_texColor);
	bgfx::destroy(s_texNormal);
	bgfx::destroy(s_texEmissive);
	bgfx::destroy(u_lightPosRadius);
	bgfx::destroy(u_lightRgbInnerR);
	bgfx::destroy(u_viewProjInv);
	bgfx::destroy(u_sphericalHarmonics);
	bgfx::destroy(u_pixelSize);
	bgfx::destroy(u_dirLightDirHandle);
	bgfx::destroy(u_dirLightColorHandle);

	if (bgfx::isValid(m_fullScreenBuffer)) {
		bgfx::destroy(m_fullScreenBuffer);
	}
	if (bgfx::isValid(gBuffer.buffer)) {
		bgfx::destroy(gBuffer.buffer);
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

void Render::DrawAll(int viewId, const ICamera& camera, std::shared_ptr<Material> overrideMaterial) {
	//TODO setup camera
	OPTICK_EVENT();

	if (!overrideMaterial) {
		auto renderersSort = [](const MeshRenderer* r1, const MeshRenderer* r2) {
			if (r1->mesh.get() < r2->mesh.get()) {
				return true;
			}
			else if (r1->mesh.get() > r2->mesh.get()) {
				return false;
			}
			if (r1->material.get() < r2->material.get()) {
				return true;
			}
			else if (r1->material.get() > r2->material.get()) {
				return false;
			}
			return r1->randomColorTextureIdx < r2->randomColorTextureIdx;
		};
		{
			OPTICK_EVENT("SortRenderers");
			auto& renderers = MeshRenderer::enabledMeshRenderers;
			std::sort(renderers.begin(), renderers.end(), renderersSort);
		}


		OPTICK_EVENT("DrawRenderers");
		bool prevEq = false;
		//TODO get rid of std::vector / std::string
		for (int i = 0; i < ((int)MeshRenderer::enabledMeshRenderers.size() - 1); i++) {
			const auto& mesh = MeshRenderer::enabledMeshRenderers[i];
			const auto& meshNext = MeshRenderer::enabledMeshRenderers[i + 1];
			bool nextEq = !renderersSort(mesh, meshNext) && !renderersSort(meshNext, mesh);
			bool drawn = DrawMesh(mesh, camera, !nextEq, !prevEq, viewId);
			prevEq = nextEq && drawn;
		}
		//TODO do we really need this ?
		if (MeshRenderer::enabledMeshRenderers.size() > 0) {
			DrawMesh(MeshRenderer::enabledMeshRenderers[MeshRenderer::enabledMeshRenderers.size() - 1], camera, true, !prevEq, viewId);
		}
	}
	else {
		//TODO refactor duplicated code

		auto renderersSort = [](const MeshRenderer* r1, const MeshRenderer* r2) {
			return r1->mesh.get() < r2->mesh.get();
		};
		{
			OPTICK_EVENT("SortRenderers");
			auto& renderers = MeshRenderer::enabledMeshRenderers;
			std::sort(renderers.begin(), renderers.end(), renderersSort);
		}


		OPTICK_EVENT("DrawRenderers");
		bool prevEq = false;
		//TODO get rid of std::vector / std::string
		for (int i = 0; i < ((int)MeshRenderer::enabledMeshRenderers.size() - 1); i++) {
			const auto& mesh = MeshRenderer::enabledMeshRenderers[i];
			const auto& meshNext = MeshRenderer::enabledMeshRenderers[i + 1];
			//TODO optimize
			auto mat = mesh->material;
			mesh->material = overrideMaterial;
			bool nextEq = !renderersSort(mesh, meshNext) && !renderersSort(meshNext, mesh);
			bool drawn = DrawMesh(mesh, camera, !nextEq, !prevEq, viewId);
			mesh->material = mat;
			prevEq = nextEq && drawn;
		}
		if (MeshRenderer::enabledMeshRenderers.size() > 0) {
			const auto& mesh = MeshRenderer::enabledMeshRenderers[MeshRenderer::enabledMeshRenderers.size() - 1];
			auto mat = mesh->material;
			mesh->material = overrideMaterial;
			DrawMesh(mesh, camera, true, !prevEq, viewId);
			mesh->material = mat;
		}
	}
}

bool Render::DrawMesh(const MeshRenderer* renderer, const ICamera& camera, bool clearState, bool updateState, int viewId) {
	const auto& matrix = renderer->m_transform->matrix;
	{
		bool isVisible = camera.IsVisible(*renderer);
		if (!isVisible) {
			dbgMeshesCulled++;
			return false;
		}
	}
	dbgMeshesDrawn++;

	if (renderer->bonesFinalMatrices.size() != 0) {
		bgfx::setTransform(&renderer->bonesFinalMatrices[0], renderer->bonesFinalMatrices.size());
	}
	else {
		bgfx::setTransform(&matrix);
	}

	if (updateState) {
		ApplyMaterialProperties(renderer->material);
		if (renderer->material->colorTex) {
			if (renderer->material->randomColorTextures.size() > 0) {
				bgfx::setTexture(0, s_texColor, renderer->material->randomColorTextures[renderer->randomColorTextureIdx]->handle);
			}
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
			;
		bgfx::setState(state);
		bgfx::setVertexBuffer(0, renderer->mesh->vertexBuffer);
		bgfx::setIndexBuffer(renderer->mesh->indexBuffer);
	}

	auto discardFlags = clearState ? BGFX_DISCARD_ALL : BGFX_DISCARD_NONE;
	bgfx::submit(viewId, renderer->material->shader->program, 0u, discardFlags);
	return true;
}

void Render::ApplyMaterialProperties(const std::shared_ptr<Material> material) {
	bgfx::setUniform(u_color, &material->color);

	if (material->colorTex) {
		bgfx::setTexture(0, s_texColor, material->colorTex->handle);
	}
	else {
		if (whiteTexture) {
			bgfx::setTexture(0, s_texColor, whiteTexture->handle);
		}
	}

	if (material->normalTex) {
		bgfx::setTexture(1, s_texNormal, material->normalTex->handle);
	}
	else {
		if (defaultNormalTexture) {
			bgfx::setTexture(1, s_texNormal, defaultNormalTexture->handle);
		}
	}

	if (material->emissiveTex) {
		bgfx::setTexture(2, s_texEmissive, material->emissiveTex->handle);
	}
	else {
		if (defaultEmissiveTexture) {
			bgfx::setTexture(2, s_texEmissive, defaultEmissiveTexture->handle);
		}
	}

	for (const auto& colorProp : material->colors) {
		auto it = colorUniforms.find(colorProp.name);
		bgfx::UniformHandle uniform;
		if (it == colorUniforms.end()) {
			uniform = bgfx::createUniform(colorProp.name.c_str(), bgfx::UniformType::Vec4);
			colorUniforms[colorProp.name] = uniform;
		}
		else {
			uniform = it->second;
		}
		bgfx::setUniform(uniform, &colorProp.value);
	}

	for (const auto& vecProp : material->vectors) {
		auto it = vectorUniforms.find(vecProp.name);
		bgfx::UniformHandle uniform;
		if (it == vectorUniforms.end()) {
			uniform = bgfx::createUniform(vecProp.name.c_str(), bgfx::UniformType::Vec4);
			vectorUniforms[vecProp.name] = uniform;
		}
		else {
			uniform = it->second;
		}
		bgfx::setUniform(uniform, &vecProp.value);
	}

	//TODO i based on shader uniformInfo samplers order ?
	int i = 0;
	for (const auto& texProp : material->textures) {
		auto it = textureUniforms.find(texProp.name);
		bgfx::UniformHandle uniform;
		if (it == textureUniforms.end()) {
			uniform = bgfx::createUniform(texProp.name.c_str(), bgfx::UniformType::Sampler);
			textureUniforms[texProp.name] = uniform;
		}
		else {
			uniform = it->second;
		}
		if (texProp.value) {
			bgfx::setTexture(i++, uniform, texProp.value->handle);
		}
		i++;
	}
}

bgfx::UniformHandle Render::GetOrCreateVectorUniform(const std::string& name) {


	auto it = vectorUniforms.find(name);
	bgfx::UniformHandle uniform;
	if (it == vectorUniforms.end()) {
		uniform = bgfx::createUniform(name.c_str(), bgfx::UniformType::Vec4);
		vectorUniforms[name] = uniform;
	}
	else {
		uniform = it->second;
	}
	return uniform;
}