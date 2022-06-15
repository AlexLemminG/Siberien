#include "pch.h"

#include "Render.h"

#include "bgfx/bgfx.h"
#include "Common.h"

#include "Render.h"

#include "SDL.h"
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
#include "Editor.h"
#include "DbgVars.h"
#include "bx/bx.h"
#include "bx/platform.h"
#include "bx/file.h"
#include "bx/mutex.h"
#include "bx/debug.h"
#include "bimg/bimg.h"
#include "dear-imgui/imgui_impl_sdl.h"

DBG_VAR_BOOL(dbg_debugShadows, "Debug Shadows", false);
DBG_VAR_BOOL(dbg_drawBones, "Draw Bones", false);

REFLECT_ENUM(bgfx::RendererType::Enum);

static std::shared_ptr<RenderSettings> s_renderSettings;
class RenderSettings : public Object {
public:
	bool frustumCulling = true;
	int msaa = 0;//TODO min/max
	bool vsync = false;
	bool debug = false;
	bool preSortRenderers = true;
	bool preSortShadowRenderers = true;
	bool tryToMinimizeStateChanges = true;
	bool bgfxDebugStats = false;
	bool maxAnisotropy = true;
	bgfx::RendererType::Enum backend = bgfx::RendererType::Vulkan;

	REFLECT_BEGIN(RenderSettings);
	REFLECT_VAR(frustumCulling);
	REFLECT_VAR(msaa);
	REFLECT_VAR(vsync);
	REFLECT_VAR(debug);
	REFLECT_VAR(preSortRenderers);
	REFLECT_VAR(preSortShadowRenderers);
	REFLECT_VAR(tryToMinimizeStateChanges);
	REFLECT_VAR(bgfxDebugStats);
	REFLECT_VAR(maxAnisotropy);
	REFLECT_VAR(backend);
	REFLECT_END();
};

DECLARE_TEXT_ASSET(RenderSettings);

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

void Render::PrepareLights(const Camera& camera) {
	OPTICK_EVENT();

	//TODO adjust for dir light
	struct LightData {
		Vector3 pos;
		float radius;
		Vector3 color;
		float innerRadius;
		//TODO maybe different structure for spot lights
		//TODO pack minAngle in dir ?
		Vector3 dir;
		float halfInnerAngle;
		float halfDeltaAngleInv;
		float padding1;
		float padding2;
		float padding3;
	};
	std::vector<LightData> lights;

	const std::vector<PointLight*>& pointLights = PointLight::pointLights;
	const std::vector<SpotLight*>& spotLights = SpotLight::spotLights;

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

		LightData lightData;
		lightData.pos = light->gameObject()->transform()->GetPosition();
		lightData.innerRadius = light->innerRadius;
		lightData.color.x = light->color.r * light->intensity;
		lightData.color.y = light->color.g * light->intensity;
		lightData.color.z = light->color.b * light->intensity;
		lightData.radius = light->radius;
		lightData.dir = light->gameObject()->transform()->GetForward();
		lightData.halfInnerAngle = Mathf::pi;
		lightData.halfDeltaAngleInv = Mathf::pi;
		lights.push_back(lightData);

		//TODO
		//shadowRenderer->Draw(light, camera);
	}

	for (auto light : spotLights) {
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

		LightData lightData;
		lightData.pos = light->gameObject()->transform()->GetPosition();
		lightData.innerRadius = light->innerRadius;
		lightData.color.x = light->color.r * light->intensity;
		lightData.color.y = light->color.g * light->intensity;
		lightData.color.z = light->color.b * light->intensity;
		lightData.radius = light->radius;
		lightData.dir = light->gameObject()->transform()->GetForward();
		lightData.halfInnerAngle = Mathf::DegToRad(light->innerAngle / 2.f);
		lightData.halfDeltaAngleInv = 1.f / Mathf::Max(0.001f, Mathf::DegToRad(light->outerAngle / 2.f) - lightData.halfInnerAngle);
		lights.push_back(lightData);

		//TODO
		//shadowRenderer->Draw(light, camera);
	}

	std::vector<DirLight*> dirLights = DirLight::dirLights;
	bool hasDirLights = false;
	for (auto light : dirLights) {
		if (light->color.r <= 0.f && light->color.g <= 0.f && light->color.b <= 0.f) {
			continue;
		}
		hasDirLights = true;
		shadowRenderer->Draw(this, light, camera);
		auto dir = Vector4(light->gameObject()->transform()->GetForward(), 0.f);
		//TODO Color to Vector4 func
		auto color = Vector4(light->color.r, light->color.g, light->color.b, 0.f) * light->intensity;
		//TODO support more than 1 dir light
		bgfx::setUniform(u_dirLightDirHandle, &dir, 1);
		bgfx::setUniform(u_dirLightColorHandle, &color, 1);
	}
	if (!hasDirLights) {
		//no dir lights
		Vector4 color = Vector4(0, 0, 0, 0);
		bgfx::setUniform(u_dirLightColorHandle, &color, 1);
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
	auto viewProj = camera.GetViewProjectionMatrix();
	Vector3 clusterScale = Vector3(1.f / clusterWidth, 1.f / clusterHeight, 1.f / clusterDepth);
	Vector3 clusterScaleInv = 1.f / clusterScale;
	Vector3 clusterOffsetInitial = Vector3(-1.f + clusterScale.x, -1.f + clusterScale.y, -1.f + clusterScale.z);

	std::vector<ItemData> items;
	//items.resize(maxItemsCount);
	//lights.resize(maxLightsCount);
	std::vector<ClusterData> clusters;
	clusters.resize(clustersCount);
	int currentOffset = 0;

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
				for (int lightIdx = 0; lightIdx < lights.size(); lightIdx++) {
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

	if (items.size() > 0) {

		int itemsTexelsTotal = items.size() * texelsPerItem;
		int itemsWidth = Mathf::Min(itemsTexelsTotal, itemsDiv);
		int itemsHeight = 1 + (itemsTexelsTotal - 1) / itemsDiv;
		items.resize(itemsHeight * itemsWidth / texelsPerItem);
		bgfx::updateTexture2D(m_itemsListTex, 0, 0, 0, 0, itemsWidth, itemsHeight, bgfx::copy(items.data(), items.size() * sizeof(ItemData)));
		bgfx::updateTexture2D(m_lightsListTex, 0, 0, 0, 0, lights.size() * texelsPerLight, 1, bgfx::copy(lights.data(), lights.size() * sizeof(LightData)));
	}
}

class bgfxCallbacks : public bgfx::CallbackI {
public:
	virtual ~bgfxCallbacks()
	{
	}

	virtual void fatal(const char* _filePath, uint16_t _line, bgfx::Fatal::Enum _code, const char* _str) override
	{
		//TODO
		LogError("%s", _str ? _str : "");
		//bgfx::trace(_filePath, _line, "BGFX 0x%08x: %s\n", _code, _str);

		if (bgfx::Fatal::DebugCheck == _code)
		{
			bx::debugBreak();
		}
		else
		{
			abort();
		}
	}

	virtual void traceVargs(const char* _filePath, uint16_t _line, const char* _format, va_list _argList) override
	{
		char temp[2048];
		char* out = temp;
		va_list argListCopy;
		va_copy(argListCopy, _argList);
		int32_t len = bx::snprintf(out, sizeof(temp), "%s (%d): ", _filePath, _line);
		int32_t total = len + bx::vsnprintf(out + len, sizeof(temp) - len, _format, argListCopy);
		va_end(argListCopy);
		if ((int32_t)sizeof(temp) < total)
		{
			out = (char*)alloca(total + 1);
			bx::memCopy(out, temp, len);
			bx::vsnprintf(out + len, total - len, _format, _argList);
		}
		out[total] = '\0';
		bx::debugOutput(out);
	}
	virtual void profilerBegin(const char* _name, uint32_t _abgr, const char* _filePath, uint16_t _line) override
	{
#ifdef USE_OPTICK
		Optick::Event::Push(_name);
#endif// USE_OPTICK
	}

	virtual void profilerBeginLiteral(const char* _name, uint32_t _abgr, const char* _filePath, uint16_t _line) override
	{
#ifdef USE_OPTICK
		Optick::Event::Push(_name);
#endif// USE_OPTICK
	}
	virtual void profilerThreadStart(const char* _name) override {
#ifdef USE_OPTICK
		Optick::RegisterThread(_name);
#endif// USE_OPTICK
	}
	virtual void profilerThreadQuit(const char* /*_name*/) override {
#ifdef USE_OPTICK
		Optick::UnRegisterThread(false);
#endif// USE_OPTICK
	}

	virtual void profilerEnd() override
	{
#ifdef USE_OPTICK
		Optick::Event::Pop();
#endif// USE_OPTICK
	}

	virtual uint32_t cacheReadSize(uint64_t /*_id*/) override
	{
		return 0;
	}

	virtual bool cacheRead(uint64_t /*_id*/, void* /*_data*/, uint32_t /*_size*/) override
	{
		return false;
	}

	virtual void cacheWrite(uint64_t /*_id*/, const void* /*_data*/, uint32_t /*_size*/) override
	{
	}

	virtual void screenShot(const char* _filePath, uint32_t _width, uint32_t _height, uint32_t _pitch, const void* _data, uint32_t _size, bool _yflip) override
	{
		BX_UNUSED(_filePath, _width, _height, _pitch, _data, _size, _yflip);

		const int32_t len = bx::strLen(_filePath) + 5;
		char* filePath = (char*)alloca(len);
		bx::strCopy(filePath, len, _filePath);
		bx::strCat(filePath, len, ".tga");

		bx::FileWriter writer;
		if (bx::open(&writer, filePath))
		{
			bimg::imageWriteTga(&writer, _width, _height, _pitch, _data, false, _yflip);
			bx::close(&writer);
		}
	}

	virtual void captureBegin(uint32_t /*_width*/, uint32_t /*_height*/, uint32_t /*_pitch*/, bgfx::TextureFormat::Enum /*_format*/, bool /*_yflip*/) override
	{
		BX_TRACE("Warning: using capture without callback (a.k.a. pointless).");
	}

	virtual void captureEnd() override
	{
	}

	virtual void captureFrame(const void* /*_data*/, uint32_t /*_size*/) override
	{
	}
} callbacksForBgfx;


bool Render::Init()
{
	OPTICK_EVENT();
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

	renderSettings = AssetDatabase::Get()->Load<RenderSettings>("settings.asset");
	ASSERT(renderSettings);
	s_renderSettings = renderSettings;//TODO kinda hacky

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
	SetFullScreen(CfgGetBool("fullscreen")); // TODO move to some RenderSettings class and load it as ordinary asset
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
	initInfo.type = renderSettings->backend;
	initInfo.callback = &callbacksForBgfx;
	//TODO preload renderdoc.dll from tools folder
#ifdef SE_HAS_DEBUG
	initInfo.limits.transientVbSize *= 10;
	initInfo.limits.transientIbSize *= 10;
	initInfo.debug = renderSettings->debug;
	initInfo.profile = renderSettings->debug;
#endif
	bgfx::init(initInfo);

	bgfx::setViewRect(kRenderPassGeometry, 0, 0, width, height);
	bgfx::setViewRect(1, 0, 0, width, height);

	u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
	u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
	s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
	s_texEmissive = bgfx::createUniform("s_texEmissive", bgfx::UniformType::Sampler);
	s_texMetalic = bgfx::createUniform("s_texMetalic", bgfx::UniformType::Sampler);
	s_texRoughness = bgfx::createUniform("s_texRoughness", bgfx::UniformType::Sampler);
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

	//TODO is it good idea to load something after database is unloaded ?
	databaseAfterUnloadedHandle = AssetDatabase::Get()->onAfterUnloaded.Subscribe([this]() { LoadAssets(); });
	databaseBeforeUnloadedHandle = AssetDatabase::Get()->onBeforeUnloaded.Subscribe([this]() { UnloadAssets(); });

	LoadAssets();

	Graphics::SetRenderPtr(this);

	return true;
}

void Render::LoadAssets() {
	auto* database = AssetDatabase::Get();
	if (database == nullptr) {
		return;;//HACK for terminated app
	}
	renderSettings = AssetDatabase::Get()->Load<RenderSettings>("settings.asset");
	ASSERT(renderSettings);
	s_renderSettings = renderSettings;//TODO kinda hacky

	whiteTexture = database->Load<Texture>("engine\\textures\\white.png");
	defaultNormalTexture = database->Load<Texture>("engine\\textures\\defaultNormal.png");
	defaultEmissiveTexture = database->Load<Texture>("engine\\textures\\defaultEmissive.png");
	simpleBlitMat = database->Load<Material>("engine\\materials\\simpleBlit.asset");
	brokenMat = database->Load<Material>("engine\\materials\\broken.asset");

	//TODO assert not null
}

void Render::UnloadAssets() {
	//TODO terminate whole database textures, shaders and other stuff on Render::Term()
	whiteTexture = nullptr;
	defaultNormalTexture = nullptr;
	defaultEmissiveTexture = nullptr;
	simpleBlitMat = nullptr;
	brokenMat = nullptr;
	renderSettings = nullptr;
	s_renderSettings = nullptr;//TODO kinda hacky
}

bool Render::IsDebugMode() {
	return s_renderSettings->debug;
}

void Render::Draw(SystemsManager& systems)
{
	OPTICK_EVENT();

	//TODO this should really be in editor class and not here, but 'window' var is hard to access
	std::string windowTitle = "Siberien";
	if (Editor::Get()->IsInEditMode()) {
		windowTitle += " Editor";
	}
	if (Editor::Get()->HasUnsavedFiles()) {
		windowTitle += " [NOT SAVED]";
	}
	if (prevWindowTitle != windowTitle) {
		prevWindowTitle = windowTitle;
		SDL_SetWindowTitle(window, windowTitle.c_str());
	}

	currentFreeViewId = kRenderPassFree;
	currentFullScreenTextureIdx = 0;

	//TODO not on draw!

	//TODO not here
	if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_RETURN) && (Input::GetKey(SDL_Scancode::SDL_SCANCODE_LALT) || Input::GetKey(SDL_Scancode::SDL_SCANCODE_RALT))) {
		SetFullScreen(!IsFullScreen());
	}
	auto bgfxDbg = BGFX_DEBUG_TEXT;
	if (renderSettings->bgfxDebugStats) {
		bgfxDbg |= BGFX_DEBUG_STATS;
	}
	bgfx::setDebug(bgfxDbg);

	int width;
	int height;
	SDL_GetWindowSize(window, &width, &height);

	auto bgfxResetFlags = BGFX_RESET_NONE;
	if (renderSettings->vsync) {
		bgfxResetFlags |= BGFX_RESET_VSYNC;
	}
	if (renderSettings->maxAnisotropy) {
		bgfxResetFlags |= BGFX_RESET_MAXANISOTROPY;
	}
	if (renderSettings->msaa > 0) {
		int msaaOffset = Mathf::Clamp(Mathf::Log2Floor(renderSettings->msaa), 0, 4);
		if (msaaOffset > 0) {
			bgfxResetFlags |= msaaOffset << BGFX_RESET_MSAA_SHIFT;
		}
	}

	if (height != prevHeight || width != prevWidth || !bgfx::isValid(m_fullScreenBuffer) || !bgfx::isValid(m_fullScreenBuffer2) || this->bgfxResetFlags != bgfxResetFlags) {
		prevWidth = width;
		prevHeight = height;
		this->bgfxResetFlags = bgfxResetFlags;
		bgfx::reset(width, height, bgfxResetFlags);
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

		uint64_t tsFlagsWithMsaa = tsFlags;
		if (renderSettings->msaa > 0) {
			int msaaOffset = Mathf::Clamp(Mathf::Log2Floor(renderSettings->msaa), 0, 4);
			tsFlagsWithMsaa = Bits::SetMaskFalse(tsFlagsWithMsaa, BGFX_TEXTURE_RT);
			tsFlagsWithMsaa |= uint64_t(msaaOffset + 1) << BGFX_TEXTURE_RT_MSAA_SHIFT;
		}


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

		gBuffer.albedoTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlagsWithMsaa);
		//gBuffer.normalTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		//gBuffer.lightTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		//gBuffer.emissiveTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, tsFlags);
		gBuffer.depthTexture = bgfx::createTexture2D(uint16_t(width), uint16_t(height), false, 1, bgfx::TextureFormat::D32F, tsFlagsWithMsaa | BGFX_TEXTURE_RT_WRITE_ONLY);
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

	Camera* shadowCamera = camera;
	if (dbg_debugShadows) {
		if (camera->gameObject()->tag == Camera::editorCameraTag) {
			for (auto& camera : Camera::GetAllCameras()) {
				if (camera->gameObject()->tag != Camera::editorCameraTag) {
					camera->OnBeforeRender();
					shadowCamera = camera;
					break;
				}
			}
		}
	}

	camera->OnBeforeRender();

	uint8_t clearAlbedo = 1;
	uint8_t clearOther = 2;
	bgfx::setPaletteColor(clearAlbedo, camera->GetClearColor().ToIntRGBA());
	bgfx::setPaletteColor(clearOther, Colors::black.ToIntRGBA());
	bgfx::setViewClear(kRenderPassGeometry, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 1.0f, 0, clearAlbedo, clearOther, clearOther, clearOther, clearOther, clearOther, clearOther, clearOther);

	Vector3 lightsPoi = camera->gameObject()->transform()->GetPosition() + camera->gameObject()->transform()->GetForward() * 15.f;

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

	if (CfgGetBool("showFps")) { // TODO move to some RenderSettings class and load it as ordinary asset
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


	//dbgMeshesDrawn = 0;
	//dbgMeshesCulled = 0;

	PrepareLights(*shadowCamera);

	systems.Draw();

	DrawAll(kRenderPassGeometry, *camera, nullptr);

	Dbg::DrawAll(kRenderPassGeometry);
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

	{
		OPTICK_EVENT("bgfx::frame");
		bgfx::frame();
	}
}

void Render::Term()
{
	OPTICK_EVENT();
	Graphics::SetRenderPtr(nullptr);
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
	bgfx::destroy(s_texMetalic);
	bgfx::destroy(s_texRoughness);
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
void Render::DrawAll(int viewId, const ICamera& camera, std::shared_ptr<Material> overrideMaterial, const std::vector<MeshRenderer*>* ptrRenderers) {
	//TODO setup camera
	OPTICK_EVENT();

	std::vector<MeshRenderer*> renderers = ptrRenderers != nullptr ? *ptrRenderers : MeshRenderer::enabledMeshRenderers;

	if (renderSettings->frustumCulling) {
		int size = renderers.size();
		for (int i = 0; i < size; i++) {
			if (!camera.IsVisible(*renderers[i])) {
				//dbgMeshesCulled++;
				size--;
				renderers[i] = renderers[size];
				i--;
			}
		}
		renderers.resize(size);
	}

	if (!overrideMaterial) {
		static auto renderersSort = [](const MeshRenderer* r1, const MeshRenderer* r2) {
			if (r1->mesh.get() < r2->mesh.get()) {
				return true;
			}
			else if (r1->mesh.get() > r2->mesh.get()) {
				return false;
			}
			else if (r1->material.get() > r2->material.get()) {
				return false;
			}
			else if (r1->material.get() < r2->material.get()) {
				return true;
			}
			else {
				return r1->randomColorTextureIdx < r2->randomColorTextureIdx;
			}
		};
		if (renderSettings->preSortRenderers) {
			OPTICK_EVENT("SortRenderers");
			std::sort(renderers.begin(), renderers.end(), renderersSort);
		}


		OPTICK_EVENT("DrawRenderers");
		bool prevEq = false;
		const MeshRenderer* meshNext;
		bool nextEq;
		//TODO get rid of std::vector / std::string
		if (renderSettings->tryToMinimizeStateChanges) {
			for (int i = 0; i < ((int)renderers.size() - 1); i++) {
				const auto mesh = renderers[i];
				meshNext = renderers[i + 1];
				nextEq = !renderersSort(mesh, meshNext) && !renderersSort(meshNext, mesh);
				DrawMesh(mesh, mesh->material.get(), camera, !nextEq, !nextEq, !prevEq, !prevEq, viewId);
				prevEq = nextEq;
			}
			//TODO do we really need this ?
			if (renderers.size() > 0) {
				const auto& mesh = renderers[renderers.size() - 1];
				DrawMesh(mesh, mesh->material.get(), camera, true, true, !prevEq, !prevEq, viewId);
			}
		}
		else {
			for (const auto renderer : renderers) {
				DrawMesh(renderer, renderer->material.get(), camera, true, true, true, true, viewId);
			}
		}
	}
	else {
		//TODO refactor duplicated code

		static auto renderersSort = [](const MeshRenderer* r1, const MeshRenderer* r2) {
			return r1->mesh.get() < r2->mesh.get();
		};
		if (renderSettings->preSortShadowRenderers) {
			OPTICK_EVENT("SortShadowRenderers");
			std::sort(renderers.begin(), renderers.end(), renderersSort);
		}

		OPTICK_EVENT("DrawRenderers");
		bool prevEq = false;
		bool materialApplied = false;//TODO dsplit drawMesh into bind material / bind mesh / submit / clear
		//TODO get rid of std::vector / std::string
		if (renderSettings->tryToMinimizeStateChanges) {
			if (renderers.size() > 0) {
				const auto& mesh = renderers[0];
				DrawMesh(mesh, overrideMaterial.get(), camera, renderers.size() == 1, renderers.size() == 1, true, true, viewId);
			}
			for (int i = 1; i < ((int)renderers.size() - 1); i++) {
				const auto mesh = renderers[i];
				const auto meshNext = renderers[i + 1];
				//TODO optimize
				bool nextEq = mesh == meshNext;
				DrawMesh(mesh, overrideMaterial.get(), camera, false, !nextEq, false, !prevEq, viewId);
				prevEq = nextEq;
				materialApplied = true;
			}
			if (renderers.size() > 1) {
				const auto& mesh = renderers[renderers.size() - 1];
				DrawMesh(mesh, overrideMaterial.get(), camera, true, true, false, !prevEq, viewId);
			}
		}
		else {
			for (const auto renderer : renderers) {
				DrawMesh(renderer, overrideMaterial.get(), camera, true, true, true, true, viewId);
			}
		}
	}
}

void Render::DrawMesh(const MeshRenderer* renderer, const Material* material, const ICamera& camera, bool clearMaterialState, bool clearMeshState, bool updateMaterialState, bool updateMeshState, int viewId) {
	if (!material || !material->shader || !bgfx::isValid(material->shader->program)) {
		material = brokenMat.get();//PERF checking and fixing cornercase
	}
	ASSERT(material && material->shader && bgfx::isValid(material->shader->program));

	const auto& matrix = renderer->m_transform->GetMatrix();

	if (renderer->bonesFinalMatrices.size() != 0) {
		bgfx::setTransform(&renderer->bonesFinalMatrices[0], renderer->bonesFinalMatrices.size());

		if (dbg_drawBones) {
			for (const auto& bone : renderer->bonesWorldMatrices) {
				Dbg::Draw(bone);
			}
		}
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
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_MSAA //TODO not in shadows
			;
		if (material->shader->isAlphaBlending) {
			state |= BGFX_STATE_BLEND_ALPHA;
		}
		else {
			//TODO separate shader var for this
			state |= BGFX_STATE_WRITE_Z;
		}
		bgfx::setState(state);
	}
	if (updateMeshState) {
		bgfx::setVertexBuffer(0, renderer->mesh->vertexBuffer);
		bgfx::setIndexBuffer(renderer->mesh->indexBuffer);
	}

	auto discardFlags = BGFX_DISCARD_NONE;
	if (clearMaterialState && clearMeshState) {
		discardFlags = BGFX_DISCARD_ALL;
	}
	else if (clearMeshState) {
		discardFlags = BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS;
	}
	else if (clearMaterialState) {
		discardFlags = BGFX_DISCARD_BINDINGS | BGFX_DISCARD_STATE;
	}
	Vector4 viewPos = (camera.GetViewMatrix() * matrix.GetColumn(3));
	uint32_t depth = viewPos.z * 1024.f;//TODO * farplane
	bgfx::submit(viewId, material->shader->program, depth, discardFlags);
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
	if (material->metalicTex) {
		bgfx::setTexture(3, s_texMetalic, material->metalicTex->handle);
	}
	else {
		if (defaultEmissiveTexture) {
			bgfx::setTexture(3, s_texMetalic, defaultEmissiveTexture->handle);
		}
	}
	if (material->roughnessTex) {
		bgfx::setTexture(4, s_texRoughness, material->roughnessTex->handle);
	}
	else {
		if (whiteTexture) {
			bgfx::setTexture(4, s_texRoughness, whiteTexture->handle);
		}
	}


	bgfx::setTexture(5, s_texClusterList, m_clusterListTex);
	bgfx::setTexture(6, s_texItemsList, m_itemsListTex);
	bgfx::setTexture(7, s_texLightsList, m_lightsListTex);

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
	static auto setVectorUniform = [](Render* render, const std::string& name, const Vector4& value) {
		auto it = render->vectorUniforms.find(name);
		bgfx::UniformHandle uniform;
		if (it == render->vectorUniforms.end()) {
			uniform = bgfx::createUniform(name.c_str(), bgfx::UniformType::Vec4);
			render->vectorUniforms[name] = uniform;
		}
		else {
			uniform = it->second;
		}
		bgfx::setUniform(uniform, &value);
	};
	setVectorUniform(this, "u_uvOffset", Vector4_zero);

	for (const auto& vecProp : material->vectors) {
		setVectorUniform(this, vecProp.name, vecProp.value);
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