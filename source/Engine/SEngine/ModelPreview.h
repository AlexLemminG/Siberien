#pragma once

#include "Component.h"
#include "SMath.h"

class ModelPreview : public Component {
public:
	virtual void OnEnable() override;
	virtual void Update() override;
	virtual void OnDisable() override;

	REFLECT_BEGIN(ModelPreview);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_END();

private:
	void SelectPrefab(std::shared_ptr<GameObject> prefab);
	void UpdateSelection();
	void UpdateAnimator();
	void UpdateCamera();

	bool wasMouseDown = false;
	Vector2 mouseDownPos;
	Vector2 mouseDownRotation;
	Vector2 currentRotation{ 180.f, 0.f };

	std::shared_ptr<GameObject> currentPrefab;
	std::shared_ptr<GameObject> currentGameObject;
	std::vector<std::string> allGameObjects;
};