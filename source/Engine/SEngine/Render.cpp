

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
constexpr bgfx::ViewId kRenderPassBlitToScreen = 1;
constexpr bgfx::ViewId kRenderPassGeometry = 11;
constexpr bgfx::ViewId kRenderPassLight = 10;
constexpr bgfx::ViewId kRenderPassDebugLights = 12;
constexpr bgfx::ViewId kRenderPassDebugGBuffer = 13;
constexpr bgfx::ViewId kRenderPassFullScreen1 = 14;
constexpr bgfx::ViewId kRenderPassFullScreen2 = 15;
constexpr bgfx::ViewId kRenderPassFree = 16;


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

void Render::PrepareLights(const ICamera& camera) {
	OPTICK_EVENT();
	if (!deferredLightShader || !deferredDirLightShader) {
		return;
	}
	std::vector<PointLight*> pointLights = PointLight::pointLights;
	std::vector<PointLight*> visiblePointLights;

	for (auto light : pointLights) {
		if (light->radius <= 0.f) {
			continue;
		}
		if (light->color.r <= 0.f && light->color.g <= 0.f && light->color.b <= 0.f) {
			continue;
		}

		auto pos = light->gameObject()->transform()->GetPosition();
		auto sphere = Sphere{ pos, light->radius };
		bool isVisible = camera.IsVisible(sphere);
		if (!isVisible) {
			continue;
		}
		visiblePointLights.push_back(light);
		//TODO
		shadowRenderer->Draw(light, camera);

		//auto posRadius = Vector4(pos, light->radius);
		//bgfx::setUniform(u_lightPosRadius, &posRadius, 1);

		//auto colorInnerRadius = Vector4(light->color.r, light->color.g, light->color.b, light->innerRadius);
		//bgfx::setUniform(u_lightRgbInnerR, &colorInnerRadius, 1);

		//AABB aabb = sphere.ToAABB();

		//const Vector3 box[8] =
		//{
		//	{ aabb.min.x, aabb.min.y, aabb.min.z },
		//	{ aabb.min.x, aabb.min.y, aabb.max.z },
		//	{ aabb.min.x, aabb.max.y, aabb.min.z },
		//	{ aabb.min.x, aabb.max.y, aabb.max.z },
		//	{ aabb.max.x, aabb.min.y, aabb.min.z },
		//	{ aabb.max.x, aabb.min.y, aabb.max.z },
		//	{ aabb.max.x, aabb.max.y, aabb.min.z },
		//	{ aabb.max.x, aabb.max.y, aabb.max.z },
		//};


		//Vector3 xyz = multH(camera.GetViewProjectionMatrix(), box[0]);
		//Vector3 min = xyz;
		//Vector3 max = xyz;

		//for (uint32_t ii = 1; ii < 8; ++ii)
		//{
		//	xyz = multH(camera.GetViewProjectionMatrix(), box[ii]);
		//	min = Vector3::Min(min, xyz);
		//	max = Vector3::Max(max, xyz);
		//}

		//float x0 = Mathf::Clamp((min.x * 0.5f + 0.5f) * prevWidth, 0.0f, (float)prevWidth);
		//float y0 = Mathf::Clamp((min.y * 0.5f + 0.5f) * prevHeight, 0.0f, (float)prevHeight);
		//float x1 = Mathf::Clamp((max.x * 0.5f + 0.5f) * prevWidth, 0.0f, (float)prevWidth);
		//float y1 = Mathf::Clamp((max.y * 0.5f + 0.5f) * prevHeight, 0.0f, (float)prevHeight);

		//if (aabb.Contains(camera.GetPosition())) {
		//	x0 = 0.f;
		//	y0 = 0.f;
		//	x1 = prevWidth;
		//	y1 = prevHeight;
		//}

		//bgfx::setTexture(0, gBuffer.normalSampler, gBuffer.normalTexture);
		//bgfx::setTexture(1, gBuffer.depthSampler, gBuffer.depthTexture);
		//const uint16_t scissorHeight = uint16_t(y1 - y0);
		////TODO something wrong
		//bgfx::setScissor(uint16_t(x0), uint16_t(prevHeight - scissorHeight - y0), uint16_t(x1 - x0), uint16_t(scissorHeight));
		//bgfx::setState(0
		//	| BGFX_STATE_WRITE_RGB
		//	| BGFX_STATE_WRITE_A
		//	| BGFX_STATE_BLEND_ADD
		//);
		//Graphics::Get()->SetScreenSpaceQuadBuffer();
		//bgfx::submit(kRenderPassLight, deferredLightShader->program);
	}

	std::vector<DirLight*> dirLights = DirLight::dirLights;
	for (auto light : dirLights) {
		if (light->color.r <= 0.f && light->color.g <= 0.f && light->color.b <= 0.f) {
			continue;
		}
		shadowRenderer->Draw(light, camera);
		auto dir = Vector4(GetRot(light->gameObject()->transform()->matrix) * Vector3_forward, 0.f);
		auto color = Vector4(light->color.r, light->color.g, light->color.b, 0.f);
		bgfx::setUniform(u_dirLightDirHandle, &dir, 1);
		bgfx::setUniform(u_dirLightColorHandle, &color, 1);

		//bgfx::setTexture(0, gBuffer.normalSampler, gBuffer.normalTexture);
		//bgfx::setTexture(1, gBuffer.depthSampler, gBuffer.depthTexture);
		//bgfx::setState(0
		//	| BGFX_STATE_WRITE_RGB
		//	| BGFX_STATE_WRITE_A
		//	| BGFX_STATE_BLEND_ADD
		//);
		//Graphics::Get()->SetScreenSpaceQuadBuffer();
		//bgfx::submit(kRenderPassLight, deferredDirLightShader->program);
	}


	struct ClusterData {
		uint32_t offset = 0;
		uint8_t numLights = 0;
		uint8_t numDecals = 0;
		uint8_t numProbes = 0;
		uint8_t padding;
	};
	struct ItemData {
		uint16_t lightIdx;
		uint16_t dummy;
	};
	//TODO adjust for dir light
	struct LightData {
		Vector3 pos;
		float radius;
		Vector3 color;
		float innerRadius;
	};
	auto viewProj = camera.GetViewProjectionMatrix();
	Vector3 clusterScale = Vector3(1.f / clusterWidth, 1.f / clusterHeight, 1.f / clusterDepth);
	Vector3 clusterScaleInv = 1.f / clusterScale;
	Vector3 clusterOffsetInitial = Vector3(-1.f + clusterScale.x, -1.f + clusterScale.y, -1.f + clusterScale.z);

	std::vector<ItemData> items;
	//items.resize(maxItemsCount);
	std::vector<LightData> lights;
	//lights.resize(maxLightsCount);
	std::vector<ClusterData> clusters;
	clusters.resize(clustersCount);
	int currentOffset = 0;
	for (int lightIdx = 0; lightIdx < visiblePointLights.size(); lightIdx++) {
		auto light = visiblePointLights[lightIdx];
		LightData lightData;
		lightData.pos = light->gameObject()->transform()->GetPosition();
		lightData.innerRadius = light->innerRadius;
		lightData.color.x = light->color.r;
		lightData.color.y = light->color.g;
		lightData.color.z = light->color.b;
		lightData.radius = light->radius;
		lights.push_back(lightData);
	}
	for (int w = 0; w < clusterWidth; w++) {
		for (int h = 0; h < clusterHeight; h++) {
			for (int d = 0; d < clusterDepth; d++) {
				int offsetBefore = currentOffset;
				Vector3 pos = clusterOffsetInitial + clusterScale * Vector3(w, h, d) * 2.f;

				//inverse of Matrix4::Transform(pos, identity, clusterScale)
				auto extraMat = Matrix4::Identity();
				extraMat(0, 0) = clusterScaleInv.x;
				extraMat(1, 1) = clusterScaleInv.y;
				extraMat(2, 2) = clusterScaleInv.z;
				extraMat(0, 3) = -pos.x * clusterScaleInv.x;
				extraMat(1, 3) = -pos.y * clusterScaleInv.y;
				extraMat(2, 3) = -pos.z * clusterScaleInv.z;


				auto clusterViewProj = extraMat * viewProj;

				int idx = w * clusterDepth * clusterHeight + h * clusterDepth + d;

				Frustum frustum;
				frustum.SetFromViewProjection(clusterViewProj);

				//Dbg::Draw(frustum);

				int numLights = 0;
				for (int lightIdx = 0; lightIdx < visiblePointLights.size(); lightIdx++) {
					auto pos = lights[lightIdx].pos;
					auto sphere = Sphere{ pos, lights[lightIdx].radius };
					if (frustum.IsOverlapingSphere(sphere)) {
						numLights++;
						auto data = ItemData();
						data.lightIdx = lightIdx;
						items.push_back(data);
						currentOffset++;
					}
				}

				clusters[idx].offset = offsetBefore;
				clusters[idx].numLights = numLights;
				clusters[idx].numDecals = 0;
				clusters[idx].numProbes = 0;
				clusters[idx].padding = 0;
			}
		}
	}

	bgfx::updateTexture2D(m_clusterListTex, 0, 0, 0, 0, clustersCount, 1, bgfx::copy(clusters.data(), clusters.size() * sizeof(ClusterData)));

	int itemsTexelsTotal = items.size() * texelsPerItem;
	int itemsWidth = Mathf::Min(itemsTexelsTotal, itemsDiv);
	int itemsHeight = 1 + (itemsTexelsTotal - 1) / itemsDiv;
	items.resize(itemsHeight * itemsWidth / texelsPerItem);
	bgfx::updateTexture2D(m_itemsListTex, 0, 0, 0, 0, itemsWidth, itemsHeight, bgfx::copy(items.data(), items.size() * sizeof(ItemData)));
	bgfx::updateTexture2D(m_lightsListTex, 0, 0, 0, 0, lights.size() * texelsPerLight, 1, bgfx::copy(lights.data(), lights.size() * sizeof(LightData)));
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
#ifdef SE_DBG_OUT
	//initInfo.limits.transientVbSize *= 10;//TODO debug only
	//initInfo.limits.transientIbSize *= 10;//TODO debug only
#endif
	bgfx::init(initInfo);
	bgfx::reset(width, height, CfgGetBool("vsync") ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);

	bgfx::resetView(kRenderPassGeometry);
	bgfx::resetView(1);
	bgfx::setDebug(BGFX_DEBUG_TEXT /*| BGFX_DEBUG_STATS*/);

	bgfx::setViewRect(kRenderPassGeometry, 0, 0, width, height);
	bgfx::setViewRect(1, 0, 0, width, height);

	u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
	u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
	s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
	s_texEmissive = bgfx::createUniform("s_texEmissive", bgfx::UniformType::Sampler);
	s_texClusterList = bgfx::createUniform("s_texClusterList", bgfx::UniformType::Sampler);
	s_texItemsList = bgfx::createUniform("s_texItemsList", bgfx::UniformType::Sampler);
	s_texLightsList = bgfx::createUniform("s_texLightsList", bgfx::UniformType::Sampler);

	//u_lightPosRadius = bgfx::createUniform("u_lightPosRadius", bgfx::UniformType::Vec4, maxLightsCount);
	//u_lightRgbInnerR = bgfx::createUniform("u_lightRgbInnerR", bgfx::UniformType::Vec4, maxLightsCount);
	u_viewProjInv = bgfx::createUniform("u_viewProjInv", bgfx::UniformType::Mat4);

	u_sphericalHarmonics = bgfx::createUniform("u_sphericalHarmonics", bgfx::UniformType::Vec4, 9);

	u_pixelSize = bgfx::createUniform("u_pixelSize", bgfx::UniformType::Vec4);

	u_dirLightDirHandle = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
	u_dirLightColorHandle = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4);


	m_fullScreenTex.idx = bgfx::kInvalidHandle;
	m_fullScreenBuffer.idx = bgfx::kInvalidHandle;
	m_fullScreenTex2.idx = bgfx::kInvalidHandle;
	m_fullScreenBuffer2.idx = bgfx::kInvalidHandle;
	gBuffer.buffer.idx = bgfx::kInvalidHandle;

	prevWidth = width;
	prevHeight = height;

	shadowRenderer = std::make_shared<ShadowRenderer>();
	shadowRenderer->Init();

	imguiCreate();
	ImGui_ImplSDL2_InitForMetal(window);

	Dbg::Init();

	LoadAssets();
	//TODO is it good idea to load something after database is unloaded ?
	databaseAfterUnloadedHandle = AssetDatabase::Get()->onAfterUnloaded.Subscribe([this]() {LoadAssets(); });
	databaseBeforeUnloadedHandle = AssetDatabase::Get()->onBeforeUnloaded.Subscribe([this]() {UnloadAssets(); });

	return true;
}

void Render::LoadAssets() {
	auto* database = AssetDatabase::Get();
	if (database == nullptr) {
		return;;//HACK for terminated app
	}
	whiteTexture = database->Load<Texture>("textures\\white.png");
	defaultNormalTexture = database->Load<Texture>("textures\\defaultNormal.png");
	defaultEmissiveTexture = database->Load<Texture>("textures\\defaultEmissive.png");
	simpleBlitMat = database->Load<Material>("materials\\simpleBlit.asset");
	deferredLightShader = database->Load<Shader>("shaders\\deferredLight.asset");
	deferredDirLightShader = database->Load<Shader>("shaders\\deferredDirLight.asset");
	defferedCombineMaterial = database->Load<Material>("materials\\deferredCombine.asset");
}

void Render::UnloadAssets() {
	//TODO terminate whole database textures, shaders and other stuff on Render::Term()

	whiteTexture = nullptr;
	defaultNormalTexture = nullptr;
	defaultEmissiveTexture = nullptr;
	simpleBlitMat = nullptr;
	deferredLightShader = nullptr;
	deferredDirLightShader = nullptr;
	defferedCombineMaterial = nullptr;
}
void Render::Draw(SystemsManager& systems)
{
	OPTICK_EVENT();

	currentFreeViewId = kRenderPassFree;
	currentFullScreenTextureIdx = 0;

	//TODO not on draw!

	//TODO not here
	if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_RETURN) && (Input::GetKey(SDL_Scancode::SDL_SCANCODE_LALT) || Input::GetKey(SDL_Scancode::SDL_SCANCODE_RALT))) {
		SetFullScreen(!IsFullScreen());
	}

	int width;
	int height;
	SDL_GetWindowSize(window, &width, &height);

	if (height != prevHeight || width != prevWidth || !bgfx::isValid(m_fullScreenBuffer) || !bgfx::isValid(m_fullScreenBuffer2)) {
		prevWidth = width;
		prevHeight = height;
		bgfx::reset(width, height, CfgGetBool("vsync") ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);
		if (bgfx::isValid(m_fullScreenBuffer)) {
			bgfx::destroy(m_fullScreenBuffer);
		}
		if (bgfx::isValid(m_fullScreenBuffer2)) {
			bgfx::destroy(m_fullScreenBuffer2);
		}
		if (bgfx::isValid(gBuffer.buffer)) {
			bgfx::destroy(gBuffer.buffer);
		}

		const uint64_t tsFlags = 0
			| BGFX_TEXTURE_RT
			| BGFX_SAMPLER_U_CLAMP
			| BGFX_SAMPLER_V_CLAMP
			;

		//TODO dont recreate on resize
		m_clusterListTex = bgfx::createTexture2D(clustersCount * texelsPerCluster, 1, false, 1, bgfx::TextureFormat::RG32U, BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
		int itemsWidth = itemsDiv;
		int itemsHeight = (maxItemsCount * texelsPerItem) / itemsWidth + 1;
		m_itemsListTex = bgfx::createTexture2D(itemsWidth, itemsHeight, false, 1, bgfx::TextureFormat::RG16U, BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
		m_lightsListTex = bgfx::createTexture2D(maxLightsCount * texelsPerLight, 1, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
		//TODO they are not same size as clustersCount!!!

		m_fullScreenTex = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		m_fullScreenBuffer = bgfx::createFrameBuffer(1, &m_fullScreenTex, true);
		m_fullScreenTex2 = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		m_fullScreenBuffer2 = bgfx::createFrameBuffer(1, &m_fullScreenTex2, true);

		gBuffer.albedoTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		//gBuffer.normalTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		//gBuffer.lightTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		//gBuffer.emissiveTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		gBuffer.depthTexture = bgfx::createTexture2D(uint16_t(width), uint16_t(height), false, 1, bgfx::TextureFormat::D32F, tsFlags);
		bgfx::TextureHandle gbufferTex[] =
		{
			gBuffer.albedoTexture,
			//gBuffer.normalTexture,
			//gBuffer.lightTexture,
			//gBuffer.emissiveTexture,
			gBuffer.depthTexture
		};
		gBuffer.buffer = bgfx::createFrameBuffer(BX_COUNTOF(gbufferTex), gbufferTex, true);
		//gBuffer.lightBuffer = bgfx::createFrameBuffer(1, &gBuffer.lightTexture, true);//TODO delete
		//TODO destroy
		gBuffer.albedoSampler = bgfx::createUniform("s_albedo", bgfx::UniformType::Sampler);
		//gBuffer.normalSampler = bgfx::createUniform("s_normals", bgfx::UniformType::Sampler);
		//gBuffer.lightSampler = bgfx::createUniform("s_light", bgfx::UniformType::Sampler);
		//gBuffer.emissiveSampler = bgfx::createUniform("s_emissive", bgfx::UniformType::Sampler);
		gBuffer.depthSampler = bgfx::createUniform("s_depth", bgfx::UniformType::Sampler);

		bgfx::setName(gBuffer.buffer, "gbuffer");
		bgfx::setName(gBuffer.albedoTexture, "albedo");
		bgfx::setName(m_clusterListTex, "clusterList");
		bgfx::setName(m_itemsListTex, "itemsList");
		bgfx::setName(m_lightsListTex, "lightsList");
		//bgfx::setName(gBuffer.normalTexture, "normal");
		//bgfx::setName(gBuffer.lightTexture, "light");
		//bgfx::setName(gBuffer.lightBuffer, "light");
		//bgfx::setName(gBuffer.emissiveTexture, "emissive");
		bgfx::setName(gBuffer.depthTexture, "depth");

		bgfx::setName(m_fullScreenTex, "fullScreenTex");
		bgfx::setName(m_fullScreenBuffer, "fullScreenBuffer");
		bgfx::setName(m_fullScreenTex2, "fullScreenTex2");
		bgfx::setName(m_fullScreenBuffer2, "fullScreenBuffer2");

		bgfx::setViewFrameBuffer(kRenderPassGeometry, gBuffer.buffer);
		bgfx::setViewFrameBuffer(kRenderPassFullScreen1, m_fullScreenBuffer);
		bgfx::setViewFrameBuffer(kRenderPassFullScreen2, m_fullScreenBuffer2);
		//bgfx::setViewFrameBuffer(kRenderPassLight, gBuffer.lightBuffer);
		bgfx::setViewFrameBuffer(kRenderPassBlitToScreen, BGFX_INVALID_HANDLE);//TODO do we need third buffer to go postprocessing?

	}

	bgfx::dbgTextClear();
	bgfx::setViewRect(kRenderPassGeometry, 0, 0, width, height);
	bgfx::setViewRect(1, 0, 0, width, height);
	bgfx::setViewRect(kRenderPassLight, 0, 0, width, height);
	bgfx::setViewRect(kRenderPassLight, 0, 0, width, height);
	bgfx::setViewRect(kRenderPassFullScreen1, 0, 0, width, height);
	bgfx::setViewRect(kRenderPassFullScreen2, 0, 0, width, height);

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
		bgfx::setViewClear(kRenderPassGeometry, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, Colors::black.ToIntRGBA(), 1.0f, 0);
		bgfx::dbgTextPrintf(1, 1, 0x0f, "NO CAMERA");
		EndFrame();
		return;
	}

	camera->OnBeforeRender();

	uint8_t clearAlbedo = 1;
	uint8_t clearOther = 2;
	bgfx::setPaletteColor(clearAlbedo, camera->GetClearColor().ToIntRGBA());
	bgfx::setPaletteColor(clearOther, Colors::black.ToIntRGBA());
	bgfx::setViewClear(kRenderPassGeometry, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 1.0f, 0, clearAlbedo, clearOther, clearOther, clearOther, clearOther, clearOther, clearOther, clearOther);

	Vector3 lightsPoi = GetPos(camera->gameObject()->transform()->matrix) + GetRot(camera->gameObject()->transform()->matrix) * Vector3_forward * 15.f;

	static float fps = 0;
	{
		//TODO not here
		float realTime = Time::getRealTime();
		static float prevTimeCheck = 0;
		static int prevFramesCount = 0;
		if ((int)realTime != (int)prevTimeCheck) {
			fps = (Time::frameCount() - prevFramesCount) / (realTime - prevTimeCheck);
			prevTimeCheck = realTime;
			prevFramesCount = Time::frameCount();
		}
	}
	float time = Time::time();

	{
		float time = Time::time();
		bgfx::setUniform(u_time, &time);
	}
	{
		OPTICK_EVENT("bgfx::touch");
		bgfx::touch(kRenderPassGeometry);//TODO not needed ?
	}

	if (CfgGetBool("showFps")) {
		bgfx::dbgTextPrintf(1, 2, 0x0f, "FPS: %.1f", fps);
	}

	if (camera == nullptr) {
		if (camera == nullptr) {
			bgfx::setViewClear(kRenderPassGeometry, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, Colors::black.ToIntRGBA(), 1.0f, 0);
			bgfx::dbgTextPrintf(1, 1, 0x0f, "NO CAMERA");
		}
	}


	{
		bgfx::setViewTransform(kRenderPassGeometry, &camera->GetViewMatrix()(0, 0), &camera->GetProjectionMatrix()(0, 0));
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
		bgfx::setViewTransform(kRenderPassFullScreen1, nullptr, proj);

	}

	UpdateLights(lightsPoi);


	dbgMeshesDrawn = 0;
	dbgMeshesCulled = 0;

	PrepareLights(*camera);

	systems.Draw();

	DrawAll(kRenderPassGeometry, *camera, nullptr);

	// combining gbuffer
	{
		//if (!defferedCombineMaterial || !defferedCombineMaterial->shader) {
		//	return;
		//}
		//ApplyMaterialProperties(defferedCombineMaterial.get());

		//bgfx::setTexture(0, gBuffer.albedoSampler, gBuffer.albedoTexture);
		//bgfx::setTexture(1, gBuffer.normalSampler, gBuffer.normalTexture);
		//bgfx::setTexture(2, gBuffer.lightSampler, gBuffer.lightTexture);
		//bgfx::setTexture(3, gBuffer.emissiveSampler, gBuffer.emissiveTexture);
		//bgfx::setTexture(4, gBuffer.depthSampler, gBuffer.depthTexture);

		//bgfx::setState(0
		//	| BGFX_STATE_WRITE_RGB
		//	| BGFX_STATE_WRITE_A
		//);

		//Graphics::Get()->SetScreenSpaceQuadBuffer();
		//bgfx::submit(kRenderPassFullScreen1, defferedCombineMaterial->shader->program);
		//bgfx::setViewFrameBuffer(currentFreeViewId, currentFullScreenTextureIdx == 1 ? m_fullScreenBuffer : m_fullScreenBuffer2);
		//bgfx::setViewRect(currentFreeViewId, 0, 0, prevWidth, prevHeight);
	}
	{
		bgfx::setTexture(0, gBuffer.albedoSampler, gBuffer.albedoTexture);
		bgfx::setState(0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
		);
		Graphics::Get()->SetScreenSpaceQuadBuffer();
		bgfx::submit(kRenderPassFullScreen1, simpleBlitMat->shader->program);
		bgfx::setViewFrameBuffer(currentFreeViewId, currentFullScreenTextureIdx == 1 ? m_fullScreenBuffer : m_fullScreenBuffer2);
		bgfx::setViewRect(currentFreeViewId, 0, 0, prevWidth, prevHeight);
	}

	RenderEvents::Get()->onSceneRendered.Invoke(*this);

	Graphics::Get()->Blit(simpleBlitMat, kRenderPassBlitToScreen);

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
	if (AssetDatabase::Get()) {
		//HACK when database is terminated
		AssetDatabase::Get()->onBeforeUnloaded.Unsubscribe(databaseBeforeUnloadedHandle);
		AssetDatabase::Get()->onAfterUnloaded.Unsubscribe(databaseAfterUnloadedHandle);
	}
	Dbg::Term();
	UnloadAssets();

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
	bgfx::destroy(s_texClusterList);
	bgfx::destroy(s_texItemsList);
	bgfx::destroy(s_texLightsList);

	if (bgfx::isValid(m_fullScreenBuffer)) {
		bgfx::destroy(m_fullScreenBuffer);
	}
	if (bgfx::isValid(m_fullScreenBuffer2)) {
		bgfx::destroy(m_fullScreenBuffer2);
	}
	if (bgfx::isValid(gBuffer.buffer)) {
		bgfx::destroy(gBuffer.buffer);
	}
	if (bgfx::isValid(m_clusterListTex)) {
		bgfx::destroy(m_clusterListTex);
	}
	if (bgfx::isValid(m_itemsListTex)) {
		bgfx::destroy(m_itemsListTex);
	}
	if (bgfx::isValid(m_lightsListTex)) {
		bgfx::destroy(m_lightsListTex);
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

bgfx::TextureHandle Render::GetCurrentFullScreenTexture() const {
	return currentFullScreenTextureIdx == 0 ? m_fullScreenTex : m_fullScreenTex2;
}

bgfx::TextureHandle Render::GetNextFullScreenTexture() const {
	return currentFullScreenTextureIdx == 1 ? m_fullScreenTex : m_fullScreenTex2;
}

void Render::FlipFullScreenTextures() {
	currentFullScreenTextureIdx = (currentFullScreenTextureIdx + 1) % 2;
	currentFreeViewId++;
	bgfx::setViewFrameBuffer(currentFreeViewId, currentFullScreenTextureIdx == 1 ? m_fullScreenBuffer : m_fullScreenBuffer2);
	bgfx::setViewRect(currentFreeViewId, 0, 0, prevWidth, prevHeight);
}

int Render::GetNextFullScreenTextureViewId() const {
	return currentFreeViewId;
}

void Render::UpdateLights(Vector3 poi) {
	//std::vector<PointLight*> lights = PointLight::pointLights;

	//Vector4 posRadius[maxLightsCount];
	//Vector4 colorInnerRadius[maxLightsCount];

	//for (int i = 0; i < maxLightsCount; i++) {
	//	if (lights.size() == 0) {
	//		for (int j = i; j < maxLightsCount; j++) {
	//			posRadius[j] = Vector4(FLT_MAX, FLT_MAX, FLT_MAX, 0.f);
	//			colorInnerRadius[j].x = 0;
	//			colorInnerRadius[j].y = 0;
	//			colorInnerRadius[j].z = 0;
	//			colorInnerRadius[j].w = 0;
	//		}
	//		break;
	//	}
	//	int closestLightIdx = 0;
	//	float closestDistance = FLT_MAX;
	//	for (int lightIdx = 0; lightIdx < lights.size(); lightIdx++) {
	//		float distance = Vector3::Distance(poi, lights[lightIdx]->gameObject()->transform()->GetPosition());
	//		distance -= lights[lightIdx]->radius;
	//		if (distance < closestDistance) {
	//			closestDistance = distance;
	//			closestLightIdx = lightIdx;
	//		}
	//	}

	//	auto closestLight = lights[closestLightIdx];
	//	lights.erase(lights.begin() + closestLightIdx);

	//	Vector3 pos = closestLight->gameObject()->transform()->GetPosition();
	//	posRadius[i] = Vector4(pos, closestLight->radius);
	//	Color color = closestLight->color;
	//	colorInnerRadius[i] = Vector4(color.r, color.g, color.b, closestLight->innerRadius);
	//}
	//bgfx::setUniform(u_lightPosRadius, &posRadius, maxLightsCount);
	//bgfx::setUniform(u_lightRgbInnerR, &colorInnerRadius, maxLightsCount);

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
			bool drawn = DrawMesh(mesh, mesh->material.get(), camera, !nextEq, !nextEq, !prevEq, !prevEq, viewId);
			prevEq = nextEq && drawn;
		}
		//TODO do we really need this ?
		if (MeshRenderer::enabledMeshRenderers.size() > 0) {
			const auto& mesh = MeshRenderer::enabledMeshRenderers[MeshRenderer::enabledMeshRenderers.size() - 1];
			DrawMesh(mesh, mesh->material.get(), camera, true, true, !prevEq, !prevEq, viewId);
		}
	}
	else {
		//TODO refactor duplicated code

		auto renderersSort = [](const MeshRenderer* r1, const MeshRenderer* r2) {
			return r1->mesh.get() < r2->mesh.get();
		};
		{
			//assuming renderers are already sorted by mesh type
			// 
			//OPTICK_EVENT("SortRenderers");
			//auto& renderers = MeshRenderer::enabledMeshRenderers;
			//std::sort(renderers.begin(), renderers.end(), renderersSort);
		}


		OPTICK_EVENT("DrawRenderers");
		bool prevEq = false;
		bool materialApplied = false;//TODO dsplit drawMesh into bind material / bind mesh / submit / clear
		//TODO get rid of std::vector / std::string
		for (int i = 0; i < ((int)MeshRenderer::enabledMeshRenderers.size() - 1); i++) {
			const auto& mesh = MeshRenderer::enabledMeshRenderers[i];
			const auto& meshNext = MeshRenderer::enabledMeshRenderers[i + 1];
			//TODO optimize
			bool nextEq = !renderersSort(mesh, meshNext) && !renderersSort(meshNext, mesh);
			bool drawn = DrawMesh(mesh, overrideMaterial.get(), camera, false, !nextEq, !materialApplied, !prevEq, viewId);
			prevEq = nextEq && drawn;
			materialApplied |= drawn;
		}
		if (MeshRenderer::enabledMeshRenderers.size() > 0) {
			const auto& mesh = MeshRenderer::enabledMeshRenderers[MeshRenderer::enabledMeshRenderers.size() - 1];
			DrawMesh(mesh, overrideMaterial.get(), camera, true, true, !materialApplied, !prevEq, viewId);
		}
	}
}

bool Render::DrawMesh(const MeshRenderer* renderer, const Material* material, const ICamera& camera, bool clearMaterialState, bool clearMeshState, bool updateMaterialState, bool updateMeshState, int viewId) {
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

	if (updateMaterialState) {
		ApplyMaterialProperties(material);
		if (material->colorTex) {
			if (material->randomColorTextures.size() > 0) {
				bgfx::setTexture(0, s_texColor, material->randomColorTextures[renderer->randomColorTextureIdx]->handle);
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
	}
	if (updateMeshState) {
		bgfx::setVertexBuffer(0, renderer->mesh->vertexBuffer);
		bgfx::setIndexBuffer(renderer->mesh->indexBuffer);
	}

	auto discardFlags = BGFX_DISCARD_NONE;
	if (clearMaterialState) {
		discardFlags = Bits::SetMaskTrue(discardFlags, BGFX_DISCARD_BINDINGS | BGFX_DISCARD_STATE);
	}
	if (clearMeshState) {
		discardFlags = Bits::SetMaskTrue(discardFlags, BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS);
	}
	if (clearMaterialState && clearMeshState) {
		discardFlags = Bits::SetMaskTrue(discardFlags, BGFX_DISCARD_ALL);
	}
	bgfx::submit(viewId, material->shader->program, 0u, discardFlags);
	return true;
}

void Render::ApplyMaterialProperties(const Material* material) {
	shadowRenderer->ApplyUniforms();

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
	bgfx::setTexture(3, s_texClusterList, m_clusterListTex);
	bgfx::setTexture(4, s_texItemsList, m_itemsListTex);
	bgfx::setTexture(5, s_texLightsList, m_lightsListTex);

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