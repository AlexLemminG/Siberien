$input v_texcoord0

/*
 * Copyright 2018 Eric Arnebäck. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

uniform vec4 u_playerHealthParams;

SAMPLER2D(s_texColor,  0);

void main()
{
    vec2 texcoord = v_texcoord0;
	texcoord.y = 1.0-texcoord.y;
	
	vec4 color = texture2D(s_texColor, texcoord);
	
	float intensity = u_playerHealthParams.x;
	float intensityFromLastHit = u_playerHealthParams.y;
	
	intensity = min(1.0, intensity + intensityFromLastHit * 0.5);
	
	float grayscale = (color.r + color.g + color.b) / 3.0;
	vec3 grayscaleColor = vec3(grayscale,grayscale,grayscale);
	vec3 bloodyColor = vec3(1.0, 0.0, 0.0);
	float borderT = pow(pow(texcoord.x - 0.5, 2.0) + pow(texcoord.y - 0.5, 2.0), 1.0);
	
	color.rgb = lerp(color.rgb, grayscale, intensity);
	color.rgb = lerp(color.rgb, bloodyColor, borderT * intensity);
	
	gl_FragColor.rgb = color.rgb;
}
