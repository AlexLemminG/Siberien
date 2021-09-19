#pragma once

#include <string>
#include <memory>
#include "Defines.h"
#include "GameEvents.h"

class Scene;

class SE_CPP_API SceneManager {
public:
	static void LoadScene(std::string sceneName);

	static bool Init() { return true; }
	static void Update();
	static void Term();

	static std::shared_ptr<Scene> GetCurrentScene();
	static std::string GetCurrentScenePath();

	static GameEvent<> onSceneLoaded;
	static GameEvent<> onBeforeSceneEnabled;
	static GameEvent<> onAfterSceneDisabled;
private:
	//TODO less static stuff
	static std::string lastLoadRequest;
	static std::shared_ptr<Scene> currentScene;
};