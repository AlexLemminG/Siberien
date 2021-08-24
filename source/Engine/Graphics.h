#pragma once
#include "System.h"

class Render;
class Material;

class Graphics : public System<Graphics> {
	friend class Render;
public:
	bool Init();
	void Blit(std::shared_ptr<Material> material, int targetViewId = 1);
	void SetRenderPtr(Render* render);//TODO make private with friend
private:
	void SetScreenSpaceQuadBuffer();
	Render* render = nullptr;
};