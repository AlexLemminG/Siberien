

#include <iostream>
#include "bgfx/bgfx.h"
#include "Common.h"

#include "Render.h"

#include "SDL.h"
#include "yaml-cpp/yaml.h"
#include <SDL_syswm.h>
#include <bgfx/platform.h>
#include "Resources.h"
#include "Serialization.h"
#include "Camera.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Input.h"
#include "Scene.h"
#include "System.h"
#include "SceneManager.h"
#include "Engine.h"
#include "Graphics.h"
#include "Config.h"
#include "imgui/imgui.h"
#include "GameLibrariesManager.h"
#include <dear-imgui/imgui_impl_sdl.h>

std::string GetFirstSceneName() {
	return CfgGetString("scene") + ".asset";
}

static Engine* engine = nullptr;
Engine* Engine::Get() {
	//TODO not here
	return engine;
}

class EngineLib : public GameLibrary {
	INNER_LIBRARY(EngineLib);
};
DEFINE_LIBRARY(EngineLib);


int main(int argc, char* argv[]) {
	//TODO no sins here please
start:
	bool needReload = false;

	Config config;
	AssetDatabase assets;
	Render render;
	SystemsManager systemsManager;
	Engine engine;
	::engine = &engine;
	GameLibrariesManager libs;

	{
		OPTICK_EVENT("Init");
		if (!config.Init()) {
			return -1;
		}

		if (!assets.Init()) {
			return -1;
		}

		if (!render.Init()) {
			return -1;
		}

		if (!libs.Init()) {
			return -1;
		}

		if (!systemsManager.Init()) {
			return -1;
		}


		Graphics::Get()->SetRenderPtr(&render); //TODO this is not the way

		Input::Init();

		SceneManager::Init();

		SceneManager::LoadScene(GetFirstSceneName());

		SceneManager::Update();
	}

	bool quit = false;
	bool needConstantSceneReload = false;
	bool needSceneReload = false;
	while (!quit) {
		OPTICK_FRAME("MainThread");

		Input::Update();

		//TODO move away
		ImGui_ImplSDL2_NewFrame();
		imguiBeginFrame();


		if (Scene::Get()) {
			Scene::Get()->Update();
		}
		systemsManager.Update();

		render.Draw(systemsManager);

		quit |= Input::GetQuit();
		if (CfgGetBool("godMode")) {
			if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_F5)) {
				quit = true;
				needReload = true;
			}
			if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_F6)) {
				if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_LSHIFT)) {
					needConstantSceneReload = !needConstantSceneReload;
				}
				else {
					needSceneReload = true;
				}
			}
		}

		if (needConstantSceneReload) {
			needSceneReload = true;
		}
		if (needSceneReload) {
			needSceneReload = false;
			SceneManager::LoadScene(GetFirstSceneName());
			//assets.UnloadAll();
		}

		SceneManager::Update();


		imguiEndFrame();
	}

	{
		OPTICK_EVENT("Term");
		Input::Term();

		SceneManager::Term();

		Graphics::Get()->SetRenderPtr(nullptr); //TODO this is not the way

		systemsManager.Term();

		assets.UnloadAll();

		libs.Term();

		render.Term();

		assets.Term();

		config.Term();

		::engine = nullptr;
	}
	if (needReload) {
		goto start;
	}

	return 0;
}