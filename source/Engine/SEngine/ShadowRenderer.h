#pragma once
#include <memory>
#include <vector>
#include "bgfx/bgfx.h"

class ICamera;
class Light;

class ShadowRenderer {
public:
	ShadowRenderer();

	void Init();
	void Term();

	void Draw(Light* light, const ICamera& camera);

	void ApplyUniforms();
private:
	std::vector<bgfx::FrameBufferHandle> s_rtShadowMap;
	int m_currentShadowMapSize = 0;
	int lastRenderedFrame = -1;
	float shadowBias = 0.f;
};