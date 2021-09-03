#pragma once

#include "Object.h"
#include "SMath.h"//TODO move color to Color.h ?

class SphericalHarmonics : public Object {
public:
	std::vector<Color> coeffs;
	REFLECT_BEGIN(SphericalHarmonics);
	REFLECT_VAR(coeffs);
	REFLECT_END();
};