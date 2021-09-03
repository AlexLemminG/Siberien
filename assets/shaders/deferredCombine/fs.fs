$input v_texcoord0

/*
 * Copyright 2018 Eric Arnebäck. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

SAMPLER2D(s_albedo, 0);
SAMPLER2D(s_normal, 1);
SAMPLER2D(s_light,  2);
SAMPLER2D(s_emissive,  3);
SAMPLER2D(s_depth,  4);

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



void main()
{
	vec3 albedo = toLinear(texture2D(s_albedo, v_texcoord0).rgb);
	
	vec3 normal = decodeNormalUint(texture2D(s_normal, v_texcoord0).xyz);
	
	vec3 emissive = toLinear(texture2D(s_emissive, v_texcoord0).rgb);
	vec3 light = toLinear(texture2D(s_light, v_texcoord0).rgb) * 5.0;
	
	vec3 lightColor = light;
	lightColor += SampleSH(normal, u_sphericalHarmonics);


	vec3 color = max(vec3_splat(0.05), lightColor) * albedo + emissive;
	
	gl_FragColor.rgb = toGamma(color);
	gl_FragColor.w = 1.0;
}
