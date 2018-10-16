#include <common>
#include <tonemapping.glsl>

layout(location = 0) out vec4 out_color;
#if MOTIONBLUR_ENABLED
layout(location = 1) out vec4 out_motion;
#endif

void main()
{
	vec4 color = vec4(1.0, 0.0, 1.0, 1.0);
#if POSTFX_ENABLED
    out_color = color;
#else
    out_color = tonemap(color);
#endif
#if MOTIONBLUR_ENABLED
	out_motion = vec4(0.0f);
#endif
}
