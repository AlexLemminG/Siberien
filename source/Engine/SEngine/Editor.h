#pragma once

#include <memory>
#include "System.h"
#include "GameEvents.h"

class Object;

class Editor : public System<Editor> {
public:
	static void SetDirty(std::shared_ptr<Object> dirtyObj);


	void Update() override;

	bool IsInEditMode() const;

	std::shared_ptr<Object> selectedObject;

	GameEvent<std::shared_ptr<GameObject>&> onGameObjectEdited; //called each time gameObject or it's components are changed from editor
private:
	bool autoSaveEveryFrame = true;

	std::vector<std::shared_ptr<Object>> dirtyObjs;

	void SaveAllDirty();
};