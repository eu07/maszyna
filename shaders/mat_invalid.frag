#version 330

#include <common>
#include <tonemapping.glsl>

void main()
{
	vec4 color = vec4(1.0, 0.0, 1.0, 1.0);
#if POSTFX_ENABLED
    gl_FragData[0] = color;
#else
    gl_FragData[0] = tonemap(color);
#endif
#if MOTIONBLUR_ENABLED
	gl_FragData[1] = vec4(0.0f);
#endif
}
