$input v_texcoord0

/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

vec2 blinn(vec3 _lightDir, vec3 _normal, vec3 _viewDir)
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

vec4 lit(float _ndotl, float _rdotv, float _m)
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

vec3 calcLight(vec3 _normal, vec3 _view, vec3 _lightRgb, vec3 _lightDir)
{
	vec3 lp = -_lightDir;
	float attn = 1.0;
	vec3 lightDir = normalize(lp);
	vec2 bln = blinn(lightDir, _normal, _view);
	vec4 lc = lit(bln.x, bln.y, 1.0);
	vec3 rgb = _lightRgb * saturate(lc.y) * attn;
	return rgb;
}



SAMPLER2D(s_normal, 0);
SAMPLER2D(s_depth,  1);

uniform vec4 u_lightDir[1];
uniform vec4 u_lightColor[1];
uniform mat4 u_viewProjInv;

uniform mat4 u_lightMtx;
uniform mat4 u_shadowMapMtx0;
uniform mat4 u_shadowMapMtx1;
uniform mat4 u_shadowMapMtx2;
uniform mat4 u_shadowMapMtx3;

#define SM_HARD 1
#define SM_LINEAR 1
#define SM_CSM 1

#include "fs_shadowmaps_color_lighting.sh"

void main()
{
	vec3  normal      = decodeNormalUint(texture2D(s_normal, v_texcoord0).xyz);
	float deviceDepth = texture2D(s_depth, v_texcoord0).x;
	float depth       = toClipSpaceDepth(deviceDepth);
	
	

	vec3 clip = vec3(v_texcoord0 * 2.0 - 1.0, depth);
#if !BGFX_SHADER_LANGUAGE_GLSL
	clip.y = -clip.y;
#endif // !BGFX_SHADER_LANGUAGE_GLSL
	vec3 wpos = clipToWorld(u_viewProjInv, clip);

	vec3 view = mul(u_view, vec4(wpos, 0.0) ).xyz;
	view = -normalize(view);

	vec3 lightColor = calcLight(normal, view, u_lightColor[0].xyz, u_lightDir[0].xyz);
	//gl_FragColor.xyz = toGamma(lightColor.xyz / 5.0);
	//gl_FragColor.w = 1.0;
	
	//return;
	
	
	
	
	vec3 v_view = view;
	vec3 v_normal = normal;
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
	
	
	
	
	
	
	
	
	//gl_FragColor.xyz = vec3_splat(v_texcoord3.w);
	//gl_FragColor.w = 1.0;
	//return;
	float visibility;
	
#include "fs_shadowmaps_color_lighting_main.sh"


	gl_FragColor.xyz = toGamma(lightColor.xyz / 5.0) * visibility;
	
	
	
	
	
	

	//gl_FragColor.xyz = vec3_splat(visibility);
}
