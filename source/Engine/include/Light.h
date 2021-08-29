#pragma once

#include "SMath.h"
#include "Component.h"

class Light : public Component {
public:
	Color color = Colors::white;
	bool drawShadows = false;
};

class SE_CPP_API DirLight : public Light {
public:

	void OnEnable();
	void OnDisable();

	static std::vector<DirLight*> dirLights;

	REFLECT_BEGIN(DirLight);
	REFLECT_VAR(color);
	REFLECT_VAR(drawShadows);//TODO inheritance
	REFLECT_END();
};

class SE_CPP_API PointLight : public Light {
public:
	void OnEnable();
	void OnDisable();

	static std::vector<PointLight*> pointLights;

	float radius = 5.f;
	float innerRadius = 0.f;

	REFLECT_BEGIN(PointLight);
	REFLECT_VAR(color);
	REFLECT_VAR(radius);
	REFLECT_VAR(innerRadius);
	REFLECT_VAR(drawShadows);
	REFLECT_END();
};

DECLARE_TEXT_ASSET(DirLight);
DECLARE_TEXT_ASSET(PointLight);