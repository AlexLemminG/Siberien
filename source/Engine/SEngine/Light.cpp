

#include "Light.h"

std::vector<PointLight*> PointLight::pointLights;

void PointLight::OnEnable() {
	pointLights.push_back(this);
}

void PointLight::OnDisable() {
	pointLights.erase(std::find(pointLights.begin(), pointLights.end(), this));
}


std::vector<DirLight*> DirLight::dirLights;

void DirLight::OnEnable() {
	dirLights.push_back(this);
}

void DirLight::OnDisable() {
	dirLights.erase(std::find(dirLights.begin(), dirLights.end(), this));
}

std::vector<SpotLight*> SpotLight::spotLights;

void SpotLight::OnEnable() {
	spotLights.push_back(this);
}

void SpotLight::OnDisable() {
	spotLights.erase(std::find(spotLights.begin(), spotLights.end(), this));
}


DECLARE_TEXT_ASSET(DirLight);
DECLARE_TEXT_ASSET(PointLight);
DECLARE_TEXT_ASSET(SpotLight);
