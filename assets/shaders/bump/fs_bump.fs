$input v_wpos, v_view, v_normal, v_tangent, v_bitangent, v_texcoord0// in...

/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

SAMPLER2D(s_texColor,  0);
SAMPLER2D(s_texNormal, 1);
SAMPLER2D(s_texEmissive, 2);


void main()
{
	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);

	vec3 normal;
	normal.xy = texture2D(s_texNormal, v_texcoord0).xy * 2.0 - 1.0;
	normal.z = sqrt(1.0 - dot(normal.xy, normal.xy) );

	vec3 wnormal = mul(tbn, normal);

	gl_FragData[0] = texture2D(s_texColor, v_texcoord0);
	gl_FragData[1] = vec4(encodeNormalUint(wnormal), 1.0);
	gl_FragData[3] = texture2D(s_texEmissive, v_texcoord0);
}
