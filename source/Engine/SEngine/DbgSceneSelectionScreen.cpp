#include "Dbg.h"
#include "DbgVars.h"
#include "System.h"
#include "Resources.h"
#include "dear-imgui/imgui.h"
#include "dear-imgui/imgui_internal.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Input.h"

DBG_VAR_TRIGGER(dbg_showLevelSelectionScreen, "select scene");


class DbgSceneSelectionScreen : public System<DbgSceneSelectionScreen> {
public:
	virtual bool Init()override {
		return true;
	}
	virtual void Update()override {
		bool openWindow = dbg_showLevelSelectionScreen;
		if (Input::GetKeyDown(SDL_SCANCODE_F3)) {
			if (screenShown) {
				screenShown = false;
			}
			else {
				openWindow = true;
			}
		}

		if (openWindow && !screenShown) {
			screenShown = true;

			sceneAssets.clear();
			//TODO
			auto allAssets = AssetDatabase::Get()->GetAllAssetNames();
			//TODO get rid of std::vector
			for (const auto& asset : allAssets) {

				if (asset.find(".asset") == -1) {
					continue;
				}
				auto scene = AssetDatabase::Get()->Load<Scene>(asset);
				if (!scene) {
					continue;
				}
				sceneAssets.push_back(asset);
			}
		}

		if (screenShown && ImGui::Begin("Scene selection", &screenShown)) {
			if (selectedScene.empty()) {
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}
			if (ImGui::Button("Load") && !selectedScene.empty()) {
				SceneManager::LoadScene(selectedScene);
			}
			if (selectedScene.empty()) {
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}

			for each (auto & scene in sceneAssets) {
				if (ImGui::Selectable(scene.c_str(), scene == selectedScene)) {
					if (selectedScene == scene) {
						SceneManager::LoadScene(selectedScene);
					}
					else {
						selectedScene = scene;
					}
				}
			}
			ImGui::End();
		}

	}
	virtual void Term()override {
	}
private:
	bool screenShown = false;
	std::vector<std::string> sceneAssets;
	std::string selectedScene;
};
REGISTER_SYSTEM(DbgSceneSelectionScreen);
