#pragma once

#include "Component.h"

class ModelPreview : public Component {
	virtual void OnEnable() override;
	virtual void Update() override;
	virtual void OnDisable() override;

	REFLECT_BEGIN(ModelPreview);
	REFLECT_END();

private:
	void SelectPrefab(std::shared_ptr<GameObject> prefab);
	void UpdateSelection();
	void UpdateAnimator();

	std::shared_ptr<GameObject> currentPrefab;
	std::shared_ptr<GameObject> currentGameObject;
	std::vector<std::string> allGameObjects;
};