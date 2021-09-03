$input a_position, a_texcoord0
$output v_texcoord0

/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

void main()
{
	gl_Position = vec4(a_position.x * 2.0, a_position.y * 2.0, a_position.z, 1.0);
	v_texcoord0 = a_texcoord0;
}
