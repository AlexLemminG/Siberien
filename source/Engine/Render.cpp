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

SDL_Window* Render::window = nullptr;

bool Render::Init()
{
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
	window = SDL_CreateWindow("SDL Tutorial", posX, posY, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (window == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return false;
	}
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
	bgfx::setDebug(BGFX_DEBUG_TEXT /*| BGFX_DEBUG_STATS*/);

	bgfx::setViewRect(0, 0, 0, width, height);

	InitVertexLayouts();

	u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);

	prevWidth = width;
	prevHeight = height;

	Dbg::Init();

	return true;
}
void Render::Draw(SystemsManager& systems)
{
	Matrix4 cameraViewMatrix;
	auto camera = Camera::GetMain();
	int width;
	int height;
	float fov = 60.f;
	float nearPlane = 0.1f;
	float farPlane = 100.f;
	SDL_GetWindowSize(window, &width, &height);

	if (height != prevHeight || width != prevWidth) {
		bgfx::reset(width, height, BGFX_RESET_VSYNC);
	}
	bgfx::setViewRect(0, 0, 0, width, height);

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

		near
	}
	float time = Time::time();
	bgfx::setUniform(u_time, &time);

	static float prevTimeCheck = 0;
	static int prevFramesCount = 0;
	static float fps = 0;
	if ((int)time != (int)prevTimeCheck) {
		fps = (Time::frameCount() - prevFramesCount) / (time - prevTimeCheck);
		prevTimeCheck = time;
		prevFramesCount = Time::frameCount();
	}

	bgfx::touch(0);
	bgfx::dbgTextClear();

	bgfx::dbgTextPrintf(1, 2, 0x0f, "FPS: %.1f", fps);

	if (camera == nullptr) {
		bgfx::dbgTextPrintf(1, 1, 0x0f, "NO CAMERA");
	}
	else {
		bgfx::dbgTextPrintf(1, 1, 0x0f, "YES CAMERA");
	}

	{
		float proj[16];
		bx::mtxProj(proj, fov, float(width) / float(height), nearPlane, farPlane, bgfx::getCaps()->homogeneousDepth);
		bgfx::setViewTransform(0, &cameraViewMatrix(0, 0), proj);

	}

	for (auto mesh : MeshRenderer::enabledMeshRenderers) {
		DrawMesh(mesh);
	}

	systems.Draw();

	Dbg::DrawAll();

	bgfx::frame();
}

void Render::Term()
{
	Dbg::Term();
	//TODO destroy programs, buffers and other shit
	bgfx::destroy(u_time);

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
	//float* ff = &(renderer->worldMatrix[0][0]);
	bgfx::setTransform(&(renderer->gameObject()->transform()->matrix));

	bgfx::setVertexBuffer(0, renderer->mesh->vertexBuffer);
	bgfx::setIndexBuffer(renderer->mesh->indexBuffer);

	bgfx::setState(state);

	bgfx::submit(0, renderer->material->shader->program);
}
