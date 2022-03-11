/*
 * Copyright 2013-2014 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

float linstep(float _edge0, float _edge1, float _x)
{
	return clamp((_x-_edge0)/(_edge1-_edge0), 0.0, 1.0);
}

float attenuation(float _dist, vec3 _attn)
{
	return 1.0 / ( _attn.x                  //const
				 + _attn.y * _dist          //linear
				 + _attn.z * _dist * _dist  //quadrantic
				 );
}

float spot(float _ldotsd, float _inner, float _outer)
{
	float inner = cos(radians(_inner));
	float outer = cos(radians(min(_outer, _inner - 0.001)));
	float spot = clamp((_ldotsd - inner) / (outer - inner), 0.0, 1.0);
	return spot;
}

vec2 lit(vec3 _ld, vec3 _n, vec3 _vd, float _exp)
{
	//diff
	float ndotl = dot(_n, _ld);

	//spec
	vec3 r = 2.0*ndotl*_n - _ld; // reflect(_ld, _n);
	float rdotv = dot(r, _vd);
	float spec = step(0.0, ndotl) * pow(max(0.0, rdotv), _exp) * (2.0 + _exp)/8.0;

	return max(vec2(ndotl, spec), 0.0);
}

struct Light
{
	vec3 l;
	vec3 ld;
	float attn;
};

Light evalLight(vec3 _v, vec4 _l, vec3 _spotDirection, float _spotInner, float _spotOuter, vec3 _attnParams)
{
	Light light;

	//directional
	light.l    = _l.xyz;
	light.ld   = -normalize(light.l);
	light.attn = 1.0;

	if (0.0 != _l.w) //point or spot
	{
		light.l  = _l.xyz - _v;
		light.ld = normalize(light.l);

		float ldotsd = max(0.0, dot(-light.ld, normalize(_spotDirection)));
		float falloff = spot(ldotsd, _spotOuter, _spotInner);
		light.attn = attenuation(length(light.l), _attnParams) * mix(falloff, 1.0, step(90, _spotOuter));
	}

	return light;
}

float texcoordInRange(vec2 _texcoord)
{
	bool inRange = all(greaterThan(_texcoord, vec2_splat(0.0)))
				&& all(lessThan   (_texcoord, vec2_splat(1.0)))
				 ;

	return float(inRange);
}

float hardShadow(sampler2DShadow _sampler, vec4 _shadowCoord, float _bias)
{
	vec3 texCoord = _shadowCoord.xyz/_shadowCoord.w;

	bool outside = any(greaterThan(texCoord.xy, vec2_splat(1.0)))
				|| any(lessThan   (texCoord.xy, vec2_splat(0.0)))
				 ;

	if (outside)
	{
		return 1.0;
	}
	float receiver = texCoord.z-_bias;
	float visibility = shadow2D(_sampler, vec3(texCoord.xy, receiver)).r;
	
	return visibility;
}

float OccluderDistance(sampler2D _sampler, vec4 _shadowCoord, vec4 _shadowCoordWithOffset, float _bias)
{
	vec3 texCoord = _shadowCoordWithOffset.xyz/_shadowCoordWithOffset.w;

	bool outside = any(greaterThan(texCoord.xy, vec2_splat(1.0)))
				|| any(lessThan   (texCoord.xy, vec2_splat(0.0)))
				 ;

	if (outside)
	{
		return 1.0;
	}
	float receiver = _shadowCoord.z/_shadowCoord.w-_bias;
	float occluder = texture2D(_sampler, texCoord.xy) ;
	
	return max(0.0, receiver - occluder);
}


float PCFSoft(sampler2DShadow _sampler, sampler2D _samplerSimple, vec4 _shadowCoord, float _bias, vec4 _pcfParams, vec2 _texelSize)
{
	float result = 0.0;
	vec2 offset = _pcfParams.zw * _texelSize * _shadowCoord.w;
	
	float distanceBias = 0.001;
	float f = 0.0;//OccluderDistance(_samplerSimple, _shadowCoord, _shadowCoord, distanceBias);
	//return f;
	for (int dx = -1; dx <= 1; dx += 1){
		for (int dy = -1; dy <= 1; dy += 1){
			f = max(f, OccluderDistance(_samplerSimple, _shadowCoord, _shadowCoord + vec4(vec2(dx, dy) * offset*1.0, 0.0, 0.0), distanceBias));
		}
	}
	//f = min(f, 1.0);
	//f = pow(f, 2);
	f /= 9;
	//return f;
	if(f == 0.0){
		//return f;
	}
	//return f;
	offset *= f * 10.0;
	//return offset.x;
	//result = f;
	
	for (int dx = -3; dx <= 3; dx += 2){
		for (int dy = -3; dy <= 3; dy += 2){
			result += hardShadow(_sampler, _shadowCoord + vec4(vec2(dx, dy) * offset, 0.0, 0.0), _bias + distanceBias);
		}
	}

	return result / 16.0;
}
float PCFHard(sampler2DShadow _sampler, sampler2D _samplerSimple, vec4 _shadowCoord, float _bias, vec4 _pcfParams, vec2 _texelSize)
{
	float result = 0.0;
	vec2 offset = _pcfParams.zw * _texelSize * _shadowCoord.w;
	
	for (int dx = -3; dx <= 3; dx += 2){
		for (int dy = -3; dy <= 3; dy += 2){
			result += hardShadow(_sampler, _shadowCoord + vec4(vec2(dx, dy) * offset, 0.0, 0.0), _bias);
		}
	}

	return result / 16.0;
}
float PCF(sampler2DShadow _sampler, sampler2D _samplerSimple, vec4 _shadowCoord, float _bias, vec4 _pcfParams, vec2 _texelSize)
{
	return PCFHard(_sampler, _samplerSimple, _shadowCoord, _bias, _pcfParams, _texelSize);
}

float VSM(sampler2DShadow _sampler, vec4 _shadowCoord, float _bias, float _depthMultiplier, float _minVariance)
{
	//vec2 texCoord = _shadowCoord.xy/_shadowCoord.w;
	//
	//bool outside = any(greaterThan(texCoord, vec2_splat(1.0)))
	//			|| any(lessThan   (texCoord, vec2_splat(0.0)))
	//			 ;
	//
	//if (outside)
	//{
	//	return 1.0;
	//}
	//
	//float receiver = (_shadowCoord.z-_bias)/_shadowCoord.w * _depthMultiplier;
	//vec4 rgba = shadow2D(_sampler, vec3(texCoord, 0.0)); //TODO not 0.0
	//vec2 occluder = vec2(unpackHalfFloat(rgba.rg), unpackHalfFloat(rgba.ba)) * _depthMultiplier;
	//
	//if (receiver < occluder.x)
	//{
	//	return 1.0;
	//}
	//
	//float variance = max(occluder.y - (occluder.x*occluder.x), _minVariance);
	//float d = receiver - occluder.x;
	//
	//float visibility = variance / (variance + d*d);
	//
	//return visibility;
	return 0.0;
}

float ESM(sampler2DShadow _sampler, vec4 _shadowCoord, float _bias, float _depthMultiplier)
{
	//vec2 texCoord = _shadowCoord.xy/_shadowCoord.w;
	//
	//bool outside = any(greaterThan(texCoord, vec2_splat(1.0)))
	//			|| any(lessThan   (texCoord, vec2_splat(0.0)))
	//			 ;
	//
	//if (outside)
	//{
	//	return 1.0;
	//}
	//
	//float receiver = (_shadowCoord.z-_bias)/_shadowCoord.w;
	//float occluder = unpackRgbaToFloat(shadow2D(_sampler, vec3(texCoord, 0.0)) );//TODO not 0.0
	//
	//float visibility = clamp(exp(_depthMultiplier * (occluder-receiver) ), 0.0, 1.0);
	//
	//return visibility;
	return 0.0;
}


vec4 blur9(sampler2DShadow _sampler, vec2 _uv0, vec4 _uv1, vec4 _uv2, vec4 _uv3, vec4 _uv4)
{
#define _BLUR9_WEIGHT_0 1.0
#define _BLUR9_WEIGHT_1 0.9
#define _BLUR9_WEIGHT_2 0.55
#define _BLUR9_WEIGHT_3 0.18
#define _BLUR9_WEIGHT_4 0.1
#define _BLUR9_NORMALIZE (_BLUR9_WEIGHT_0+2.0*(_BLUR9_WEIGHT_1+_BLUR9_WEIGHT_2+_BLUR9_WEIGHT_3+_BLUR9_WEIGHT_4) )
#define BLUR9_WEIGHT(_x) (_BLUR9_WEIGHT_##_x/_BLUR9_NORMALIZE)

	float blur;
	//blur  = unpackRgbaToFloat(texture2D(_sampler, _uv0)    * BLUR9_WEIGHT(0));
	//blur += unpackRgbaToFloat(texture2D(_sampler, _uv1.xy) * BLUR9_WEIGHT(1));
	//blur += unpackRgbaToFloat(texture2D(_sampler, _uv1.zw) * BLUR9_WEIGHT(1));
	//blur += unpackRgbaToFloat(texture2D(_sampler, _uv2.xy) * BLUR9_WEIGHT(2));
	//blur += unpackRgbaToFloat(texture2D(_sampler, _uv2.zw) * BLUR9_WEIGHT(2));
	//blur += unpackRgbaToFloat(texture2D(_sampler, _uv3.xy) * BLUR9_WEIGHT(3));
	//blur += unpackRgbaToFloat(texture2D(_sampler, _uv3.zw) * BLUR9_WEIGHT(3));
	//blur += unpackRgbaToFloat(texture2D(_sampler, _uv4.xy) * BLUR9_WEIGHT(4));
	//blur += unpackRgbaToFloat(texture2D(_sampler, _uv4.zw) * BLUR9_WEIGHT(4));
	return packFloatToRgba(blur);
}

vec4 blur9VSM(sampler2DShadow _sampler, vec2 _uv0, vec4 _uv1, vec4 _uv2, vec4 _uv3, vec4 _uv4)
{
#define _BLUR9_WEIGHT_0 1.0
#define _BLUR9_WEIGHT_1 0.9
#define _BLUR9_WEIGHT_2 0.55
#define _BLUR9_WEIGHT_3 0.18
#define _BLUR9_WEIGHT_4 0.1
#define _BLUR9_NORMALIZE (_BLUR9_WEIGHT_0+2.0*(_BLUR9_WEIGHT_1+_BLUR9_WEIGHT_2+_BLUR9_WEIGHT_3+_BLUR9_WEIGHT_4) )
#define BLUR9_WEIGHT(_x) (_BLUR9_WEIGHT_##_x/_BLUR9_NORMALIZE)

	vec2 blur;
	vec4 val;
	//val = texture2D(_sampler, _uv0) * BLUR9_WEIGHT(0);
	//blur = vec2(unpackHalfFloat(val.rg), unpackHalfFloat(val.ba));
	//val = texture2D(_sampler, _uv1.xy) * BLUR9_WEIGHT(1);
	//blur += vec2(unpackHalfFloat(val.rg), unpackHalfFloat(val.ba));
	//val = texture2D(_sampler, _uv1.zw) * BLUR9_WEIGHT(1);
	//blur += vec2(unpackHalfFloat(val.rg), unpackHalfFloat(val.ba));
	//val = texture2D(_sampler, _uv2.xy) * BLUR9_WEIGHT(2);
	//blur += vec2(unpackHalfFloat(val.rg), unpackHalfFloat(val.ba));
	//val = texture2D(_sampler, _uv2.zw) * BLUR9_WEIGHT(2);
	//blur += vec2(unpackHalfFloat(val.rg), unpackHalfFloat(val.ba));
	//val = texture2D(_sampler, _uv3.xy) * BLUR9_WEIGHT(3);
	//blur += vec2(unpackHalfFloat(val.rg), unpackHalfFloat(val.ba));
	//val = texture2D(_sampler, _uv3.zw) * BLUR9_WEIGHT(3);
	//blur += vec2(unpackHalfFloat(val.rg), unpackHalfFloat(val.ba));
	//val = texture2D(_sampler, _uv4.xy) * BLUR9_WEIGHT(4);
	//blur += vec2(unpackHalfFloat(val.rg), unpackHalfFloat(val.ba));
	//val = texture2D(_sampler, _uv4.zw) * BLUR9_WEIGHT(4);
	//blur += vec2(unpackHalfFloat(val.rg), unpackHalfFloat(val.ba));

	return vec4(packHalfFloat(blur.x), packHalfFloat(blur.y));
}
