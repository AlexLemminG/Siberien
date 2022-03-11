#define u_ambientPass    u_params0.x
#define u_lightingPass   u_params0.y
#define u_hasShadowMap   u_params0.z

#define u_shadowMapBias   u_params1.x
#define u_shadowMapParam0 u_params1.z
#define u_shadowMapParam1 u_params1.w

#define u_shadowMapShowCoverage u_params2.y
#define u_shadowMapTexelSize    u_params2.z

#define u_spotDirection   u_lightSpotDirectionInner.xyz
#define u_spotInner       u_lightSpotDirectionInner.w
#define u_lightAttnParams u_lightAttenuationSpotOuter.xyz
#define u_spotOuter       u_lightAttenuationSpotOuter.w

// Pcf
#define u_shadowMapPcfMode     u_shadowMapParam0
#define u_shadowMapNoiseAmount u_shadowMapParam1

// Vsm
#define u_shadowMapMinVariance     u_shadowMapParam0
#define u_shadowMapDepthMultiplier u_shadowMapParam1

// Esm
#define u_shadowMapHardness        u_shadowMapParam0
#define u_shadowMapDepthMultiplier u_shadowMapParam1

if(u_hasShadowMap > 0.0){
	vec3 colorCoverage;

#if SM_CSM
	vec2 texelSize = vec2_splat(u_shadowMapTexelSize);

	vec2 texcoord1 = v_texcoord1.xy/v_texcoord1.w;
	vec2 texcoord2 = v_texcoord2.xy/v_texcoord2.w;
	vec2 texcoord3 = v_texcoord3.xy/v_texcoord3.w;
	vec2 texcoord4 = v_texcoord4.xy/v_texcoord4.w;

	bool selection0 = all(lessThan(texcoord1, vec2_splat(0.99))) && all(greaterThan(texcoord1, vec2_splat(0.01)));
	bool selection1 = all(lessThan(texcoord2, vec2_splat(0.99))) && all(greaterThan(texcoord2, vec2_splat(0.01)));
	bool selection2 = all(lessThan(texcoord3, vec2_splat(0.99))) && all(greaterThan(texcoord3, vec2_splat(0.01)));
	bool selection3 = all(lessThan(texcoord4, vec2_splat(0.99))) && all(greaterThan(texcoord4, vec2_splat(0.01)));

	//TODO numSplits as parameter
	//selection1 = false;
	selection2 = false;
	selection3 = false;

	if (selection0)
	{
		vec4 shadowcoord = v_texcoord1;

		float coverage = texcoordInRange(shadowcoord.xy/shadowcoord.w) * 0.4;
		colorCoverage = vec3(-coverage, coverage, -coverage);
		visibility = computeVisibility(s_shadowMap0
						, s_shadowMap0Simple
						, shadowcoord
						, u_shadowMapBias
						, u_smSamplingParams
						, texelSize
						, u_shadowMapDepthMultiplier
						, u_shadowMapMinVariance
						, u_shadowMapHardness
						);
	}
	else if (selection1)
	{
		vec4 shadowcoord = v_texcoord2;

		float coverage = texcoordInRange(shadowcoord.xy/shadowcoord.w) * 0.4;
		colorCoverage = vec3(coverage, coverage, -coverage);
		visibility = computeVisibility(s_shadowMap1, s_shadowMap0Simple
						, shadowcoord
						, u_shadowMapBias
						, u_smSamplingParams
						, texelSize/2.0
						, u_shadowMapDepthMultiplier
						, u_shadowMapMinVariance
						, u_shadowMapHardness
						);
	}
	else if (selection2)
	{
		vec4 shadowcoord = v_texcoord3;

		float coverage = texcoordInRange(shadowcoord.xy/shadowcoord.w) * 0.4;
		colorCoverage = vec3(-coverage, -coverage, coverage);
		visibility = computeVisibility(s_shadowMap2, s_shadowMap0Simple
						, shadowcoord
						, u_shadowMapBias
						, u_smSamplingParams
						, texelSize/3.0
						, u_shadowMapDepthMultiplier
						, u_shadowMapMinVariance
						, u_shadowMapHardness
						);
	}
	else if(selection3)
	{
		vec4 shadowcoord = v_texcoord4;

		float coverage = texcoordInRange(shadowcoord.xy/shadowcoord.w) * 0.4;
		colorCoverage = vec3(coverage, -coverage, -coverage);
		visibility = computeVisibility(s_shadowMap3, s_shadowMap0Simple
						, shadowcoord
						, u_shadowMapBias
						, u_smSamplingParams
						, texelSize/4.0
						, u_shadowMapDepthMultiplier
						, u_shadowMapMinVariance
						, u_shadowMapHardness
						);
	}else{
		vec4 shadowcoord = v_texcoord4;

		float coverage = texcoordInRange(shadowcoord.xy/shadowcoord.w) * 0.4;
		colorCoverage = vec3(coverage, -coverage, -coverage);
		visibility = 1.0;
	}
#elif SM_OMNI
	vec2 texelSize = vec2_splat(u_shadowMapTexelSize/4.0);

	vec4 faceSelection;
	vec3 pos = v_position.xyz;
	faceSelection.x = dot(u_tetraNormalGreen.xyz,  pos);
	faceSelection.y = dot(u_tetraNormalYellow.xyz, pos);
	faceSelection.z = dot(u_tetraNormalBlue.xyz,   pos);
	faceSelection.w = dot(u_tetraNormalRed.xyz,    pos);

	vec4 shadowcoord;
	float faceMax = max(max(faceSelection.x, faceSelection.y), max(faceSelection.z, faceSelection.w));
	if (faceSelection.x == faceMax)
	{
		shadowcoord = v_texcoord1;

		float coverage = texcoordInRange(shadowcoord.xy/shadowcoord.w) * 0.3;
		colorCoverage = vec3(-coverage, coverage, -coverage);
	}
	else if (faceSelection.y == faceMax)
	{
		shadowcoord = v_texcoord2;

		float coverage = texcoordInRange(shadowcoord.xy/shadowcoord.w) * 0.3;
		colorCoverage = vec3(coverage, coverage, -coverage);
	}
	else if (faceSelection.z == faceMax)
	{
		shadowcoord = v_texcoord3;

		float coverage = texcoordInRange(shadowcoord.xy/shadowcoord.w) * 0.3;
		colorCoverage = vec3(-coverage, -coverage, coverage);
	}
	else // (faceSelection.w == faceMax)
	{
		shadowcoord = v_texcoord4;

		float coverage = texcoordInRange(shadowcoord.xy/shadowcoord.w) * 0.3;
		colorCoverage = vec3(coverage, -coverage, -coverage);
	}

	visibility = computeVisibility(s_shadowMap0, s_shadowMap0Simple
					, shadowcoord
					, u_shadowMapBias
					, u_smSamplingParams
					, texelSize
					, u_shadowMapDepthMultiplier
					, u_shadowMapMinVariance
					, u_shadowMapHardness
					);
#else
	vec2 texelSize = vec2_splat(u_shadowMapTexelSize);

	float coverage = texcoordInRange(v_shadowcoord.xy/v_shadowcoord.w) * 0.3;
	colorCoverage = vec3(coverage, -coverage, -coverage);

	visibility = computeVisibility(s_shadowMap0, s_shadowMap0Simple
					, v_shadowcoord
					, u_shadowMapBias
					, u_smSamplingParams
					, texelSize
					, u_shadowMapDepthMultiplier
					, u_shadowMapMinVariance
					, u_shadowMapHardness
					);
#endif
	//visibility = (1.0 + colorCoverage.x)*0.5*0.5 + (1.0 + colorCoverage.y)*0.5*0.25;// + (1.0 + colorCoverage.z)*0.5*0.125;
}else{
	//u_hasShadowMap == 0.0
	visibility = 1.0;
}
