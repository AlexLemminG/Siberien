$input a_position, a_normal, a_tangent, a_texcoord0, a_indices, a_weight
$output v_wpos, v_view, v_proj, v_normal, v_tangent, v_bitangent, v_texcoord0

/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */



#include "../common/common.sh"

void main()
{
	vec3 wpos = vec3(0.0,0.0,0.0);
	 mat4 transform = u_model[a_indices[0]] * a_weight[0];
	  transform += u_model[a_indices[1]] * a_weight[1];
	  transform += u_model[a_indices[2]] * a_weight[2];
	  transform += u_model[a_indices[3]] * a_weight[3];
	  
	  //transform = u_model[0];
	 
	wpos = mul(transform, vec4(a_position, 1.0) ).xyz;

	v_wpos = wpos;

	v_proj = mul(u_viewProj, vec4(wpos, 1.0) );
	gl_Position = v_proj;
	
	vec4 normal = a_normal;// * 2.0 - 1.0;
	vec4 tangent = a_tangent;// * 2.0 - 1.0;
	

	vec3 wnormal = mul(transform, vec4(normal.xyz, 0.0) ).xyz;
	vec3 wtangent = mul(transform, vec4(tangent.xyz, 0.0) ).xyz;

	v_normal = normalize(wnormal);
	// v_normal = vec3(0.0, 0.0, 1.0);
	v_tangent = normalize(wtangent);
	v_bitangent = cross(v_normal, v_tangent) * tangent.w;

	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);

	// eye position in world space
	vec3 weyepos = mul(vec4(0.0, 0.0, 0.0, 1.0), u_view).xyz;
	// tangent space view dir
	v_view = mul(weyepos - wpos, tbn);
	v_texcoord0 = a_texcoord0;
}
