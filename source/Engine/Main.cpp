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
//#include <bgfx_utils.h>


std::shared_ptr<Scene> LoadScene(AssetDatabase& assets) {
	auto scene = assets.LoadByPath<Scene>(CfgGetString("scene") + ".asset");
	if (scene) {
		scene->Init();
	}
	return scene;
}

int main(int argc, char* argv[]) {
	bool needConstantSceneReload = false;

	//TODO no sins here please
start:
	bool needReload = false;

	Config config;
	if (!config.Init()) {
		return -1;
	}

	Render render;
	if (!render.Init()) {
		return -1;
	}

	AssetDatabase assets;
	if (!assets.Init()) {
		return -1;
	}

	SystemsManager systemsManager;
	if (!systemsManager.Init()) {
		return -1;
	}

	//auto camera = assets.LoadByPath<Camera>("camera.asset");
	//Camera::SetMain(camera);
	auto scene = LoadScene(assets);

	//auto gameObject = assets.LoadByPath<GameObject>("gameObject.asset");

	Input::Init();

	bool quit = false;
	while (!quit) {
		Input::Update();

		if (scene) {
			scene->Update();
		}

		render.Draw();

		quit |= Input::GetQuit();
		if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_F5)) {
			quit = true;
			needReload = true;
		}
		if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_F6)) {
			needConstantSceneReload = !needConstantSceneReload;
		}
		if (needConstantSceneReload) {
			if (scene) {
				scene->Term();
			}
			assets.Term();
			assets = AssetDatabase();
			assets.Init();
			scene = LoadScene(assets);
		}
	}

	Input::Term();

	if (scene) {
		scene->Term();
	}

	render.Term();

	systemsManager.Term();

	assets.Term();

	config.Term();

	if (needReload) {
		goto start;
	}

	return 0;
}