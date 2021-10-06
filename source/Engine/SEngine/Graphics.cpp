#include "Graphics.h"
#include "Material.h"
#include "Shader.h"
#include "bgfx/bgfx.h"
#include "Render.h"

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

REGISTER_SYSTEM(Graphics);

bool Graphics::Init() {
	PosTexCoord0Vertex::init();//TODO make local
	return true;
}

void Graphics::Blit(std::shared_ptr<Material> material) {
	if (!material || !material->shader) {
		return;
	}

	render->ApplyMaterialProperties(material.get());

	auto s_tex = render->GetTexColorSampler();

	auto texture = render->GetCurrentFullScreenTexture();
	bgfx::setTexture(0, s_tex, texture);

	bgfx::setState(0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
	);

	SetScreenSpaceQuadBuffer();

	bgfx::submit(render->GetNextFullScreenTextureViewId(), material->shader->program);
	render->FlipFullScreenTextures();
}

void Graphics::Blit(std::shared_ptr<Material> material, int targetViewId) {
	if (!material || !material->shader) {
		return;
	}

	render->ApplyMaterialProperties(material.get());

	auto s_tex = render->GetTexColorSampler();

	auto texture = render->GetCurrentFullScreenTexture();
	bgfx::setTexture(0, s_tex, texture);

	bgfx::setState(0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
	);

	SetScreenSpaceQuadBuffer();

	bgfx::submit(targetViewId, material->shader->program);
}


void Graphics::SetRenderPtr(Render* render) { this->render = render; }

int Graphics::GetScreenWidth() const { return render->GetWidth(); }

int Graphics::GetScreenHeight() const { return render->GetHeight(); }

Vector2Int Graphics::GetScreenSize() const { return Vector2Int(GetScreenWidth(), GetScreenHeight()); }

void Graphics::SetScreenSpaceQuadBuffer() {
	float textureWidth = render->GetWidth();
	float textureHeight = render->GetHeight();

	const bgfx::RendererType::Enum renderer = bgfx::getRendererType();
	auto texelHalf = bgfx::RendererType::Direct3D9 == renderer ? 0.5f : 0.0f;

	auto caps = bgfx::getCaps();

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

		const float texelHalfW = texelHalf / textureWidth;
		const float texelHalfH = texelHalf / textureHeight;
		const float minu = 0.0f + texelHalfW;
		const float maxu = 1.0f + texelHalfH;

		const float zz = 0.0f;

		float minv = 1.0f + texelHalfH;
		float maxv = 0.0f + texelHalfH;

		if (caps->originBottomLeft)
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