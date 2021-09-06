

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
