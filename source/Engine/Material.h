#pragma once

#include "Object.h"
#include "Math.h"//TODO move color to Color

class Shader;
class Texture;

class Material : public Object {
public:
	Color color = Colors::white;
	std::shared_ptr<Shader> shader;
	std::shared_ptr<Texture> colorTex;
	std::shared_ptr<Texture> normalTex;
	std::shared_ptr<Texture> emissiveTex;
	std::vector<std::shared_ptr<Texture>> randomColorTextures;

	REFLECT_BEGIN(Material);
	REFLECT_VAR(colorTex);
	REFLECT_VAR(normalTex);
	REFLECT_VAR(emissiveTex);
	REFLECT_VAR(color);
	REFLECT_VAR(shader);
	REFLECT_VAR(randomColorTextures);
	REFLECT_END();
};
