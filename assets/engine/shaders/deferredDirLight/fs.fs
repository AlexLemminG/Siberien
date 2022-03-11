/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

vec2 dirLightBlinn(vec3 _lightDir, vec3 _normal, vec3 _viewDir)
{
	float ndotl = dot(_normal, _lightDir);
	vec3 reflected = _lightDir - 2.0*ndotl*_normal; // reflect(_lightDir, _normal);
	float rdotv = dot(reflected, _viewDir);
	return vec2(ndotl, rdotv);
}

float fresnel(float _ndotl, float _bias, float _pow)
{
	float facing = (1.0 - _ndotl);
	return max(_bias + (1.0 - _bias) * pow(facing, _pow), 0.0);
}

vec4 dirLightLit(float _ndotl, float _rdotv, float _m)
{
	float diff = max(0.0, _ndotl);
	float spec = step(0.0, _ndotl) * max(0.0, _rdotv * _m);
	return vec4(1.0, diff, spec, 1.0);
}

vec4 powRgba(vec4 _rgba, float _pow)
{
	vec4 result;
	result.xyz = pow(_rgba.xyz, vec3_splat(_pow) );
	result.w = _rgba.w;
	return result;
}

vec3 dirLightCalcLight(vec3 _normal, vec3 _view, vec3 _lightRgb, vec3 _lightDir)
{
	vec3 lp = -_lightDir;
	float attn = 1.0;
	vec3 lightDir = normalize(lp);
	vec2 bln = dirLightBlinn(lightDir, _normal, _view);
	vec4 lc = dirLightLit(bln.x, bln.y, 1.0);
	vec3 rgb = _lightRgb * saturate(lc.y) * attn;
	return rgb;
}


uniform vec4 u_lightDir[1];
uniform vec4 u_lightColor[1];
uniform mat4 u_viewProjInv;

uniform mat4 u_lightMtx;
uniform mat4 u_shadowMapMtx0;
uniform mat4 u_shadowMapMtx1;
uniform mat4 u_shadowMapMtx2;
uniform mat4 u_shadowMapMtx3;

//#define SM_HARD 1
//#define SM_LINEAR 1
#define SM_CSM 1
#define SM_PCF 1

#include "fs_shadowmaps_color_lighting.sh"

float dirLightVisibility(vec3 worldPos){
	vec3 w_pos = worldPos;
	vec3 w_normal = vec3_splat(0.0);
	
	vec3 wpos = w_pos;

	vec3 view = mul(u_view, vec4(wpos, 0.0) ).xyz;
	view = -normalize(view);

	vec3 v_view = view;
	vec4 v_worldPos = vec4(wpos, 1.0);	
	
	vec4 v_texcoord1 = mul(u_shadowMapMtx0, v_worldPos);
	vec4 v_texcoord2 = mul(u_shadowMapMtx1, v_worldPos);
	vec4 v_texcoord3 = mul(u_shadowMapMtx2, v_worldPos);
	vec4 v_texcoord4 = mul(u_shadowMapMtx3, v_worldPos);
	//TODO why?
	v_texcoord1.z += 0.5;
	v_texcoord2.z += 0.5;
	v_texcoord3.z += 0.5;
	v_texcoord4.z += 0.5;
	
	float visibility;
	
#include "fs_shadowmaps_color_lighting_main.sh"

	return visibility;
}

vec3 dirLight(vec3 w_normal, vec3 w_pos)
{
	vec3 wpos = w_pos;

	vec3 view = mul(u_view, vec4(wpos, 0.0) ).xyz;
	view = -normalize(view);

	vec3 lightColor = dirLightCalcLight(w_normal, view, u_lightColor[0].xyz, u_lightDir[0].xyz);
	
	vec3 v_view = view;
	vec4 v_worldPos = vec4(wpos, 1.0);	
	
	vec4 v_texcoord1 = mul(u_shadowMapMtx0, v_worldPos);
	vec4 v_texcoord2 = mul(u_shadowMapMtx1, v_worldPos);
	vec4 v_texcoord3 = mul(u_shadowMapMtx2, v_worldPos);
	vec4 v_texcoord4 = mul(u_shadowMapMtx3, v_worldPos);
	//TODO why?
	v_texcoord1.z += 0.5;
	v_texcoord2.z += 0.5;
	v_texcoord3.z += 0.5;
	v_texcoord4.z += 0.5;
	
	float visibility;
	
	//TODO as func and not like this
#include "fs_shadowmaps_color_lighting_main.sh"

	return lightColor.xyz * visibility;
}
