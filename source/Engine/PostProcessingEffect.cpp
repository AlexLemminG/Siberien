#include "PostProcessingEffect.h"
#include "bgfx/bgfx.h"
#include "Render.h"
#include "Texture.h"
#include "System.h"
#include "Shader.h"


struct PosTexCoord0Vertex
{
	float m_x;
	float m_y;
	float m_z;
	float m_u;
	float m_v;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	}

	static bgfx::VertexLayout ms_layout;
};
bgfx::VertexLayout PosTexCoord0Vertex::ms_layout;

class PostProcessingSystem : public System< PostProcessingSystem> {
	bool Init() {
		PosTexCoord0Vertex::init();//TODO make local
		return true;
	}
};
REGISTER_SYSTEM(PostProcessingSystem);

void PostProcessingEffect::Draw(Render& render) {

	if (!shader || !bgfx::isValid(shader->program)) {
		return;
	}

	int width = render.GetWidth();
	int height = render.GetHeight();

	const bgfx::RendererType::Enum renderer = bgfx::getRendererType();
	auto texelHalf = bgfx::RendererType::Direct3D9 == renderer ? 0.5f : 0.0f;

	auto caps = bgfx::getCaps();

	const float pixelSize[4] =
	{
		1.0f / width,
		1.0f / height,
		0.0f,
		0.0f,
	};
	auto u_pixelSize = render.GetPixelSizeUniform();
	bgfx::setUniform(u_pixelSize, pixelSize);
	auto s_tex = render.GetTexColorSampler();
	bgfx::setTexture(0, s_tex, bgfx::getTexture(render.GetFullScreenBuffer()));

	auto win = AssetDatabase::Get()->LoadByPath<Texture>("textures\\win.png");
	if (win && bgfx::isValid(win->handle)) {
		auto s_tex2 = render.GetEmissiveColorSampler();
		bgfx::setTexture(2, s_tex2, win->handle);
	}

	bgfx::setState(0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
	);
	Vector4 params;
	params[0] = intensity;
	params[1] = intensityFromLastHit;
	params[2] = winScreenFade;

	bgfx::setUniform(render.GetPlayerHealthParamsUniform(), &params.x);

	ScreenSpaceQuad((float)width, (float)height, texelHalf, caps->originBottomLeft);
	bgfx::submit(1, shader->program);
}

std::vector<std::shared_ptr<PostProcessingEffect>> PostProcessingEffect::activeEffects; //TODO no static please
void PostProcessingEffect::ScreenSpaceQuad(
	float _textureWidth,
	float _textureHeight,
	float _texelHalf,
	bool _originBottomLeft
) {
	float _width = 1.0f;
	float _height = 1.0f;

	if (4 == bgfx::getAvailTransientVertexBuffer(4, PosTexCoord0Vertex::ms_layout) && 6 == bgfx::getAvailTransientIndexBuffer(6))
	{
		bgfx::TransientVertexBuffer vb;
		bgfx::allocTransientVertexBuffer(&vb, 4, PosTexCoord0Vertex::ms_layout);
		bgfx::TransientIndexBuffer ib;
		bgfx::allocTransientIndexBuffer(&ib, 6);


		PosTexCoord0Vertex* vertex = (PosTexCoord0Vertex*)vb.data;

		const float minx = -_width / 2.f;
		const float maxx = _width / 2.f;
		const float miny = -_height / 2.f;
		const float maxy = _height / 2.f;

		const float texelHalfW = _texelHalf / _textureWidth;
		const float texelHalfH = _texelHalf / _textureHeight;
		const float minu = 0.0f + texelHalfW;
		const float maxu = 1.0f + texelHalfH;

		const float zz = 0.0f;

		float minv = texelHalfH;
		float maxv = 1.0f + texelHalfH;

		if (_originBottomLeft)
		{
			float temp = minv;
			minv = maxv;
			maxv = temp;

			minv -= 1.0f;
			maxv -= 1.0f;
		}

		vertex[0].m_x = minx;
		vertex[0].m_y = miny;
		vertex[0].m_z = zz;
		vertex[0].m_u = minu;
		vertex[0].m_v = minv;

		vertex[1].m_x = maxx;
		vertex[1].m_y = miny;
		vertex[1].m_z = zz;
		vertex[1].m_u = maxu;
		vertex[1].m_v = minv;

		vertex[2].m_x = maxx;
		vertex[2].m_y = maxy;
		vertex[2].m_z = zz;
		vertex[2].m_u = maxu;
		vertex[2].m_v = maxv;

		vertex[3].m_x = minx;
		vertex[3].m_y = maxy;
		vertex[3].m_z = zz;
		vertex[3].m_u = minu;
		vertex[3].m_v = maxv;

		auto index = (uint16_t*)ib.data;
		index[0] = 1;
		index[1] = 3;
		index[2] = 2;

		index[3] = 1;
		index[4] = 0;
		index[5] = 3;

		bgfx::setVertexBuffer(0, &vb);
		bgfx::setIndexBuffer(&ib);
	}
}
DECLARE_TEXT_ASSET(PostProcessingEffect);