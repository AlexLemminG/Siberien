#include "Editor.h"
#include "Resources.h"
#include <set>
#include <iostream>
#include "ryml.hpp"
#include "Scene.h"
#include "SceneManager.h"
#include "DbgVars.h"
#include "Input.h"

DBG_VAR_BOOL(dbg_isEditMode, "EditMode", false)
REGISTER_SYSTEM(Editor);

void Editor::SetDirty(std::shared_ptr<Object> dirtyObj)
{
	Get()->dirtyObjs.push_back(dirtyObj);
}


void Editor::Update() {
	if (Input::GetKeyDown(SDL_SCANCODE_F4)) {
		dbg_isEditMode = !dbg_isEditMode;
	}
	auto scene = Scene::Get();
	if (dbg_isEditMode && scene && !scene->IsInEditMode()) {
		SceneManager::LoadSceneForEditing(SceneManager::GetCurrentScenePath());
	}
	else if (!dbg_isEditMode && scene && scene->IsInEditMode()) {
		SceneManager::LoadScene(SceneManager::GetCurrentScenePath());
	}

	if (autoSaveEveryFrame) {
		SaveAllDirty();
	}
}

//TODO what is this function even mean?
bool Editor::IsInEditMode() const {
	auto scene = Scene::Get();
	if (!scene) {
		return false;
	}
	else {
		return scene->IsInEditMode();
	}
}


void Editor::SaveAllDirty() {
	std::set<std::string> filesToSave;
	for (const auto& obj : dirtyObjs) {
		auto path = AssetDatabase::Get()->GetAssetPath(obj);

		if (path.empty()) {
			//TODO error
			continue;
		}
		if (AssetDatabase::Get()->GetOriginalSerializedAsset(path) == nullptr) {
			//TODO error
			continue;
		}

		filesToSave.insert(path);
	}
	for (const auto& path : filesToSave) {
		//TODO no hardcode
		//TODO do it through asset database
		std::ofstream output("assets\\" + path);
		if (!output.is_open()) {
			//TODO error
			continue;
		}
		auto tree = AssetDatabase::Get()->GetOriginalSerializedAsset(path);
		output << tree->rootref();
	}
	dirtyObjs.clear();
}


