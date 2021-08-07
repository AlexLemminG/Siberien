#pragma once

#include "Component.h"
#include "Math.h"

class Transform : public Component {
	//TODO private
	//TODO custom serializer from pos,eulers to matrix
public:
	Matrix4 matrix = Matrix4::Identity();

	REFLECT_BEGIN(Transform);
	REFLECT_END();
};