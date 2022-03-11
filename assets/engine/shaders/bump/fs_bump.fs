$input v_wpos, v_normal, v_tangent, v_bitangent, v_texcoord0// in...

/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

SAMPLER2D(s_texColor,  0);
SAMPLER2D(s_texNormal, 1);
SAMPLER2D(s_texEmissive, 2);
SAMPLER2D(s_texMetalic, 3);
SAMPLER2D(s_texRoughness, 4);

USAMPLER2D(s_texCluster, 5);
USAMPLER2D(s_texItems, 6);
SAMPLER2D(s_texLightParams, 7);

//TODO one big shadowmap with different + uvs
SAMPLER2DSHADOW(s_shadowMap0, 8);
SAMPLER2DSHADOW(s_shadowMap1, 9);
SAMPLER2DSHADOW(s_shadowMap2, 10);
SAMPLER2DSHADOW(s_shadowMap3, 11);

uniform SamplerState s_shadowMap0SimpleSampler : REGISTER(s, 12);
static BgfxSampler2D s_shadowMap0Simple = { s_shadowMap0SimpleSampler, s_shadowMap0Texture };

#include "../deferredDirLight/fs.fs"

uniform vec4 u_sphericalHarmonics[9];
uniform vec4 u_cameraPos;
uniform vec4 u_emissiveColor;

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
	vec3 dir;
	float halfInnerAngle;
	float halfDeltaAngleInv;
	vec3 padding;
};

struct Surface{
	vec3 albedo;
	float alpha;
	vec3 emissive;
	vec3 normal;
	vec3 pos;
	float metalic;
	float roughness;
	vec3 reflectionAtZeroIncidence;
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
	vec4 raw1 = texelFetch(s_texLightParams, ivec2(offset*4,0), 0).rgba;
	vec4 raw2 = texelFetch(s_texLightParams, ivec2(offset*4+1,0), 0).rgba;
	vec4 raw3 = texelFetch(s_texLightParams, ivec2(offset*4+2,0), 0).rgba;
	vec4 raw4 = texelFetch(s_texLightParams, ivec2(offset*4+3,0), 0).rgba;
	
	LightData data;
	data.pos = raw1.xyz;
	data.radius = raw1.w;
	data.color = raw2.xyz;
	data.innerRadius = raw2.w;
	data.dir = raw3.xyz;
	data.halfInnerAngle = raw3.w;
	data.halfDeltaAngleInv = raw4.x;
	
	return data;
}

#define PI 3.14159265359

float DistributionGGX(vec3 N, vec3 H, float extraCos, float roughness)
{
	roughness = clamp(roughness, 0.1, 0.9);
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = saturate(dot(N, H) + pow(extraCos, 2.0));
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float extraCos, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = saturate(dot(N, L) + extraCos);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	// return vec3_splat(cosTheta);
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalcLightPBR(vec3 lightRadiance, vec3 lightDir, float extraCos, float extraCosReflection, Surface surface, vec3 viewDir){
	vec3 halfDir = normalize(lightDir + viewDir);
	float lightDotView = saturate(dot(halfDir, viewDir) + extraCos);
	vec3 F = fresnelSchlick(lightDotView, surface.reflectionAtZeroIncidence);
	float NDF = DistributionGGX(surface.normal, halfDir, extraCosReflection, surface.roughness);       
	float G   = GeometrySmith(surface.normal, viewDir, lightDir, extraCos, surface.roughness); 
	
	vec3 numerator    = NDF * G * F;
	float denominator = 4.0 * max(dot(surface.normal, viewDir), 0.0) * saturate(dot(surface.normal, lightDir) + extraCos)  + 0.0001;
	vec3 specular     = numerator / denominator;  
	
	vec3 kS = F;
	vec3 kD = vec3_splat(1.0) - kS;
	  
	kD *= 1.0 - surface.metalic;
	
	float NdotL = saturate(dot(surface.normal, lightDir)+extraCos);       
	
	return (kD * surface.albedo / PI + specular) * lightRadiance * NdotL;
}

vec3 CalcPointLightPBR(LightData light, Surface surface, vec3 viewDir){
	float distance    = max(light.innerRadius + 0.01, length(light.pos - surface.pos));
	float attenuation = 1.0 / (distance * distance);
	//attenuation *= light.radius / 4.0; //asuming bigger radius = bigger light intensity
	attenuation *= pow(saturate(1.0 - pow(saturate(distance / light.radius), 4)), 2);
		
	vec3 lightDir = normalize(light.pos - surface.pos);
	float angle = acos(dot(light.dir, -lightDir));
	float attenuationFromSpotAngle = mix(1.f, 0.f, max(0.0, angle - light.halfInnerAngle) * light.halfDeltaAngleInv);
	attenuationFromSpotAngle = saturate(attenuationFromSpotAngle);
	attenuationFromSpotAngle = attenuationFromSpotAngle * attenuationFromSpotAngle;
	attenuation *= attenuationFromSpotAngle;
	
	vec3 lightRadiance     = light.color * attenuation;
	
	float extraCos = sin(min(PI * 0.5, atan(light.innerRadius / distance)));
	
	vec3 lightPosReflected = light.pos - surface.normal * dot(surface.normal,light.pos - surface.pos) * 2.0;
	
	float distanceFromView = length(lightPosReflected - u_cameraPos.xyz);
	float extraCosReflection = sin(min(PI * 0.5, atan(light.innerRadius / distanceFromView)));
	
	//return vec3_splat(attenuationFromSpotAngle);

	return CalcLightPBR(lightRadiance, lightDir, extraCos, extraCosReflection, surface, viewDir);
}

//TODO more than 1 light
vec3 CalcDirLightPBR(Surface surface, vec3 viewDir){
	float visibility = dirLightVisibility(surface.pos);
	if(visibility <= 0.0){
		return vec3_splat(0.0);
	}
	
	vec3 lightDir = -u_lightDir[0].xyz; //CalcLightPBR expects inverted
	vec3 lightRadiance = u_lightColor[0].rgb * visibility;
	
	return CalcLightPBR(lightRadiance, lightDir, 0.0, 0.0, surface, viewDir);
}

vec4 CalcPBR(Surface surface){
	vec4 proj = mul(u_viewProj, vec4(surface.pos, 1.0) );
	vec3 viewDir = normalize(u_cameraPos.xyz - surface.pos);
	
	vec4 color = vec4_splat(0.0);
	
	ClusterData clusterData = GetClasterData(proj);
	for(uint i = 0; i < clusterData.lightsCount; i++){
		ItemData item = GetItemData(i + clusterData.offset);
		LightData light = GetLightData(item.lightIdx);
		
		color.rgb += CalcPointLightPBR(light, surface, viewDir);
	}
	color.rgb += CalcDirLightPBR(surface, viewDir);
	
	//TODO IBL
	
	color.rgb += SampleSH(surface.normal, u_sphericalHarmonics) * surface.albedo.rgb; //TODO PBR
	
	color.rgb += surface.emissive.rgb;
	//color.rgb = vec3_splat(dirLightVisibility(surface.pos));
	return color;
}


void main()
{
	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);

	vec3 localNormal;
	localNormal.xy = texture2D(s_texNormal, v_texcoord0).xy * 2.0 - 1.0;
	localNormal.z = sqrt(1.0 - dot(localNormal.xy, localNormal.xy) );
	
	vec4 albedoAlpha = toLinear(texture2D(s_texColor, v_texcoord0));
	vec3 emissive = toLinear(texture2D(s_texEmissive, v_texcoord0)).rgb;
	
	float metalic = texture2D(s_texMetalic, v_texcoord0).r;
	float roughness = texture2D(s_texRoughness, v_texcoord0).r;
	
	Surface surface;
	surface.albedo = albedoAlpha.rgb;
	surface.alpha = albedoAlpha.a;
	surface.emissive = emissive.rgb * u_emissiveColor;
	surface.normal = normalize(mul(tbn, localNormal));
	surface.pos = v_wpos;
	surface.metalic = metalic;
	surface.roughness = roughness;
	surface.reflectionAtZeroIncidence = mix(vec3_splat(0.04), albedoAlpha.rgb, metalic);
	
	vec4 color = CalcPBR(surface);
	
	color.rgb = color.rgb / (vec4_splat(1.0) + color.rgb); // Reinhard tone mapping
	color.rgb = toGamma(color.rgb); // gamma correction
	gl_FragData[0].rgb = color;
	
#if TRANSPARENT
	gl_FragData[0].a = albedoAlpha.a;
#endif
}
