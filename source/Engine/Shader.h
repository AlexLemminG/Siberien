#pragma once

#include "Object.h"
#include "bgfx/bgfx.h"//TODO forward declare
#include "Reflect.h"

class BinaryAsset;

class Shader : public Object {
public:
	bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
	~Shader();
	std::vector<std::shared_ptr<BinaryAsset>> buffers;
	REFLECT_BEGIN(Shader);
	REFLECT_END();
	std::string name;
};
