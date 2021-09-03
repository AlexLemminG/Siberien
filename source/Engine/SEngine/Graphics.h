#pragma once
#include "System.h"

class Render;
class Material;

class SE_CPP_API Graphics : public System<Graphics> {
	friend class Render;
	friend class ShadowRenderer;
public:
	bool Init();
	void Blit(std::shared_ptr<Material> material, int targetViewId = 1);
	void SetRenderPtr(Render* render);//TODO make private with friend

	int GetScreenWidth() const;
	int GetScreenHeight() const;
private:
	void SetScreenSpaceQuadBuffer();
	Render* render = nullptr;
};