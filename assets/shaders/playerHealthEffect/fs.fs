$input v_texcoord0

/*
 * Copyright 2018 Eric Arnebäck. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

uniform vec4 u_playerHealthParams;

SAMPLER2D(s_texColor, 0);
SAMPLER2D(s_win, 1);

void main()
{
	vec4 color = texture2D(s_texColor, v_texcoord0);
	
	float intensity = u_playerHealthParams.x;
	float intensityFromLastHit = u_playerHealthParams.y;
	float winCreenFade = u_playerHealthParams.z;
	
	intensity = min(1.0, intensity + intensityFromLastHit * 0.5);
	
	vec3 grayscaleColor = luma(color);
	vec3 bloodyColor = vec3(1.0, 0.0, 0.0);
	float borderT = pow(pow(v_texcoord0.x - 0.5, 2.0) + pow(v_texcoord0.y - 0.5, 2.0), 1.0);
	
	color.rgb = lerp(color.rgb, grayscaleColor, intensity);
	color.rgb = lerp(color.rgb, bloodyColor, borderT * intensity);
	color.rgb = lerp(color.rgb, texture2D(s_win, v_texcoord0), winCreenFade);
	
	gl_FragColor.rgb = color.rgb;
}
