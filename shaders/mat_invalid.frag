#version 330

#include <common>

void main()
{
	gl_FragData[0] = vec4(1.0, 0.0, 1.0, 1.0);
#ifdef MOTIONBLUR_ENABLED
	gl_FragData[1] = vec4(0.0f);
#endif
}
