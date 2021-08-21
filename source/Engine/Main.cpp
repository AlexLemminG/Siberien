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
//#include <bgfx_utils.h>

std::string GetFirstSceneName() {
	return CfgGetString("scene") + ".asset";
}



int main(int argc, char* argv[]) {
	//TODO no sins here please
start:
	bool needReload = false;

	Config config;
	if (!config.Init()) {
		return -1;
	}

	AssetDatabase assets;
	if (!assets.Init()) {
		return -1;
	}

	Render render;
	if (!render.Init()) {
		return -1;
	}

	SystemsManager systemsManager;
	if (!systemsManager.Init()) {
		return -1;
	}

	Input::Init();

	SceneManager::Init();
	SceneManager::LoadScene(GetFirstSceneName());

	SceneManager::Update();

	bool quit = false;
	bool needConstantSceneReload = false;
	while (!quit) {
		OPTICK_FRAME("MainThread");

		Input::Update();

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
				needConstantSceneReload = !needConstantSceneReload;
			}
		}

		if (needConstantSceneReload) {
			SceneManager::LoadScene(GetFirstSceneName());
			assets.UnloadAll();
		}

		SceneManager::Update();
	}

	Input::Term();

	SceneManager::Term();

	assets.UnloadAll();

	systemsManager.Term();

	render.Term();

	assets.Term();

	config.Term();

	if (needReload) {
		goto start;
	}

	return 0;
}