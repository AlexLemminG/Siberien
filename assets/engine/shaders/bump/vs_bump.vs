$input a_position, a_normal, a_tangent, a_texcoord0, a_indices, a_weight
$output v_wpos, v_normal, v_tangent, v_bitangent, v_texcoord0

/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

void main()
{
	mat4 transform = u_model[a_indices[0]] * a_weight[0];
	transform += u_model[a_indices[1]] * a_weight[1];
	transform += u_model[a_indices[2]] * a_weight[2];
	transform += u_model[a_indices[3]] * a_weight[3];

	v_wpos = mul(transform, vec4(a_position, 1.0) ).xyz;

	gl_Position = mul(u_viewProj, vec4(v_wpos, 1.0) );

	vec3 wnormal = mul(transform, vec4(a_normal.xyz, 0.0) ).xyz;
	vec3 wtangent = mul(transform, vec4(a_tangent.xyz, 0.0) ).xyz;

	v_normal = normalize(wnormal);
	v_tangent = normalize(wtangent);
	v_bitangent = cross(v_normal, v_tangent) * a_tangent.w;

	v_texcoord0 = a_texcoord0;
}
