#pragma once
#include <memory>
#include <vector>
#include "bgfx/bgfx.h"

class Camera;
class Light;
class Render;
class ShadowSettings;

class ShadowRenderer {
public:
	ShadowRenderer();

	void Init();
	void Term();

	void Draw(Render* render, Light* light, const Camera& camera);

	void ApplyUniforms();
private:
	std::shared_ptr<ShadowSettings> settings;
	std::vector<bgfx::FrameBufferHandle> s_rtShadowMap;
	int m_currentShadowMapSize = 0;
	int lastRenderedFrame = -1;
	float shadowBias = 0.f;
};