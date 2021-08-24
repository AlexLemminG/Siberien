$input v_texcoord0

/*
 * Copyright 2018 Eric Arnebäck. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

SAMPLER2D(s_texColor,  0);

void main()
{	
	gl_FragColor = texture2D(s_texColor, v_texcoord0);
}
