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

std::string GetFirstSceneName() {
	return CfgGetString("scene") + ".asset";
}


std::shared_ptr<Scene> LoadScene(AssetDatabase& assets, std::string sceneName) {
	auto scene = assets.LoadByPath<Scene>(sceneName);
	scene = Object::Instantiate(scene);
	if (scene) {
		scene->Init();
		auto creep = scene->FindGameObjectByTag("Creep");
		if (creep) {
			static int h = 1000;
			for (int i = 0; i < h; i++) {
				auto go = Object::Instantiate(creep);
				float r = 10.f;
				SetPos(go->transform()->matrix, GetPos(go->transform()->matrix) + Vector3{ Random::Range(-r, r), 5.f, Random::Range(-r, r) });
				if (go) {
					scene->AddGameObject(go);
				}
			}
		}
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
	auto scene = LoadScene(assets, GetFirstSceneName());

	//auto gameObject = assets.LoadByPath<GameObject>("gameObject.asset");

	Input::Init();

	bool quit = false;
	while (!quit) {
		Input::Update();

		if (scene) {
			scene->Update();
		}
		systemsManager.Update();

		render.Draw(systemsManager);

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
			scene = LoadScene(assets, GetFirstSceneName());;
		}
	}

	//vv.Init();

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