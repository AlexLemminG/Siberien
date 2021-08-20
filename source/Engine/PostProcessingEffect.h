#pragma once

#include "Object.h"
#include "Reflect.h"

class Render;
class Shader;

class PostProcessingEffect : public Object {
public:
	void Draw(Render& render);

	float winScreenFade = 0.f;
	float intensityFromLastHit;
	float intensity;
private:
	std::shared_ptr<Shader> shader;

	REFLECT_BEGIN(PostProcessingEffect);
	REFLECT_VAR(shader);
	REFLECT_END();

	static std::vector<std::shared_ptr<PostProcessingEffect>> activeEffects; //TODO no static please
	void ScreenSpaceQuad(
		float _textureWidth,
		float _textureHeight,
		float _texelHalf,
		bool _originBottomLeft);
};