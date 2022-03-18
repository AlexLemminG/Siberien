#pragma once
#include "System.h"
#include "SMath.h"

class Render;
class Material;

class SE_CPP_API Graphics : public System<Graphics> {
	friend class Render;
	friend class ShadowRenderer;
public:
	bool Init();
	void Blit(std::shared_ptr<Material> material); // blits from current screen texture to next
	void Blit(std::shared_ptr<Material> material, int targetViewId);

	int GetScreenWidth() const;
	int GetScreenHeight() const;
	Vector2Int GetScreenSize() const;
private:
	void SetScreenSpaceQuadBuffer();
	static void SetRenderPtr(Render* render);
};