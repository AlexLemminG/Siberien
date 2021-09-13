#pragma once

#include "SMath.h"
#include "Component.h"

class Light : public Component {
public:
	Color color = Colors::white;
	//TODO fix full everything is in shadow when drawShadows==false
	//TODO shadows for pointLight
	bool drawShadows = false;
	float shadowBias = 0.0012f;
	REFLECT_BEGIN(Light, Component);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_VAR(color);
	REFLECT_VAR(shadowBias);
	REFLECT_VAR(drawShadows);
	REFLECT_END();
};

class SE_CPP_API DirLight : public Light {
public:

	void OnEnable();
	void OnDisable();

	static std::vector<DirLight*> dirLights;

	REFLECT_BEGIN(DirLight, Light);
	REFLECT_END();
};

class SE_CPP_API PointLight : public Light {
public:
	void OnEnable();
	void OnDisable();

	static std::vector<PointLight*> pointLights;

	float radius = 5.f;
	float innerRadius = 0.f;

	REFLECT_BEGIN(PointLight, Light);
	REFLECT_VAR(radius);
	REFLECT_VAR(innerRadius);
	REFLECT_END();
};