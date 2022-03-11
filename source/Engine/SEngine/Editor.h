#pragma once

#include <memory>
#include <set>
#include "System.h"
#include "GameEvents.h"

class Object;

class Editor : public System<Editor> {
public:
	static void SetDirty(std::shared_ptr<Object> dirtyObj);


	bool Init() override;
	void Update() override;
	void Term() override;

	bool IsInEditMode() const;
	bool HasUnsavedFiles() const;

	std::shared_ptr<Object> selectedObject;

	GameEvent<std::shared_ptr<GameObject>&> onGameObjectEdited; //called each time gameObject or it's components are changed from editor
private:
	bool autoSaveOnTerm = true;
	bool autoSaveEveryFrame = false;

	std::set<std::shared_ptr<Object>> dirtyObjs;

	GameEventHandle onBeforeDatabaseUnloadedHandle;

	void SaveAllDirty();
};