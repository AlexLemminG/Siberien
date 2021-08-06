#pragma once

#include "Object.h"
#include "Serialization.h"

class Component : public Object {
public:
	virtual void OnEnable() {}
	virtual void OnDisable() {}
};