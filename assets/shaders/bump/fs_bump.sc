$input v_wpos, v_view, v_normal, v_tangent, v_bitangent, v_texcoord0// in...

/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

SAMPLER2D(s_texColor,  0);
SAMPLER2D(s_texNormal, 1);
SAMPLER2D(s_texEmissive, 2);
uniform vec4 u_lightPosRadius[8];
uniform vec4 u_lightRgbInnerR[8];


uniform vec4 u_sphericalHarmonics[9];

vec3 SampleSH(vec3 normal, vec4 sph[9]) {
  float x = normal.x;
  float y = normal.y;
  float z = normal.z;

  vec4 result = (
    sph[0] +

    sph[1] * x +
    sph[2] * y +
    sph[3] * z +

    sph[4] * z * x +
    sph[5] * y * z +
    sph[6] * y * x +
    sph[7] * (3.0 * z * z - 1.0) +
    sph[8] * (x*x - y*y)
  );

  return max(result.xyz, vec3(0.0, 0.0, 0.0));
}

vec2 blinn(vec3 _lightDir, vec3 _normal, vec3 _viewDir)
{
	float ndotl = dot(_normal, _lightDir);
	//vec3 reflected = _lightDir - 2.0*ndotl*_normal; // reflect(_lightDir, _normal);
	vec3 reflected = 2.0*ndotl*_normal - _lightDir;
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

vec3 calcLight(int _idx, mat3 _tbn, vec3 _wpos, vec3 _normal, vec3 _view)
{
	vec3 lp = u_lightPosRadius[_idx].xyz - _wpos;
	if(dot(lp,lp) > u_lightPosRadius[_idx].w * u_lightPosRadius[_idx].w){
		return vec3(0.0,0.0,0.0);
	}
	float attn = 1.0 - smoothstep(u_lightRgbInnerR[_idx].w, 1.0, length(lp) / u_lightPosRadius[_idx].w);
	vec3 lightDir = mul( normalize(lp), _tbn );
	vec2 bln = blinn(lightDir, _normal, _view);
	vec4 lc = lit(bln.x, bln.y, 1.0);
	vec3 rgb = u_lightRgbInnerR[_idx].xyz * saturate(lc.y) * attn;
	return rgb;
}

void main()
{
	
	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);

	vec3 normal;
	normal.xy = texture2D(s_texNormal, v_texcoord0).xy * 2.0 - 1.0;
	normal.z = sqrt(1.0 - dot(normal.xy, normal.xy) );
	vec3 view = normalize(v_view);

	vec3 lightColor = vec3(0.0,0.0,0.0);
	
	for(int i = 0; i < 8; i++){
		lightColor +=  calcLight(i, tbn, v_wpos, normal, view);
	}
	
	lightColor += SampleSH(mul(tbn, normal), u_sphericalHarmonics);

	vec4 color = toLinear(texture2D(s_texColor, v_texcoord0) );
	vec4 colorEmissive = toLinear(texture2D(s_texEmissive, v_texcoord0) );

	gl_FragColor.xyz = max(vec3_splat(0.05), lightColor.xyz)*color.xyz + colorEmissive;
	gl_FragColor.w = 1.0;
	gl_FragColor = toGamma(gl_FragColor);
}