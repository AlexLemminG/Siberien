#pragma once

#include <string>
#include <memory>
#include "Defines.h"

class Scene;

class SE_CPP_API SceneManager {
public:
	static void LoadScene(std::string sceneName);

	static bool Init() { return true; }
	static void Update();
	static void Term();

	static std::shared_ptr<Scene> GetCurrentScene() { return currentScene; }

	static std::string lastLoadRequest;
	static std::shared_ptr<Scene> currentScene;
};