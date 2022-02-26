#pragma once

#include <memory>
#include "System.h"

class Object;

class Editor : public System<Editor> {
public:
	static void SetDirty(std::shared_ptr<Object> dirtyObj);


	void Update() override;

	bool IsInEditMode() const;

	std::shared_ptr<Object> selectedObject;
private:
	bool autoSaveEveryFrame = true;

	std::vector<std::shared_ptr<Object>> dirtyObjs;

	void SaveAllDirty();
};