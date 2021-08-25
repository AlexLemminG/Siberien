#pragma once

#include "Object.h"
#include "Reflect.h"
#include "bgfx/bgfx.h" //TODO forward declare instead

namespace bimg {
	class ImageContainer;
}

class Texture : public Object {
public:
	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
	~Texture();
	REFLECT_BEGIN(Texture);
	REFLECT_END();
};