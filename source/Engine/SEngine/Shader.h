#pragma once

#include "Object.h"
#include "bgfx/bgfx.h"//TODO forward declare
#include "Reflect.h"


class PixelShader : public Object {
public:
	bgfx::ShaderHandle handle = BGFX_INVALID_HANDLE;
};
class VertexShader : public Object {
public:
	bgfx::ShaderHandle handle = BGFX_INVALID_HANDLE;
};

//TODO ShaderVariant class for single vs+fs+defines combination
//TODO Shader is collection of variants + GetProgram(defines) 

class Shader : public Object {
public:
	bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
	~Shader();

	std::shared_ptr<PixelShader> fs;
	std::shared_ptr<PixelShader> vs;
	std::vector<std::string> includedFiles;
	REFLECT_BEGIN(Shader);
	REFLECT_VAR(fs);
	REFLECT_VAR(vs);
	REFLECT_VAR(isAlphaBlending);
	REFLECT_END();
	std::string name;
	bool isAlphaBlending = false;

	virtual void OnAfterDeserializeCallback(const SerializationContext& context);
};
