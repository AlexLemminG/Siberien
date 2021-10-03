$input v_wpos, v_view, v_proj, v_normal, v_tangent, v_bitangent, v_texcoord0// in...

/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

SAMPLER2D(s_texColor,  0);
SAMPLER2D(s_texNormal, 1);
SAMPLER2D(s_texEmissive, 2);
USAMPLER2D(s_texCluster, 3);
USAMPLER2D(s_texItems, 4);
SAMPLER2D(s_texLightParams, 5);

//TODO one big shadowmap with different + uvs
SAMPLER2D(s_shadowMap0, 6);
SAMPLER2D(s_shadowMap1, 7);
SAMPLER2D(s_shadowMap2, 8);
SAMPLER2D(s_shadowMap3, 9);

#include "../deferredDirLight/fs.fs"

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

struct ClusterData{
	uint offset;
	uint lightsCount;
};

struct ItemData{
	uint lightIdx;
};

struct LightData{
	vec3 pos;
	float radius;
	vec3 color;
	float innerRadius;
};

ClusterData GetClasterData(vec4 proj){
	int clusterWidth = 16;
	int clusterHeight = 8;
	int clusterDepth = 1;
	float3 clip;
	clip.xy = (proj.xy / proj.w + 1.0) / 2.0;
	clip.z = (proj.z / proj.w);
	ivec3 cluster = ivec3(int(clip.x * clusterWidth), int(clip.y * clusterHeight), int(clip.z * clusterDepth));
	
	uvec2 rg = texelFetch(s_texCluster, ivec2(cluster.x * clusterDepth * clusterHeight + cluster.y * clusterDepth + cluster.z,0), 0).rg;
	
	ClusterData data;
	data.offset = rg.r;
	data.lightsCount = rg.g & 255;
	return data;
}

ItemData GetItemData(int offset){
	
	int itemsDiv = 1024;
	int x = offset % itemsDiv;
	int y = offset / itemsDiv;
	uvec2 rg = texelFetch(s_texItems, ivec2(x,y), 0).rg;
	
	ItemData data;
	data.lightIdx = rg.r;
	return data;
}

LightData GetLightData(int offset){
	vec4 raw1 = texelFetch(s_texLightParams, ivec2(offset*2,0), 0).rgba;
	vec4 raw2 = texelFetch(s_texLightParams, ivec2(offset*2+1,0), 0).rgba;
	
	LightData data;
	data.pos = raw1.xyz;
	data.radius = raw1.w;
	data.color = raw2.xyz;
	data.innerRadius = raw2.w;
	return data;
}

vec2 blinn(vec3 _lightDir, vec3 _normal, vec3 _viewDir)
{
	float ndotl = dot(_normal, _lightDir);
	//vec3 reflected = _lightDir - 2.0*ndotl*_normal; // reflect(_lightDir, _normal);
	vec3 reflected = 2.0*ndotl*_normal - _lightDir;
	float rdotv = dot(reflected, _viewDir);
	return vec2(ndotl, rdotv);
}

vec4 lit(float _ndotl, float _rdotv, float _m)
{
	float diff = max(0.0, _ndotl);
	float spec = step(0.0, _ndotl) * max(0.0, _rdotv * _m);
	return vec4(1.0, diff, spec, 1.0);
}

vec3 CalcLight(LightData light, mat3 tbn, vec3 worldPos, vec3 worldNormal, vec3 view){
	
	vec3 lp = light.pos - worldPos;
	if(dot(lp,lp) > light.radius * light.radius){
		return vec3(0.0,0.0,0.0);
	}
	float attn = 1.0 - smoothstep(light.innerRadius, 1.0, length(lp) / light.radius);
	vec3 lightDir = mul( normalize(lp), tbn );
	vec2 bln = blinn(normalize(lp), worldNormal, view);
	vec4 lc = lit(bln.x, bln.y, 1.0);
	vec3 rgb = light.color * saturate(lc.y) * attn;
	
	return rgb;
}


void main()
{
	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);

	vec3 normal;
	normal.xy = texture2D(s_texNormal, v_texcoord0).xy * 2.0 - 1.0;
	normal.z = sqrt(1.0 - dot(normal.xy, normal.xy) );
	
	vec3 view = normalize(v_view);

	vec3 wnormal = mul(tbn, normal);
	
	vec4 albedo = toLinear(texture2D(s_texColor, v_texcoord0));
	vec3 emissive = toLinear(texture2D(s_texEmissive, v_texcoord0)).rgb;
	vec3 color = vec3(0.0,0.0,0.0);

	ClusterData clusterData = GetClasterData(v_proj);
	for(int i = 0; i < clusterData.lightsCount; i++){
		ItemData item = GetItemData(i + clusterData.offset);
		LightData light = GetLightData(item.lightIdx);
		color.rgb += CalcLight(light, tbn, v_wpos, wnormal, view);
	}
	color.rgb += dirLight(wnormal, v_wpos);
	color.rgb += SampleSH(wnormal, u_sphericalHarmonics);
	color.rgb *= albedo.rgb;
	color.rgb += emissive;
	
	
	gl_FragData[0].rgb = toGamma(color);
	
	
#if TRANSPARENT
	gl_FragData[0].a = albedo.a;
#endif
}
