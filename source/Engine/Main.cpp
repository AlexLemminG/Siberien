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
//#include <bgfx_utils.h>



int main(int argc, char* argv[]) {
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

	//auto camera = assets.LoadByPath<Camera>("camera.asset");
	//Camera::SetMain(camera);

	auto scene = assets.LoadByPath<Scene>("scene.asset");

	scene->Init();

	//auto gameObject = assets.LoadByPath<GameObject>("gameObject.asset");

	Input::Init();

	bool quit = false;
	while (!quit) {
		Input::Update();
		
		scene->Update();
		
		render.Draw();

		quit |= Input::GetQuit();
		if (Input::GetKeyDown(SDL_Scancode::SDL_SCANCODE_F5)) {
			quit = true;
			needReload = true;
		}
	}

	Input::Term();

	scene->Term();

	render.Term();

	assets.Term();

	config.Term();

	if (needReload) {
		goto start;
	}

	return 0;
}