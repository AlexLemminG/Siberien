$input a_position, a_indices, a_weight
$output v_depth

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

	gl_Position = mul(u_viewProj, vec4(wpos, 1.0) );
	
	
	v_depth = gl_Position.z * 0.5 + 0.5;
}
