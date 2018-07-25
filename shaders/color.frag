#version 330

in vec3 f_color;

#include <common>

void main()
{
	gl_FragData[0] = vec4(f_color, 1.0f);
#if MOTIONBLUR_ENABLED
	gl_FragData[1] = vec4(0.0f);
#endif
}
