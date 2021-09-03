#include "ShadowRenderer.h"
#include "GameObject.h"
#include "Transform.h"
#include "Graphics.h"
#include "Camera.h"
#include "Light.h"
#include "Render.h"
#include "Material.h"
#include "Resources.h"
#include "bgfx/bgfx.h"
#include "bx/bx.h"
#include "bx/math.h"

ShadowRenderer::ShadowRenderer() {}

void worldSpaceFrustumCorners(
	float* _corners24f
	, float _near
	, float _far
	, float _projWidth
	, float _projHeight
	, const float* _invViewMtx
)
{
	// Define frustum corners in view space.
	const float nw = _near * _projWidth;
	const float nh = _near * _projHeight;
	const float fw = _far * _projWidth;
	const float fh = _far * _projHeight;

	const uint8_t numCorners = 8;
	const bx::Vec3 corners[numCorners] =
	{
		{ -nw,  nh, _near },
		{  nw,  nh, _near },
		{  nw, -nh, _near },
		{ -nw, -nh, _near },
		{ -fw,  fh, _far  },
		{  fw,  fh, _far  },
		{  fw, -fh, _far  },
		{ -fw, -fh, _far  },
	};

	// Convert them to world space.
	float(*out)[3] = (float(*)[3])_corners24f;
	for (uint8_t ii = 0; ii < numCorners; ++ii)
	{
		bx::store(&out[ii], bx::mul(corners[ii], _invViewMtx));
	}
}

/**
 * _splits = { near0, far0, near1, far1... nearN, farN }
 * N = _numSplits
 */
void splitFrustum(float* _splits, uint8_t _numSplits, float _near, float _far, float _splitWeight = 0.75f)
{
	const float l = _splitWeight;
	const float ratio = _far / _near;
	const int8_t numSlices = _numSplits * 2;
	const float numSlicesf = float(numSlices);

	// First slice.
	_splits[0] = _near;

	for (uint8_t nn = 2, ff = 1; nn < numSlices; nn += 2, ff += 2)
	{
		float si = float(int8_t(ff)) / numSlicesf;

		const float nearp = l * (_near * bx::pow(ratio, si)) + (1 - l) * (_near + (_far - _near) * si);
		_splits[nn] = nearp;          //near
		_splits[ff] = nearp * 1.005f; //far from previous split
	}

	// Last slice.
	_splits[numSlices - 1] = _far;
}
void mtxYawPitchRoll(float* _result
	, float _yaw
	, float _pitch
	, float _roll
)
{
	float sroll = bx::sin(_roll);
	float croll = bx::cos(_roll);
	float spitch = bx::sin(_pitch);
	float cpitch = bx::cos(_pitch);
	float syaw = bx::sin(_yaw);
	float cyaw = bx::cos(_yaw);

	_result[0] = sroll * spitch * syaw + croll * cyaw;
	_result[1] = sroll * cpitch;
	_result[2] = sroll * spitch * cyaw - croll * syaw;
	_result[3] = 0.0f;
	_result[4] = croll * spitch * syaw - sroll * cyaw;
	_result[5] = croll * cpitch;
	_result[6] = croll * spitch * cyaw + sroll * syaw;
	_result[7] = 0.0f;
	_result[8] = cpitch * syaw;
	_result[9] = -spitch;
	_result[10] = cpitch * cyaw;
	_result[11] = 0.0f;
	_result[12] = 0.0f;
	_result[13] = 0.0f;
	_result[14] = 0.0f;
	_result[15] = 1.0f;
}

enum ShadowMapRenderTargets : uint8_t {
	_1,
	_2,
	_3,
	_4,
	Count
};
struct TetrahedronFaces
{
	enum Enum
	{
		Green,
		Yellow,
		Blue,
		Red,

		Count
	};
};
struct ProjType
{
	enum Enum
	{
		Horizontal,
		Vertical,

		Count
	};
};

struct DepthImpl
{
	enum Enum
	{
		InvZ,
		Linear,

		Count
	};
};


struct Uniforms
{
	void init()
	{
		m_ambientPass = 1.0f;
		m_lightingPass = 1.0f;

		m_shadowMapBias = 0.003f;
		m_shadowMapOffset = 0.0f;
		m_shadowMapParam0 = 0.5;
		m_shadowMapParam1 = 1.0;
		m_depthValuePow = 1.0f;
		m_showSmCoverage = 1.0f;
		m_shadowMapTexelSize = 1.0f / 512.0f;

		m_csmFarDistances[0] = 30.0f;
		m_csmFarDistances[1] = 90.0f;
		m_csmFarDistances[2] = 180.0f;
		m_csmFarDistances[3] = 1000.0f;

		m_tetraNormalGreen[0] = 0.0f;
		m_tetraNormalGreen[1] = -0.57735026f;
		m_tetraNormalGreen[2] = 0.81649661f;

		m_tetraNormalYellow[0] = 0.0f;
		m_tetraNormalYellow[1] = -0.57735026f;
		m_tetraNormalYellow[2] = -0.81649661f;

		m_tetraNormalBlue[0] = -0.81649661f;
		m_tetraNormalBlue[1] = 0.57735026f;
		m_tetraNormalBlue[2] = 0.0f;

		m_tetraNormalRed[0] = 0.81649661f;
		m_tetraNormalRed[1] = 0.57735026f;
		m_tetraNormalRed[2] = 0.0f;

		m_XNum = 2.0f;
		m_YNum = 2.0f;
		m_XOffset = 10.0f / 512.0f;
		m_YOffset = 10.0f / 512.0f;

		u_params0 = bgfx::createUniform("u_params0", bgfx::UniformType::Vec4);
		u_params1 = bgfx::createUniform("u_params1", bgfx::UniformType::Vec4);
		u_params2 = bgfx::createUniform("u_params2", bgfx::UniformType::Vec4);
		u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
		u_smSamplingParams = bgfx::createUniform("u_smSamplingParams", bgfx::UniformType::Vec4);
		u_csmFarDistances = bgfx::createUniform("u_csmFarDistances", bgfx::UniformType::Vec4);
		u_lightMtx = bgfx::createUniform("u_lightMtx", bgfx::UniformType::Mat4);

		u_tetraNormalGreen = bgfx::createUniform("u_tetraNormalGreen", bgfx::UniformType::Vec4);
		u_tetraNormalYellow = bgfx::createUniform("u_tetraNormalYellow", bgfx::UniformType::Vec4);
		u_tetraNormalBlue = bgfx::createUniform("u_tetraNormalBlue", bgfx::UniformType::Vec4);
		u_tetraNormalRed = bgfx::createUniform("u_tetraNormalRed", bgfx::UniformType::Vec4);

		u_shadowMapMtx0 = bgfx::createUniform("u_shadowMapMtx0", bgfx::UniformType::Mat4);
		u_shadowMapMtx1 = bgfx::createUniform("u_shadowMapMtx1", bgfx::UniformType::Mat4);
		u_shadowMapMtx2 = bgfx::createUniform("u_shadowMapMtx2", bgfx::UniformType::Mat4);
		u_shadowMapMtx3 = bgfx::createUniform("u_shadowMapMtx3", bgfx::UniformType::Mat4);

		u_lightPosition = bgfx::createUniform("u_lightPosition", bgfx::UniformType::Vec4);
		u_lightAmbientPower = bgfx::createUniform("u_lightAmbientPower", bgfx::UniformType::Vec4);
		u_lightDiffusePower = bgfx::createUniform("u_lightDiffusePower", bgfx::UniformType::Vec4);
		u_lightSpecularPower = bgfx::createUniform("u_lightSpecularPower", bgfx::UniformType::Vec4);
		u_lightSpotDirectionInner = bgfx::createUniform("u_lightSpotDirectionInner", bgfx::UniformType::Vec4);
		u_lightAttenuationSpotOuter = bgfx::createUniform("u_lightAttenuationSpotOuter", bgfx::UniformType::Vec4);

		u_materialKa = bgfx::createUniform("u_materialKa", bgfx::UniformType::Vec4);
		u_materialKd = bgfx::createUniform("u_materialKd", bgfx::UniformType::Vec4);
		u_materialKs = bgfx::createUniform("u_materialKs", bgfx::UniformType::Vec4);

	}

	void setPtrs(Material* _materialPtr, Light* _lightPtr, float* _colorPtr, float* _lightMtxPtr, float* _shadowMapMtx0, float* _shadowMapMtx1, float* _shadowMapMtx2, float* _shadowMapMtx3)
	{
		m_lightMtxPtr = _lightMtxPtr;
		m_colorPtr = _colorPtr;
		m_materialPtr = _materialPtr;
		m_lightPtr = _lightPtr;

		m_shadowMapMtx0 = _shadowMapMtx0;
		m_shadowMapMtx1 = _shadowMapMtx1;
		m_shadowMapMtx2 = _shadowMapMtx2;
		m_shadowMapMtx3 = _shadowMapMtx3;
	}

	// Call this once at initialization.
	void submitConstUniforms()
	{
		bgfx::setUniform(u_tetraNormalGreen, m_tetraNormalGreen);
		bgfx::setUniform(u_tetraNormalYellow, m_tetraNormalYellow);
		bgfx::setUniform(u_tetraNormalBlue, m_tetraNormalBlue);
		bgfx::setUniform(u_tetraNormalRed, m_tetraNormalRed);
	}

	// Call this once per frame.
	void submitPerFrameUniforms()
	{
		bgfx::setUniform(u_params1, m_params1);
		bgfx::setUniform(u_params2, m_params2);
		bgfx::setUniform(u_smSamplingParams, m_paramsBlur);
		bgfx::setUniform(u_csmFarDistances, m_csmFarDistances);

		//bgfx::setUniform(u_materialKa, &m_materialPtr->m_ka);
		//bgfx::setUniform(u_materialKd, &m_materialPtr->m_kd);
		//bgfx::setUniform(u_materialKs, &m_materialPtr->m_ks);

		//bgfx::setUniform(u_lightPosition, &m_lightPtr->m_position_viewSpace);
		//bgfx::setUniform(u_lightAmbientPower, &m_lightPtr->m_ambientPower);
		//bgfx::setUniform(u_lightDiffusePower, &m_lightPtr->m_diffusePower);
		//bgfx::setUniform(u_lightSpecularPower, &m_lightPtr->m_specularPower);
		//bgfx::setUniform(u_lightSpotDirectionInner, &m_lightPtr->m_spotDirectionInner_viewSpace);
		//bgfx::setUniform(u_lightAttenuationSpotOuter, &m_lightPtr->m_attenuationSpotOuter);
	}

	// Call this before each draw call.
	void submitPerDrawUniforms()
	{
		bgfx::setUniform(u_shadowMapMtx0, m_shadowMapMtx0);
		bgfx::setUniform(u_shadowMapMtx1, m_shadowMapMtx1);
		bgfx::setUniform(u_shadowMapMtx2, m_shadowMapMtx2);
		bgfx::setUniform(u_shadowMapMtx3, m_shadowMapMtx3);

		bgfx::setUniform(u_params0, m_params0);
		bgfx::setUniform(u_lightMtx, m_lightMtxPtr);
		bgfx::setUniform(u_color, m_colorPtr);
	}

	void destroy()
	{
		bgfx::destroy(u_params0);
		bgfx::destroy(u_params1);
		bgfx::destroy(u_params2);
		bgfx::destroy(u_color);
		bgfx::destroy(u_smSamplingParams);
		bgfx::destroy(u_csmFarDistances);

		bgfx::destroy(u_materialKa);
		bgfx::destroy(u_materialKd);
		bgfx::destroy(u_materialKs);

		bgfx::destroy(u_tetraNormalGreen);
		bgfx::destroy(u_tetraNormalYellow);
		bgfx::destroy(u_tetraNormalBlue);
		bgfx::destroy(u_tetraNormalRed);

		bgfx::destroy(u_shadowMapMtx0);
		bgfx::destroy(u_shadowMapMtx1);
		bgfx::destroy(u_shadowMapMtx2);
		bgfx::destroy(u_shadowMapMtx3);

		bgfx::destroy(u_lightMtx);
		bgfx::destroy(u_lightPosition);
		bgfx::destroy(u_lightAmbientPower);
		bgfx::destroy(u_lightDiffusePower);
		bgfx::destroy(u_lightSpecularPower);
		bgfx::destroy(u_lightSpotDirectionInner);
		bgfx::destroy(u_lightAttenuationSpotOuter);
	}

	union
	{
		struct
		{
			float m_ambientPass;
			float m_lightingPass;
			float m_unused00;
			float m_unused01;
		};

		float m_params0[4];
	};

	union
	{
		struct
		{
			float m_shadowMapBias;
			float m_shadowMapOffset;
			float m_shadowMapParam0;
			float m_shadowMapParam1;
		};

		float m_params1[4];
	};

	union
	{
		struct
		{
			float m_depthValuePow;
			float m_showSmCoverage;
			float m_shadowMapTexelSize;
			float m_unused23;
		};

		float m_params2[4];
	};

	union
	{
		struct
		{
			float m_XNum;
			float m_YNum;
			float m_XOffset;
			float m_YOffset;
		};

		float m_paramsBlur[4];
	};

	float m_tetraNormalGreen[3];
	float m_tetraNormalYellow[3];
	float m_tetraNormalBlue[3];
	float m_tetraNormalRed[3];
	float m_csmFarDistances[4];

	float* m_lightMtxPtr;
	float* m_colorPtr;
	Light* m_lightPtr;
	float* m_shadowMapMtx0;
	float* m_shadowMapMtx1;
	float* m_shadowMapMtx2;
	float* m_shadowMapMtx3;
	Material* m_materialPtr;

private:
	bgfx::UniformHandle u_params0;
	bgfx::UniformHandle u_params1;
	bgfx::UniformHandle u_params2;
	bgfx::UniformHandle u_color;
	bgfx::UniformHandle u_smSamplingParams;
	bgfx::UniformHandle u_csmFarDistances;

	bgfx::UniformHandle u_materialKa;
	bgfx::UniformHandle u_materialKd;
	bgfx::UniformHandle u_materialKs;

	bgfx::UniformHandle u_tetraNormalGreen;
	bgfx::UniformHandle u_tetraNormalYellow;
	bgfx::UniformHandle u_tetraNormalBlue;
	bgfx::UniformHandle u_tetraNormalRed;

	bgfx::UniformHandle u_shadowMapMtx0;
	bgfx::UniformHandle u_shadowMapMtx1;
	bgfx::UniformHandle u_shadowMapMtx2;
	bgfx::UniformHandle u_shadowMapMtx3;

	bgfx::UniformHandle u_lightMtx;
	bgfx::UniformHandle u_lightPosition;
	bgfx::UniformHandle u_lightAmbientPower;
	bgfx::UniformHandle u_lightDiffusePower;
	bgfx::UniformHandle u_lightSpecularPower;
	bgfx::UniformHandle u_lightSpotDirectionInner;
	bgfx::UniformHandle u_lightAttenuationSpotOuter;
};
static Uniforms s_uniforms;


void ShadowRenderer::Init() {
	s_rtShadowMap.push_back(BGFX_INVALID_HANDLE);
	s_rtShadowMap.push_back(BGFX_INVALID_HANDLE);
	s_rtShadowMap.push_back(BGFX_INVALID_HANDLE);
	s_rtShadowMap.push_back(BGFX_INVALID_HANDLE);
}

void ShadowRenderer::Term() {

	for (uint8_t ii = 0; ii < ShadowMapRenderTargets::Count; ++ii)
	{
		if (bgfx::isValid(s_rtShadowMap[ii])) {
			bgfx::destroy(s_rtShadowMap[ii]);
		}
	}
}

void ShadowRenderer::Draw(Light* light, const ICamera& camera)
{
	if (!light->drawShadows) {
		return;
	}
	//TODO
	int lightShadowSize = 512;
	static bool m_stencilPack = false;
	static float m_fovXAdjust = 1.0f;
	static float m_fovYAdjust = 1.0f;

	static float m_near = 0.1;//TODO params
	static float m_far = 22.f;//TODO params
	static DepthImpl::Enum m_depthImpl = DepthImpl::Linear;
	static int m_numSplits = 1;
	static float m_splitDistribution = 0.6;
	static bool m_stabilize = true;
	static uint32_t clearRgba = 0;
	static float clearDepth = 1.f;
	static uint8_t clearStencil = 0;
	static bool s_flipV = false;

	float m_shadowMapMtx[ShadowMapRenderTargets::Count][16];


	static int RENDERVIEW_SHADOWMAP_0_ID = 5;
	static int RENDERVIEW_SHADOWMAP_1_ID = 6;
	static int RENDERVIEW_SHADOWMAP_2_ID = 7;
	static int RENDERVIEW_SHADOWMAP_3_ID = 8;
	static int RENDERVIEW_SHADOWMAP_4_ID = 9;

	static int RENDERVIEW_FIRST = 5;
	static int RENDERVIEW_LAST = 9;

	// Set view and projection matrices.
	const float camFovy = Camera::GetMain()->GetFov();
	int m_width = Graphics::Get()->GetScreenWidth();
	int m_height = Graphics::Get()->GetScreenHeight();
	const float camAspect = float(int32_t(m_width)) / float(int32_t(m_height));
	const float camNear = 0.1f;
	const float camFar = 200.0f;
	const float projHeight = bx::tan(bx::toRad(camFovy) * 0.5f);
	const float projWidth = projHeight * camAspect;

	std::shared_ptr<Material> shadowCasterMaterial = AssetDatabase::Get()->LoadByPath<Material>("materials\\shadowCaster.asset");
	if (!shadowCasterMaterial) {
		return;
	}

	Vector3 lightPosition = light->gameObject()->transform()->GetPosition();

	//TODO cleaner
	bool isDirectional = dynamic_cast<DirLight*>(light) != nullptr;
	bool isPoint = dynamic_cast<PointLight*>(light) != nullptr;
	bool isSpot = false;

	const uint8_t shadowMapPasses = (uint8_t)ShadowMapRenderTargets::Count;
	float lightView[shadowMapPasses][16];
	float lightProj[shadowMapPasses][16];
	float mtxYpr[TetrahedronFaces::Count][16];

	Matrix4 screenProj;
	Matrix4 screenView;
	auto caps = bgfx::getCaps();

	screenView = Matrix4::Identity();

	bx::mtxOrtho(
		&screenProj(0, 0)
		, 0.0f
		, 1.0f
		, 1.0f
		, 0.0f
		, 0.0f
		, 100.0f
		, 0.0f
		, caps->homogeneousDepth
	);

	// Update render target size.
	uint16_t shadowMapSize = lightShadowSize;
	if (m_currentShadowMapSize != shadowMapSize)
	{
		m_currentShadowMapSize = shadowMapSize;
		s_uniforms.m_shadowMapTexelSize = 1.0f / m_currentShadowMapSize;

		{
			if (bgfx::isValid(s_rtShadowMap[0])) {
				bgfx::destroy(s_rtShadowMap[0]);
			}

			bgfx::TextureHandle fbtextures[] =
			{
				bgfx::createTexture2D(m_currentShadowMapSize, m_currentShadowMapSize, false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT),
				bgfx::createTexture2D(m_currentShadowMapSize, m_currentShadowMapSize, false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT),
			};
			s_rtShadowMap[0] = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
		}

		if (isDirectional)
		{
			for (uint8_t ii = 1; ii < ShadowMapRenderTargets::Count; ++ii)
			{
				{
					if (bgfx::isValid(s_rtShadowMap[ii])) {
						bgfx::destroy(s_rtShadowMap[ii]);
					}

					bgfx::TextureHandle fbtextures[] =
					{
						bgfx::createTexture2D(m_currentShadowMapSize, m_currentShadowMapSize, false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT),//TODO why not just depth?
						bgfx::createTexture2D(m_currentShadowMapSize, m_currentShadowMapSize, false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT),
					};
					s_rtShadowMap[ii] = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
				}
			}
		}
	}

	if (isSpot)
	{
		//TODO
		//const float fovy = m_settings.m_coverageSpotL;
		//const float aspect = 1.0f;
		//bx::mtxProj(
		//	lightProj[ProjType::Horizontal]
		//	, fovy
		//	, aspect
		//	, currentSmSettings->m_near
		//	, currentSmSettings->m_far
		//	, false
		//);

		////For linear depth, prevent depth division by variable w-component in shaders and divide here by far plane
		//if (DepthImpl::Linear == m_settings.m_depthImpl)
		//{
		//	lightProj[ProjType::Horizontal][10] /= currentSmSettings->m_far;
		//	lightProj[ProjType::Horizontal][14] /= currentSmSettings->m_far;
		//}

		//const bx::Vec3 at = bx::add(bx::load<bx::Vec3>(m_pointLight.m_position.m_v), bx::load<bx::Vec3>(m_pointLight.m_spotDirectionInner.m_v));
		//bx::mtxLookAt(lightView[TetrahedronFaces::Green], bx::load<bx::Vec3>(m_pointLight.m_position.m_v), at);
	}
	else if (isPoint)
	{
		float ypr[TetrahedronFaces::Count][3] =
		{
			{ bx::toRad(0.0f), bx::toRad(27.36780516f), bx::toRad(0.0f) },
			{ bx::toRad(180.0f), bx::toRad(27.36780516f), bx::toRad(0.0f) },
			{ bx::toRad(-90.0f), bx::toRad(-27.36780516f), bx::toRad(0.0f) },
			{ bx::toRad(90.0f), bx::toRad(-27.36780516f), bx::toRad(0.0f) },
		};


		if (m_stencilPack)
		{
			const float fovx = 143.98570868f + 3.51f + m_fovXAdjust;
			const float fovy = 125.26438968f + 9.85f + m_fovYAdjust;
			const float aspect = bx::tan(bx::toRad(fovx * 0.5f)) / bx::tan(bx::toRad(fovy * 0.5f));

			bx::mtxProj(
				lightProj[ProjType::Vertical]
				, fovx
				, aspect
				, m_near
				, m_far
				, false
			);

			//For linear depth, prevent depth division by variable w-component in shaders and divide here by far plane
			if (DepthImpl::Linear == m_depthImpl)
			{
				lightProj[ProjType::Vertical][10] /= m_far;
				lightProj[ProjType::Vertical][14] /= m_far;
			}

			ypr[TetrahedronFaces::Green][2] = bx::toRad(180.0f);
			ypr[TetrahedronFaces::Yellow][2] = bx::toRad(0.0f);
			ypr[TetrahedronFaces::Blue][2] = bx::toRad(90.0f);
			ypr[TetrahedronFaces::Red][2] = bx::toRad(-90.0f);
		}

		const float fovx = 143.98570868f + 7.8f + m_fovXAdjust;
		const float fovy = 125.26438968f + 3.0f + m_fovYAdjust;
		const float aspect = bx::tan(bx::toRad(fovx * 0.5f)) / bx::tan(bx::toRad(fovy * 0.5f));

		bx::mtxProj(
			lightProj[ProjType::Horizontal]
			, fovy
			, aspect
			, m_near
			, m_far
			, caps->homogeneousDepth
		);

		//For linear depth, prevent depth division by variable w component in shaders and divide here by far plane
		if (DepthImpl::Linear == m_depthImpl)
		{
			lightProj[ProjType::Horizontal][10] /= m_far;
			lightProj[ProjType::Horizontal][14] /= m_far;
		}


		for (uint8_t ii = 0; ii < TetrahedronFaces::Count; ++ii)
		{
			float mtxTmp[16];
			mtxYawPitchRoll(mtxTmp, ypr[ii][0], ypr[ii][1], ypr[ii][2]);

			float tmp[3] =
			{
				-bx::dot(bx::load<bx::Vec3>(&lightPosition.x), bx::load<bx::Vec3>(&mtxTmp[0])),
				-bx::dot(bx::load<bx::Vec3>(&lightPosition.x), bx::load<bx::Vec3>(&mtxTmp[4])),
				-bx::dot(bx::load<bx::Vec3>(&lightPosition.x), bx::load<bx::Vec3>(&mtxTmp[8])),
			};

			bx::mtxTranspose(mtxYpr[ii], mtxTmp);

			bx::memCopy(lightView[ii], mtxYpr[ii], 12 * sizeof(float));
			lightView[ii][12] = tmp[0];
			lightView[ii][13] = tmp[1];
			lightView[ii][14] = tmp[2];
			lightView[ii][15] = 1.0f;
		}
	}
	else // LightType::DirectionalLight == settings.m_lightType
	{
		// Setup light view mtx.

		auto viewMatrix = light->gameObject()->transform()->matrix;
		SetScale(viewMatrix, Vector3_one);
		viewMatrix = viewMatrix.Inverse();
		memcpy(&(lightView[0][0]), &viewMatrix(0, 0), 16 * sizeof(float));

		memcpy(&(lightView[1][0]), &viewMatrix(0, 0), 16 * sizeof(float));
		memcpy(&(lightView[2][0]), &viewMatrix(0, 0), 16 * sizeof(float));
		memcpy(&(lightView[3][0]), &viewMatrix(0, 0), 16 * sizeof(float));

		//bx::mtxLookAt(lightView[0], eye, at);

		// Compute camera inverse view mtx.
		float mtxViewInv[16];
		bx::mtxInverse(mtxViewInv, &camera.GetViewMatrix()(0, 0));

		// Compute split distances.
		const uint8_t maxNumSplits = 4;
		BX_ASSERT(maxNumSplits >= settings.m_numSplits, "Error! Max num splits.");

		float splitSlices[maxNumSplits * 2];
		splitFrustum(splitSlices
			, uint8_t(m_numSplits)
			, m_near
			, m_far
			, m_splitDistribution
		);

		// Update uniforms.
		for (uint8_t ii = 0, ff = 1; ii < m_numSplits; ++ii, ff += 2)
		{
			// This lags for 1 frame, but it's not a problem.
			s_uniforms.m_csmFarDistances[ii] = splitSlices[ff];
		}

		float mtxProj[16];
		bx::mtxOrtho(
			mtxProj
			, 1.0f
			, -1.0f
			, 1.0f
			, -1.0f
			, -1024.f//TODO params
			, 1024.f//TODO params
			, 0.0f
			, caps->homogeneousDepth
		);

		const uint8_t numCorners = 8;
		float frustumCorners[maxNumSplits][numCorners][3];
		for (uint8_t ii = 0, nn = 0, ff = 1; ii < m_numSplits; ++ii, nn += 2, ff += 2)
		{
			// Compute frustum corners for one split in world space.
			worldSpaceFrustumCorners((float*)frustumCorners[ii], splitSlices[nn], splitSlices[ff], projWidth, projHeight, mtxViewInv);

			bx::Vec3 min = { 9000.0f,  9000.0f,  9000.0f };
			bx::Vec3 max = { -9000.0f, -9000.0f, -9000.0f };

			for (uint8_t jj = 0; jj < numCorners; ++jj)
			{
				// Transform to light space.
				const bx::Vec3 xyz = bx::mul(bx::load<bx::Vec3>(frustumCorners[ii][jj]), lightView[0]);

				// Update bounding box.
				min = bx::min(min, xyz);
				max = bx::max(max, xyz);
			}

			const bx::Vec3 minproj = bx::mulH(min, mtxProj);
			const bx::Vec3 maxproj = bx::mulH(max, mtxProj);

			float scalex = 2.0f / (maxproj.x - minproj.x);
			float scaley = 2.0f / (maxproj.y - minproj.y);

			if (m_stabilize)
			{
				const float quantizer = 64.0f;
				scalex = quantizer / bx::ceil(quantizer / scalex);
				scaley = quantizer / bx::ceil(quantizer / scaley);
			}

			float offsetx = 0.5f * (maxproj.x + minproj.x) * scalex;
			float offsety = 0.5f * (maxproj.y + minproj.y) * scaley;

			if (m_stabilize)
			{
				const float halfSize = m_currentShadowMapSize * 0.5f;
				offsetx = bx::ceil(offsetx * halfSize) / halfSize;
				offsety = bx::ceil(offsety * halfSize) / halfSize;
			}

			float mtxCrop[16];
			bx::mtxIdentity(mtxCrop);
			mtxCrop[0] = scalex;
			mtxCrop[5] = scaley;
			mtxCrop[12] = offsetx;
			mtxCrop[13] = offsety;

			bx::mtxMul(lightProj[ii], mtxCrop, mtxProj);
		}
	}

	// Reset render targets.
	const bgfx::FrameBufferHandle invalidRt = BGFX_INVALID_HANDLE;
	for (uint8_t ii = RENDERVIEW_FIRST; ii < RENDERVIEW_LAST + 1; ++ii)
	{
		bgfx::setViewFrameBuffer(ii, invalidRt);
		bgfx::setViewRect(ii, 0, 0, m_width, m_height);
	}

	// Determine on-screen rectangle size where depth buffer will be drawn.
	uint16_t depthRectHeight = uint16_t(float(m_height) / 2.5f);
	uint16_t depthRectWidth = depthRectHeight;
	uint16_t depthRectX = 0;
	uint16_t depthRectY = m_height - depthRectHeight;

	// Setup views and render targets.
	bgfx::setViewRect(0, 0, 0, m_width, m_height);
	bgfx::setViewTransform(0, &camera.GetViewMatrix(), &camera.GetProjectionMatrix()(0, 0));

	if (isSpot)
	{
		/**
		 * RENDERVIEW_SHADOWMAP_0_ID - Clear shadow map. (used as convenience, otherwise render_pass_1 could be cleared)
		 * RENDERVIEW_SHADOWMAP_1_ID - Craft shadow map.
		 * RENDERVIEW_VBLUR_0_ID - Vertical blur.
		 * RENDERVIEW_HBLUR_0_ID - Horizontal blur.
		 * RENDERVIEW_DRAWSCENE_0_ID - Draw scene.
		 * RENDERVIEW_DRAWSCENE_1_ID - Draw floor bottom.
		 * RENDERVIEW_DRAWDEPTH_0_ID - Draw depth buffer.
		 */
		 //TODO
		 //bgfx::setViewRect(RENDERVIEW_SHADOWMAP_0_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		 //bgfx::setViewRect(RENDERVIEW_SHADOWMAP_1_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		 //bgfx::setViewRect(RENDERVIEW_VBLUR_0_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		 //bgfx::setViewRect(RENDERVIEW_HBLUR_0_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		 //bgfx::setViewRect(RENDERVIEW_DRAWSCENE_0_ID, 0, 0, m_viewState.m_width, m_viewState.m_height);
		 //bgfx::setViewRect(RENDERVIEW_DRAWSCENE_1_ID, 0, 0, m_viewState.m_width, m_viewState.m_height);
		 //bgfx::setViewRect(RENDERVIEW_DRAWDEPTH_0_ID, depthRectX, depthRectY, depthRectWidth, depthRectHeight);

		 //bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_0_ID, screenView, screenProj);
		 //bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_1_ID, lightView[0], lightProj[ProjType::Horizontal]);
		 //bgfx::setViewTransform(RENDERVIEW_VBLUR_0_ID, screenView, screenProj);
		 //bgfx::setViewTransform(RENDERVIEW_HBLUR_0_ID, screenView, screenProj);
		 //bgfx::setViewTransform(RENDERVIEW_DRAWSCENE_0_ID, m_viewState.m_view, m_viewState.m_proj);
		 //bgfx::setViewTransform(RENDERVIEW_DRAWSCENE_1_ID, m_viewState.m_view, m_viewState.m_proj);
		 //bgfx::setViewTransform(RENDERVIEW_DRAWDEPTH_0_ID, screenView, screenProj);

		 //bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_0_ID, s_rtShadowMap[0]);
		 //bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_1_ID, s_rtShadowMap[0]);
		 //bgfx::setViewFrameBuffer(RENDERVIEW_VBLUR_0_ID, s_rtBlur);
		 //bgfx::setViewFrameBuffer(RENDERVIEW_HBLUR_0_ID, s_rtShadowMap[0]);
	}
	else if (isPoint)
	{
		/**
		 * RENDERVIEW_SHADOWMAP_0_ID - Clear entire shadow map.
		 * RENDERVIEW_SHADOWMAP_1_ID - Craft green tetrahedron shadow face.
		 * RENDERVIEW_SHADOWMAP_2_ID - Craft yellow tetrahedron shadow face.
		 * RENDERVIEW_SHADOWMAP_3_ID - Craft blue tetrahedron shadow face.
		 * RENDERVIEW_SHADOWMAP_4_ID - Craft red tetrahedron shadow face.
		 * RENDERVIEW_VBLUR_0_ID - Vertical blur.
		 * RENDERVIEW_HBLUR_0_ID - Horizontal blur.
		 * RENDERVIEW_DRAWSCENE_0_ID - Draw scene.
		 * RENDERVIEW_DRAWSCENE_1_ID - Draw floor bottom.
		 * RENDERVIEW_DRAWDEPTH_0_ID - Draw depth buffer.
		 */

		bgfx::setViewRect(RENDERVIEW_SHADOWMAP_0_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		if (m_stencilPack)
		{
			const uint16_t f = m_currentShadowMapSize;   //full size
			const uint16_t h = m_currentShadowMapSize / 2; //half size
			bgfx::setViewRect(RENDERVIEW_SHADOWMAP_1_ID, 0, 0, f, h);
			bgfx::setViewRect(RENDERVIEW_SHADOWMAP_2_ID, 0, h, f, h);
			bgfx::setViewRect(RENDERVIEW_SHADOWMAP_3_ID, 0, 0, h, f);
			bgfx::setViewRect(RENDERVIEW_SHADOWMAP_4_ID, h, 0, h, f);
		}
		else
		{
			const uint16_t h = m_currentShadowMapSize / 2; //half size
			bgfx::setViewRect(RENDERVIEW_SHADOWMAP_1_ID, 0, 0, h, h);
			bgfx::setViewRect(RENDERVIEW_SHADOWMAP_2_ID, h, 0, h, h);
			bgfx::setViewRect(RENDERVIEW_SHADOWMAP_3_ID, 0, h, h, h);
			bgfx::setViewRect(RENDERVIEW_SHADOWMAP_4_ID, h, h, h, h);
		}
		//bgfx::setViewRect(RENDERVIEW_VBLUR_0_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_HBLUR_0_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_DRAWSCENE_0_ID, 0, 0, m_width, m_height);
		//bgfx::setViewRect(RENDERVIEW_DRAWSCENE_1_ID, 0, 0, m_width, m_height);
		//bgfx::setViewRect(RENDERVIEW_DRAWDEPTH_0_ID, depthRectX, depthRectY, depthRectWidth, depthRectHeight);

		bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_0_ID, &screenView(0, 0), &screenProj(0, 0));
		bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_1_ID, lightView[TetrahedronFaces::Green], lightProj[ProjType::Horizontal]);
		bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_2_ID, lightView[TetrahedronFaces::Yellow], lightProj[ProjType::Horizontal]);
		if (m_stencilPack)
		{
			bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_3_ID, lightView[TetrahedronFaces::Blue], lightProj[ProjType::Vertical]);
			bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_4_ID, lightView[TetrahedronFaces::Red], lightProj[ProjType::Vertical]);
		}
		else
		{
			bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_3_ID, lightView[TetrahedronFaces::Blue], lightProj[ProjType::Horizontal]);
			bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_4_ID, lightView[TetrahedronFaces::Red], lightProj[ProjType::Horizontal]);
		}/*
		bgfx::setViewTransform(RENDERVIEW_VBLUR_0_ID, screenView, screenProj);
		bgfx::setViewTransform(RENDERVIEW_HBLUR_0_ID, screenView, screenProj);
		bgfx::setViewTransform(RENDERVIEW_DRAWSCENE_0_ID, camera->GetViewMatrix(), camera->GetProjectionMatrix());
		bgfx::setViewTransform(RENDERVIEW_DRAWSCENE_1_ID, camera->GetViewMatrix(), camera->GetProjectionMatrix());
		bgfx::setViewTransform(RENDERVIEW_DRAWDEPTH_0_ID, screenView, screenProj);*/

		bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_0_ID, s_rtShadowMap[0]);
		bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_1_ID, s_rtShadowMap[0]);
		bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_2_ID, s_rtShadowMap[0]);
		bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_3_ID, s_rtShadowMap[0]);
		bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_4_ID, s_rtShadowMap[0]);
		//bgfx::setViewFrameBuffer(RENDERVIEW_VBLUR_0_ID, s_rtBlur);
		//bgfx::setViewFrameBuffer(RENDERVIEW_HBLUR_0_ID, s_rtShadowMap[0]);
	}
	else // LightType::DirectionalLight == settings.m_lightType
	{
		/**
		 * RENDERVIEW_SHADOWMAP_1_ID - Craft shadow map for first  split.
		 * RENDERVIEW_SHADOWMAP_2_ID - Craft shadow map for second split.
		 * RENDERVIEW_SHADOWMAP_3_ID - Craft shadow map for third  split.
		 * RENDERVIEW_SHADOWMAP_4_ID - Craft shadow map for fourth split.
		 * RENDERVIEW_VBLUR_0_ID - Vertical   blur for first  split.
		 * RENDERVIEW_HBLUR_0_ID - Horizontal blur for first  split.
		 * RENDERVIEW_VBLUR_1_ID - Vertical   blur for second split.
		 * RENDERVIEW_HBLUR_1_ID - Horizontal blur for second split.
		 * RENDERVIEW_VBLUR_2_ID - Vertical   blur for third  split.
		 * RENDERVIEW_HBLUR_2_ID - Horizontal blur for third  split.
		 * RENDERVIEW_VBLUR_3_ID - Vertical   blur for fourth split.
		 * RENDERVIEW_HBLUR_3_ID - Horizontal blur for fourth split.
		 * RENDERVIEW_DRAWSCENE_0_ID - Draw scene.
		 * RENDERVIEW_DRAWSCENE_1_ID - Draw floor bottom.
		 * RENDERVIEW_DRAWDEPTH_0_ID - Draw depth buffer for first  split.
		 * RENDERVIEW_DRAWDEPTH_1_ID - Draw depth buffer for second split.
		 * RENDERVIEW_DRAWDEPTH_2_ID - Draw depth buffer for third  split.
		 * RENDERVIEW_DRAWDEPTH_3_ID - Draw depth buffer for fourth split.
		 */

		depthRectHeight = m_height / 3;
		depthRectWidth = depthRectHeight;
		depthRectX = 0;
		depthRectY = m_height - depthRectHeight;

		bgfx::setViewRect(RENDERVIEW_SHADOWMAP_1_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		bgfx::setViewRect(RENDERVIEW_SHADOWMAP_2_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		bgfx::setViewRect(RENDERVIEW_SHADOWMAP_3_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		bgfx::setViewRect(RENDERVIEW_SHADOWMAP_4_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_VBLUR_0_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_HBLUR_0_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_VBLUR_1_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_HBLUR_1_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_VBLUR_2_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_HBLUR_2_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_VBLUR_3_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_HBLUR_3_ID, 0, 0, m_currentShadowMapSize, m_currentShadowMapSize);
		//bgfx::setViewRect(RENDERVIEW_DRAWSCENE_0_ID, 0, 0, m_width, m_height);
		//bgfx::setViewRect(RENDERVIEW_DRAWSCENE_1_ID, 0, 0, m_width, m_height);
		//bgfx::setViewRect(RENDERVIEW_DRAWDEPTH_0_ID, depthRectX + (0 * depthRectWidth), depthRectY, depthRectWidth, depthRectHeight);
		//bgfx::setViewRect(RENDERVIEW_DRAWDEPTH_1_ID, depthRectX + (1 * depthRectWidth), depthRectY, depthRectWidth, depthRectHeight);
		//bgfx::setViewRect(RENDERVIEW_DRAWDEPTH_2_ID, depthRectX + (2 * depthRectWidth), depthRectY, depthRectWidth, depthRectHeight);
		//bgfx::setViewRect(RENDERVIEW_DRAWDEPTH_3_ID, depthRectX + (3 * depthRectWidth), depthRectY, depthRectWidth, depthRectHeight);

		bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_1_ID, lightView[0], lightProj[0]);
		bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_2_ID, lightView[0], lightProj[1]);
		bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_3_ID, lightView[0], lightProj[2]);
		bgfx::setViewTransform(RENDERVIEW_SHADOWMAP_4_ID, lightView[0], lightProj[3]);
		//bgfx::setViewTransform(RENDERVIEW_VBLUR_0_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_HBLUR_0_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_VBLUR_1_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_HBLUR_1_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_VBLUR_2_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_HBLUR_2_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_VBLUR_3_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_HBLUR_3_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_DRAWSCENE_0_ID, camera->GetViewMatrix(), camera->GetProjectionMatrix());
		//bgfx::setViewTransform(RENDERVIEW_DRAWSCENE_1_ID, camera->GetViewMatrix(), camera->GetProjectionMatrix());
		//bgfx::setViewTransform(RENDERVIEW_DRAWDEPTH_0_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_DRAWDEPTH_1_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_DRAWDEPTH_2_ID, screenView, screenProj);
		//bgfx::setViewTransform(RENDERVIEW_DRAWDEPTH_3_ID, screenView, screenProj);

		bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_1_ID, s_rtShadowMap[0]);
		bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_2_ID, s_rtShadowMap[1]);
		bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_3_ID, s_rtShadowMap[2]);
		bgfx::setViewFrameBuffer(RENDERVIEW_SHADOWMAP_4_ID, s_rtShadowMap[3]);
		//bgfx::setViewFrameBuffer(RENDERVIEW_VBLUR_0_ID, s_rtBlur);         //vblur
		//bgfx::setViewFrameBuffer(RENDERVIEW_HBLUR_0_ID, s_rtShadowMap[0]); //hblur
		//bgfx::setViewFrameBuffer(RENDERVIEW_VBLUR_1_ID, s_rtBlur);         //vblur
		//bgfx::setViewFrameBuffer(RENDERVIEW_HBLUR_1_ID, s_rtShadowMap[1]); //hblur
		//bgfx::setViewFrameBuffer(RENDERVIEW_VBLUR_2_ID, s_rtBlur);         //vblur
		//bgfx::setViewFrameBuffer(RENDERVIEW_HBLUR_2_ID, s_rtShadowMap[2]); //hblur
		//bgfx::setViewFrameBuffer(RENDERVIEW_VBLUR_3_ID, s_rtBlur);         //vblur
		//bgfx::setViewFrameBuffer(RENDERVIEW_HBLUR_3_ID, s_rtShadowMap[3]); //hblur
	}

	// Clear backbuffer at beginning.
	bgfx::setViewClear(0
		, BGFX_CLEAR_COLOR
		| BGFX_CLEAR_DEPTH
		, clearRgba
		, clearDepth
		, clearStencil
	);
	bgfx::touch(0);

	// Clear shadowmap rendertarget at beginning.
	const uint8_t flags0 = (isDirectional)
		? 0
		: BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL
		;

	bgfx::setViewClear(RENDERVIEW_SHADOWMAP_0_ID
		, flags0
		, 0xfefefefe //blur fails on completely white regions
		, clearDepth
		, clearStencil
	);
	bgfx::touch(RENDERVIEW_SHADOWMAP_0_ID);

	const uint8_t flags1 = (isDirectional)
		? BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
		: 0
		;

	for (uint8_t ii = 0; ii < 4; ++ii)
	{
		bgfx::setViewClear(RENDERVIEW_SHADOWMAP_1_ID + ii
			, flags1
			, 0xfefefefe //blur fails on completely white regions
			, clearDepth
			, clearStencil
		);
		bgfx::touch(RENDERVIEW_SHADOWMAP_1_ID + ii);
	}

	// Render.

	// Craft shadow map.
	{
		// Craft stencil mask for point light shadow map packing.
		if (isPoint && m_stencilPack)
		{
			//TODO
			//if (6 == bgfx::getAvailTransientVertexBuffer(6, m_posLayout))
			//{
			//	struct Pos
			//	{
			//		float m_x, m_y, m_z;
			//	};

			//	bgfx::TransientVertexBuffer vb;
			//	bgfx::allocTransientVertexBuffer(&vb, 6, m_posLayout);
			//	Pos* vertex = (Pos*)vb.data;

			//	const float min = 0.0f;
			//	const float max = 1.0f;
			//	const float center = 0.5f;
			//	const float zz = 0.0f;

			//	vertex[0].m_x = min;
			//	vertex[0].m_y = min;
			//	vertex[0].m_z = zz;

			//	vertex[1].m_x = max;
			//	vertex[1].m_y = min;
			//	vertex[1].m_z = zz;

			//	vertex[2].m_x = center;
			//	vertex[2].m_y = center;
			//	vertex[2].m_z = zz;

			//	vertex[3].m_x = center;
			//	vertex[3].m_y = center;
			//	vertex[3].m_z = zz;

			//	vertex[4].m_x = max;
			//	vertex[4].m_y = max;
			//	vertex[4].m_z = zz;

			//	vertex[5].m_x = min;
			//	vertex[5].m_y = max;
			//	vertex[5].m_z = zz;

			//	bgfx::setState(0);
			//	bgfx::setStencil(BGFX_STENCIL_TEST_ALWAYS
			//		| BGFX_STENCIL_FUNC_REF(1)
			//		| BGFX_STENCIL_FUNC_RMASK(0xff)
			//		| BGFX_STENCIL_OP_FAIL_S_REPLACE
			//		| BGFX_STENCIL_OP_FAIL_Z_REPLACE
			//		| BGFX_STENCIL_OP_PASS_Z_REPLACE
			//	);
			//	bgfx::setVertexBuffer(0, &vb);
			//	bgfx::submit(RENDERVIEW_SHADOWMAP_0_ID, s_programs.m_black);
			//}
		}

		// Draw scene into shadowmap.
		uint8_t drawNum;
		if (isSpot)
		{
			drawNum = 1;
		}
		else if (isPoint)
		{
			drawNum = 4;
		}
		else //LightType::DirectionalLight == settings.m_lightType)
		{
			drawNum = uint8_t(m_numSplits);
		}

		for (uint8_t ii = 0; ii < drawNum; ++ii)
		{
			const uint8_t viewId = RENDERVIEW_SHADOWMAP_1_ID + ii;

			//uint8_t renderStateIndex = RenderState::ShadowMap_PackDepth;
			if (isPoint && m_stencilPack)
			{
				//TODO
				//renderStateIndex = uint8_t((ii < 2) ? RenderState::ShadowMap_PackDepthHoriz : RenderState::ShadowMap_PackDepthVert);
			}
			//TODO
			ManualCamera virtualCamera;
			virtualCamera.viewMatrix = Matrix4(lightView[ii]);
			virtualCamera.projectionMatrix = Matrix4(lightProj[ii]);
			virtualCamera.OnBeforeRender();
			//virtualCamera.OnBeforeRender();
			Graphics::Get()->render->DrawAll(viewId, virtualCamera, shadowCasterMaterial);
		}


		// Setup shadow mtx.
		float mtxShadow[16];

		const float ymul = (s_flipV) ? 0.5f : -0.5f;
		float zadd = (DepthImpl::Linear == m_depthImpl) ? 0.0f : 0.5f;

		const float mtxBias[16] =
		{
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, ymul, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.5f, 0.5f, zadd, 1.0f,
		};

		if (isSpot)
		{
			float mtxTmp[16];
			bx::mtxMul(mtxTmp, lightProj[ProjType::Horizontal], mtxBias);
			bx::mtxMul(mtxShadow, lightView[0], mtxTmp); //lightViewProjBias
		}
		else if (isPoint)
		{
			const float s = (s_flipV) ? 1.0f : -1.0f; //sign
			zadd = (DepthImpl::Linear == m_depthImpl) ? 0.0f : 0.5f;

			const float mtxCropBias[2][TetrahedronFaces::Count][16] =
			{
				{ // settings.m_stencilPack == false

					{ // D3D: Green, OGL: Blue
						0.25f,    0.0f, 0.0f, 0.0f,
					 0.0f, s * 0.25f, 0.0f, 0.0f,
					 0.0f,    0.0f, 0.5f, 0.0f,
						0.25f,   0.25f, zadd, 1.0f,
					},
					{ // D3D: Yellow, OGL: Red
						0.25f,    0.0f, 0.0f, 0.0f,
					 0.0f, s * 0.25f, 0.0f, 0.0f,
					 0.0f,    0.0f, 0.5f, 0.0f,
						0.75f,   0.25f, zadd, 1.0f,
					},
					{ // D3D: Blue, OGL: Green
						0.25f,    0.0f, 0.0f, 0.0f,
					 0.0f, s * 0.25f, 0.0f, 0.0f,
					 0.0f,    0.0f, 0.5f, 0.0f,
						0.25f,   0.75f, zadd, 1.0f,
					},
					{ // D3D: Red, OGL: Yellow
						0.25f,    0.0f, 0.0f, 0.0f,
					 0.0f, s * 0.25f, 0.0f, 0.0f,
					 0.0f,    0.0f, 0.5f, 0.0f,
						0.75f,   0.75f, zadd, 1.0f,
					},
				},
				{ // settings.m_stencilPack == true

					{ // D3D: Red, OGL: Blue
						0.25f,   0.0f, 0.0f, 0.0f,
					 0.0f, s * 0.5f, 0.0f, 0.0f,
					 0.0f,   0.0f, 0.5f, 0.0f,
						0.25f,   0.5f, zadd, 1.0f,
					},
					{ // D3D: Blue, OGL: Red
						0.25f,   0.0f, 0.0f, 0.0f,
					 0.0f, s * 0.5f, 0.0f, 0.0f,
					 0.0f,   0.0f, 0.5f, 0.0f,
						0.75f,   0.5f, zadd, 1.0f,
					},
					{ // D3D: Green, OGL: Green
						0.5f,    0.0f, 0.0f, 0.0f,
						0.0f, s * 0.25f, 0.0f, 0.0f,
						0.0f,    0.0f, 0.5f, 0.0f,
						0.5f,   0.75f, zadd, 1.0f,
					},
					{ // D3D: Yellow, OGL: Yellow
						0.5f,    0.0f, 0.0f, 0.0f,
						0.0f, s * 0.25f, 0.0f, 0.0f,
						0.0f,    0.0f, 0.5f, 0.0f,
						0.5f,   0.25f, zadd, 1.0f,
					},
				}
			};

			//Use as: [stencilPack][flipV][tetrahedronFace]
			static const uint8_t cropBiasIndices[2][2][4] =
			{
				{ // settings.m_stencilPack == false
					{ 0, 1, 2, 3 }, //flipV == false
					{ 2, 3, 0, 1 }, //flipV == true
				},
				{ // settings.m_stencilPack == true
					{ 3, 2, 0, 1 }, //flipV == false
					{ 2, 3, 0, 1 }, //flipV == true
				},
			};

			for (uint8_t ii = 0; ii < TetrahedronFaces::Count; ++ii)
			{
				ProjType::Enum projType = (m_stencilPack) ? ProjType::Enum(ii > 1) : ProjType::Horizontal;
				uint8_t biasIndex = cropBiasIndices[m_stencilPack][uint8_t(s_flipV)][ii];

				float mtxTmp[16];
				bx::mtxMul(mtxTmp, mtxYpr[ii], lightProj[projType]);
				bx::mtxMul(m_shadowMapMtx[ii], mtxTmp, mtxCropBias[m_stencilPack][biasIndex]); //mtxYprProjBias
			}

			bx::mtxTranslate(mtxShadow //lightInvTranslate
				, -lightPosition.x
				, -lightPosition.y
				, -lightPosition.z
			);
		}
		else //LightType::DirectionalLight == settings.m_lightType
		{
			for (uint8_t ii = 0; ii < m_numSplits; ++ii)
			{
				float mtxTmp[16];

				bx::mtxMul(mtxTmp, lightProj[ii], mtxBias);
				bx::mtxMul(m_shadowMapMtx[ii], lightView[0], mtxTmp); //lViewProjCropBias
			}
		}
	}

	//TODO render->GetOrCreate();
	auto& matrixUniforms = Graphics::Get()->render->matrixUniforms;
	for (int i = 0; i < _countof(m_shadowMapMtx); i++) {
		std::string name = FormatString("u_shadowMapMtx%d", i);
		auto it = matrixUniforms.find(name);
		bgfx::UniformHandle uniform;
		if (it == matrixUniforms.end()) {
			uniform = bgfx::createUniform(name.c_str(), bgfx::UniformType::Mat4);
			matrixUniforms[name] = uniform;
		}
		else {
			uniform = it->second;
		}
		bgfx::setUniform(uniform, m_shadowMapMtx[i]);//TODO set default if no shadow
	}

	auto render = Graphics::Get()->render;
	auto& textureUniforms = Graphics::Get()->render->textureUniforms;

	for (uint8_t ii = 0; ii < ShadowMapRenderTargets::Count; ++ii)
	{
		std::string name = FormatString("s_shadowMap%d", ii);
		auto it = textureUniforms.find(name);
		bgfx::UniformHandle uniform;
		if (it == textureUniforms.end()) {
			uniform = bgfx::createUniform(name.c_str(), bgfx::UniformType::Sampler);
			textureUniforms[name] = uniform;
		}
		else {
			uniform = it->second;
		}
		bgfx::setTexture(4 + ii, uniform, bgfx::getTexture(s_rtShadowMap[ii]));
	}

	Vector4 v;

	v = Vector4(1, 1, 0, 0);
	bgfx::setUniform(render->GetOrCreateVectorUniform("u_params0"), &v.x);

	v = Vector4(0.0012f, 0.001f, 0.7f, 500.0f);
	bgfx::setUniform(render->GetOrCreateVectorUniform("u_params1"), &v.x);

	v = Vector4(1, 0, 1.f / shadowMapSize, 0);
	bgfx::setUniform(render->GetOrCreateVectorUniform("u_params2"), &v.x);

	v = Vector4(2, 2, 0.2f, 0.2f);
	bgfx::setUniform(render->GetOrCreateVectorUniform("u_smSamplingParams"), &v.x);

	v = Vector4(2, 2, 0.2f, 0.2f);
	bgfx::setUniform(render->GetOrCreateVectorUniform("u_csmFarDistances"), &s_uniforms.m_csmFarDistances);
}
