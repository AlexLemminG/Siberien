#pragma once

#include "Component.h"

class GameObject : public Object {
	std::vector<std::shared_ptr<Component>> components;

	REFLECT_BEGIN(GameObject);
	//TODO default someting
	REFLECT_VAR(components, std::vector<std::shared_ptr<Component>>());
	REFLECT_END();
};