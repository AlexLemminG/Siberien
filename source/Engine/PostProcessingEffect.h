#pragma once

#include "Component.h"
#include "Reflect.h"
#include "RenderEvents.h"

class Render;
class Shader;
class Material;

class PostProcessingEffect : public Component {
public:
	void OnEnable() override;
	void OnDisable() override;
	void Draw(Render& render);

	float winScreenFade = 0.f;
	float intensityFromLastHit;
	float intensity;
private:
	GameEventHandle handle;

	std::shared_ptr<Material> material;

	REFLECT_BEGIN(PostProcessingEffect);
	REFLECT_VAR(material);
	REFLECT_END();
};