#pragma once

#include "SMath.h"
#include "Component.h"

class Light : public Component {
public:
	Color color = Colors::white;
	float intensity = 1.0f;
	//TODO shadows for pointLight
	bool drawShadows = false;
	float shadowBias = 0.0012f;
	REFLECT_BEGIN(Light, Component);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_VAR(color);
	REFLECT_VAR(intensity);
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

	float radius = 5.f;//TODO bool to calc radius automagicaly(true by default)
	float innerRadius = 0.f;

	REFLECT_BEGIN(PointLight, Light);
	REFLECT_VAR(radius);
	REFLECT_VAR(innerRadius);
	REFLECT_END();
};

class SE_CPP_API SpotLight : public Light {
public:
	void OnEnable();
	void OnDisable();

	static std::vector<SpotLight*> spotLights;

	float radius = 5.f;
	float innerRadius = 0.f;
	float innerAngle = 15.f;
	float outerAngle = 30.f;

	REFLECT_BEGIN(SpotLight, Light);
	REFLECT_VAR(radius);
	REFLECT_VAR(innerRadius);
	REFLECT_VAR(innerAngle);
	REFLECT_VAR(outerAngle);
	REFLECT_END();
};