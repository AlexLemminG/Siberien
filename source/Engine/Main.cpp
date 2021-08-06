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

	auto camera = assets.LoadByPath<Camera>("camera.asset");
	Camera::SetMain(camera);

	auto gameObject = assets.LoadByPath<GameObject>("gameObject.asset");

	bool quit = false;
	while (!quit) {
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.scancode == SDL_SCANCODE_F5) {
					needReload = true;
					quit = true;
				}
			}
		}
		render.Draw();
	}

	render.Term();

	assets.Term();

	config.Term();

	if (needReload) {
		goto start;
	}

	return 0;
}