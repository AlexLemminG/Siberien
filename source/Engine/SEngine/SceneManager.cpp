

#include "SceneManager.h"
#include "Resources.h"
#include "Scene.h"
#include "Object.h"
#include "Config.h"
#include "ryml.hpp"

std::string SceneManager::lastLoadRequest;
std::shared_ptr<Scene> SceneManager::currentScene;
GameEvent<> SceneManager::onBeforeSceneEnabled;
static GameEvent<> onBeforeSceneDisabledEvent;
GameEvent<> SceneManager::onAfterSceneDisabled;
GameEvent<> SceneManager::onSceneLoaded;
bool lastLoadRequestIsForEditing = false;

void SceneManager::LoadScene(std::string sceneName) {
	lastLoadRequest = sceneName;
	lastLoadRequestIsForEditing = false;
}

void SceneManager::LoadSceneForEditing(std::string sceneName) {
	lastLoadRequest = sceneName;
	lastLoadRequestIsForEditing = true;
}

void SceneManager::Update() {
	OPTICK_EVENT();
	if (lastLoadRequest.size() == 0) {
		return;
	}

	if (currentScene) {
		onBeforeSceneDisabledEvent.Invoke();
		currentScene->Term();
		onAfterSceneDisabled.Invoke();

		if (currentScene->IsInEditMode()) {
			//HACK to reload prefab yaml nodes
			auto path = AssetDatabase::Get()->GetAssetPath(currentScene);
			if (!path.empty()) {
				AssetDatabase::Get()->Unload(path);
			}
		}
		currentScene = nullptr;
	}

	auto sceneName = lastLoadRequest;
	lastLoadRequest = "";

	auto assets = AssetDatabase::Get();

	std::shared_ptr<Scene> scene;
	{
		OPTICK_EVENT("Load scene");
		if (sceneName != "-") { //TODO tidy
			scene = assets->Load<Scene>(sceneName);
			if (!scene) {
				LogError("Failed to load scene '%s'", sceneName.c_str());
				ASSERT(false);
			}
		}
		else {
			scene = nullptr;
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
		if (lastLoadRequestIsForEditing) {
			currentScene = scene;
		}
		else {
			currentScene = Object::Instantiate(scene);
		}
		if (currentScene) {
			currentScene->name = sceneName;
			onBeforeSceneEnabled.Invoke();
			currentScene->Init(lastLoadRequestIsForEditing);
			onSceneLoaded.Invoke();
		}
	}
}

void SceneManager::Term() {
	OPTICK_EVENT();
	LoadScene("-");
	Update();
	onSceneLoaded.UnsubscribeAll();
}

std::shared_ptr<Scene> SceneManager::GetCurrentScene() { return currentScene; }

std::string SceneManager::GetCurrentScenePath() {

	return currentScene != nullptr ? currentScene->name : "-";
}

GameEvent<>& SceneManager::onBeforeSceneDisabled() { return onBeforeSceneDisabledEvent; }
