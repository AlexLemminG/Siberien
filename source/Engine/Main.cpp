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
//#include <bgfx_utils.h>


YAML::Node config;

int main(int argc, char* argv[]) {

	config = YAML::LoadFile("config.yaml");

	Render render;

	if (!render.Init()) {
		return -1;
	}

	AssetDatabase assets;

	if (!assets.Init()) {
		return -1;
	}

	auto cameraAsset = assets.LoadByPath<TextAsset>("camera.asset");
	Camera c;
	c.Deserialize(SerializedObject(*cameraAsset));
	Camera::SetMain(&c);

	bool quit = false;
	while (!quit) {
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
		}
		render.Draw();
	}

	render.Term();

	assets.Term();


	return 0;
}