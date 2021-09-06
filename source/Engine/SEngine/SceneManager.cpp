

#include "SceneManager.h"
#include "Resources.h"
#include "Scene.h"
#include "Object.h"
#include "Config.h"
#include "ryml.hpp"

std::string SceneManager::lastLoadRequest;
std::shared_ptr<Scene> SceneManager::currentScene;
GameEvent<> SceneManager::onSceneLoaded;
void SceneManager::LoadScene(std::string sceneName) {
	lastLoadRequest = sceneName;
}

void SceneManager::Update() {
	OPTICK_EVENT();
	if (lastLoadRequest.size() == 0) {
		return;
	}

	if (currentScene) {
		currentScene->Term();
		currentScene = nullptr;
	}

	auto sceneName = lastLoadRequest;
	lastLoadRequest = "";

	auto assets = AssetDatabase::Get();

	std::shared_ptr<Scene> scene;
	{
		OPTICK_EVENT("Load scene");
		scene = assets->Load<Scene>(sceneName);
		if (!scene) {
			if (sceneName != "-") { //TODO tidy
				//ASSERT(false);
			}
			return;
		}
	}

	if (CfgGetBool("debugSceneLoading")) {
		ryml::Tree node;
		std::vector<std::shared_ptr<Object>> serializedObjects;
		serializedObjects.push_back(scene);
		SerializationContext context{ node, serializedObjects };
		context.database = AssetDatabase::Get();
		::Serialize(context, scene);
		context.FlushRequiestedToSerialize();

		std::ofstream fout("out.yaml");
		fout << context.GetYamlNode();
	}
	{
		OPTICK_EVENT("Instantiate Scene");
		currentScene = Object::Instantiate(scene);
		if (currentScene) {
			currentScene->name = sceneName;
			currentScene->Init();
			onSceneLoaded.Invoke();
		}
	}
}

void SceneManager::Term() {
	OPTICK_EVENT();
	LoadScene("-");
	Update();
}