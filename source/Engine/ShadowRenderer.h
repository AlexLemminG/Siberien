#pragma once
#include <memory>

class ICamera;
class Light;

class ShadowRenderer {
public:
	ShadowRenderer();

	void Draw(Light* light, const ICamera& camera);
};