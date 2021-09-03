#pragma once

#include "Object.h"
#include "SMath.h"//TODO move color to Color

class Shader;
class Texture;

class MaterialProperty_Color {
public:
	std::string name;
	Color value;
	REFLECT_BEGIN(MaterialProperty_Color);
	REFLECT_VAR(name);
	REFLECT_VAR(value);
	REFLECT_END();
};
class MaterialProperty_Texture {
public:
	std::string name;
	std::shared_ptr<Texture> value;
	REFLECT_BEGIN(MaterialProperty_Texture);
	REFLECT_VAR(name);
	REFLECT_VAR(value);
	REFLECT_END();
};
class MaterialProperty_Vector {
public:
	std::string name;
	Vector4 value;
	REFLECT_BEGIN(MaterialProperty_Vector);
	REFLECT_VAR(name);
	REFLECT_VAR(value);
	REFLECT_END();
};

class Material : public Object {
public:
	//TODO make private
	std::vector<MaterialProperty_Vector> vectors;
	std::vector<MaterialProperty_Color> colors;
	std::vector<MaterialProperty_Texture> textures;

	Color color = Colors::white;
	std::shared_ptr<Shader> shader;
	std::shared_ptr<Texture> colorTex;
	std::shared_ptr<Texture> normalTex;
	std::shared_ptr<Texture> emissiveTex;
	std::vector<std::shared_ptr<Texture>> randomColorTextures;

	REFLECT_BEGIN(Material);
	REFLECT_VAR(vectors);
	REFLECT_VAR(colors);
	REFLECT_VAR(textures);
	REFLECT_VAR(colorTex);
	REFLECT_VAR(normalTex);
	REFLECT_VAR(emissiveTex);
	REFLECT_VAR(color);
	REFLECT_VAR(shader);
	REFLECT_VAR(randomColorTextures);
	REFLECT_END();
};
